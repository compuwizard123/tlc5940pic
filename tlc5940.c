/************************************************************************
	tlc5940.c

    TLC5940 LED driver PIC library
    Copyright (C) 2011 Kevin Risden

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	Email: compuwizard123@gmail.com

************************************************************************/
// Global includes
#include <p18f4520.h>
#include <pwm.h>
#include <spi.h>
#include <timers.h>

// Local includes
#include "tlc5940.h"

// Array for storing the grayscale data packed into bytes
unsigned char packedGrayScaleDataBuffer1[24 * NUMBEROF5940];
unsigned char packedGrayScaleDataBuffer2[24 * NUMBEROF5940];

// Flags for the interrupt handling routine
unsigned char waitingForXLAT = 0;
unsigned char updatePending = 0;

// Set initial dot correction data
void setInitialDotCorrection(unsigned char *dotCorrectionValues)
{	
	int ledChannel;
	int bitCounter;
	// Set VPRG high (Dot correction mode)
	TLC5940_VPRG = 1;
	
	// We are passed an array of unsigned char values which are 8 bits each, however the dot 
	// correction is expecting 6 bit data for each channel (0-63) so only send the 6 least
	// significant bits of each entry in the array.  The values need to be sent MSB first.
	for(ledChannel = 0; ledChannel < (16 * NUMBEROF5940); ledChannel++) {
		unsigned char bitMask = 0b00100000;
		
		for(bitCounter = 5; bitCounter >= 0; bitCounter--) {
			// Set SIN to DC data bit
			TLC5940_SIN = (dotCorrectionValues[ledChannel] & bitMask) >> bitCounter;
			
			// Pulse the serial clock
			TLC5940_SCLK = 1;
			TLC5940_SCLK = 0;
			
			// Move to the next bit in the mask
			bitMask >>= 1;
		}
	}	
	
	// Pulse XLAT
	TLC5940_XLAT = 1;
	TLC5940_XLAT = 0;
}

// Since the TLC5940 requires an 'extra' SCLK pulse the first time the grayscales
// are cycled we have to do one input cycle manually, after this we can use the SPI
// module for extra serial interface speed (this is due to the fact that there is
// no way to tell the SPI module to generate the extra clock pulse).
void setInitialGrayScaleValues()
{
	int i;
	// Reset GSCLK_Counter = 0
	int GSCLKcounter = 0;
	
	// Reset Data_Counter = 0
	int dataCounter = 0;
	
	// Set VPRG = Low (Grayscale mode)
	TLC5940_VPRG = 0;
	
	// Set BLANK = High (Turn LED's Off)
	TLC5940_BLANK = 1;
	
	for(GSCLKcounter = 0; GSCLKcounter < 4096; GSCLKcounter++) {
		if(dataCounter > (NUMBEROF5940 * 192)) {
			// Pulse GSCLK
			TLC5940_GSCLK = 1;
			TLC5940_GSCLK = 0;
		} else {
			// Set SIN to the greyscale data bit
			TLC5940_SIN = 0; // We just output zero for everything during initialisation
			
			// Pulse SCLK
			TLC5940_SCLK = 1;
			TLC5940_SCLK = 0;
			
			// Increment Data_Counter
			dataCounter++;
			
			// Pulse GSCLK
			TLC5940_GSCLK = 1;
			TLC5940_GSCLK = 0;
		}	
	}
	
	// Pulse XLAT to latch in GS data
	TLC5940_XLAT = 1;
	TLC5940_XLAT = 0;

	// Set BLANK = Low (Turn LED's on)
	TLC5940_BLANK = 0;
	
	// Send an extra SCLK pulse since this is the first grayscale cycle
	// after the dot correction data has been set
		
	// Pulse SCLK
	TLC5940_SCLK = 1;
	TLC5940_SCLK = 0;
	
	// blank all outputs
	for(i=0;i<16;i++) {
		setGrayScaleValue(i, 0);
	}
	while(updateTlc5940() == 1);
}	

// Process XLAT interrupt
void processXLATinterrupt(void)
{
	int byteCounter;
	// Turn off the LEDs
	TLC5940_BLANK = 1;
	
	// Are we waiting for an XLAT pulse to latch new data?
	if(waitingForXLAT == 1) {
		// Pulse the XLAT signal
		TLC5940_XLAT = 1;
		TLC5940_XLAT = 0;
		
		// Clear the flag
		waitingForXLAT = 0;
	}
	
	// Turn on the LEDs
	TLC5940_BLANK = 0;
	
	// Note: Once we have reset the 5940's PWM counter by toggling the BLANK pin we can
	// shift in the serial data (since the PWM pulse for GSCLK continues to run
	// in the background).  The shifting of the data must happen before the next
	// XLAT interrupt is due.
	
	// Do we have an update to the data pending?
	if(updatePending == 1) {
		// We have an update pending, write the serial information to the device
		PIR1bits.SSPIF = 0;
		for(byteCounter = 0; byteCounter < (24 * NUMBEROF5940); byteCounter++) {
			// Send the byte to the SPI buffer
			WriteSPI(packedGrayScaleDataBuffer2[byteCounter]);
			
			// Wait for the transmission complete flag
			while(PIR1bits.SSPIF == 0);
			
			// Reset the transmission complete flag
			PIR1bits.SSPIF = 0;
		}
		
		// Serial data is now updated, clear the flag
		updatePending = 0;
		
		// Set the waiting for XLAT flag to indicate there is data waiting
		// to be latched
		waitingForXLAT = 1;
	}
}				

// Initialise the TLC5940 devices
void initialiseTlc5940()
{
	unsigned char dotCorrectionValues[16 * NUMBEROF5940];
	unsigned char ledChannel;
	
	// Initialise device pins
	TLC5940_GSCLK 	= 0;
	TLC5940_SCLK 	= 0;
	TLC5940_VPRG 	= 1;
	TLC5940_XLAT 	= 0;
	TLC5940_BLANK 	= 1;
	
	// Set up an array of dot correction values (0-63)
	for(ledChannel = 0; ledChannel < (16 * NUMBEROF5940); ledChannel++) {
		dotCorrectionValues[ledChannel] = 63;
	}
	
	// Set the initial dot correction values
	setInitialDotCorrection(dotCorrectionValues);
	
	// Set the initial grayscale values
	setInitialGrayScaleValues();
	
	// Set up SPI for communicating with the TLC5940
	OpenSPI(SPI_FOSC_4, MODE_00, SMPEND);

	// PWM1 is used to generate the GSCLK clock signal
	//
	// We want an overall PWM period of around 60Hz
	// There are 4096 PWM steps per period so we need to pulse
	// around 60Hz * 4096 steps = 245,760Hz, so we round up to
	// the next even value of 250,000 Hz
	//
	// PWM 1 is 250,000 Hz (250 KHz)
	
	OpenTimer2(TIMER_INT_OFF & T2_PS_1_1);
	SetOutputPWM1(SINGLE_OUT, PWM_MODE_1);
	OpenPWM1(7);
	CCPR1L = 0b00000001;
	CCP1CON = 0b00011100;
	
	// An interrupt on timer0 is used to generate the XLAT signal (one for
	// every 4096 GSCLK pulses)
	//
	// Timer0 should interrupt every 4096 pulses of the PWM1
	// PWM1 period is 250,000 Hz which is one pulse every 4 uS
	// We need to wait for 4096 pulses before calling the interrupt
	// so 4 uS * 4096 = 16384 uS
	//
	// Fosc is 1,000,000 (1Mhz), so Fosc/4 = 250,000
	// Therefore there are 1 ticks per 4uS
	//
	// 16,384 uS * 1/4 = 4096 timer ticks at 1:1 prescale
	//
	// Therefore we need to set the timer to 65,535 - 8 192
	// 53247 = CFFF

	OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_2);
	INTCON2bits.TMR0IP = 1;				// Set timer0 interrupt to high priority
	INTCONbits.TMR0IF = 0;				// Clear the timer0 interrupt flag
	WriteTimer0(XLATCOUNTTIMER);

	RCONbits.IPEN = 1;      // Enable prioritised interrupts
	INTCONbits.GIEH = 1;	// Global enable high priority interrupts
	INTCONbits.GIEL = 1;    // Global enable low priority interrupts
}

// Set the grayscale value of a LED channel
//
// Note: The TLC5940 expects 12 bit values for each channel, however we store
// the values in an 8 bit array (since that is what we need for sending the 
// data over the SPI).  This function places our 12 bit value in the correct
// place.
void setGrayScaleValue(unsigned char channel, int value)
{
	unsigned char eightBitIndex = (NUMBEROF5940 * 16 - 1) - channel;
	unsigned char *twelveBitIndex = packedGrayScaleDataBuffer1 + ((eightBitIndex * 3) >> 1);
	
	if(eightBitIndex & 1) {
		// Value starts in the middle of the byte
		// Set only the top 4 bits
		*twelveBitIndex = (*twelveBitIndex & 0xF0) | (value >> 8);
		
		// Now set the lower 4 bits of the next byte
		*(++twelveBitIndex) = value & 0xFF;
	} else {
		// Value starts at the start of the byte
		*(twelveBitIndex++) = value >> 4;
		
		// Now set the 4 lower bits of the next byte leaving the top 4 bits alone
		*twelveBitIndex = ((unsigned char)(value << 4)) | (*twelveBitIndex & 0xF);
	}		
}		

// Update the TLC5940 send buffer
unsigned char updateTlc5940(void)
{
	int byteCounter;
	
	// If an update is already pending, return with status 0;
	if(updatePending == 1) {
	  return 0;
	}
	
	// Copy over our packed data buffer to the send data buffer
	// Note: We are using double-buffering to prevent a partial
	// update from occurring (since an XLAT interrupt could occur
	// whilst we are still updating the data)
	for(byteCounter = 0; byteCounter < (24 * NUMBEROF5940); byteCounter++) {
		packedGrayScaleDataBuffer2[byteCounter] = packedGrayScaleDataBuffer1[byteCounter];
	}
		
	// Set the update pending flag
	updatePending = 1;
	
	// Update ok, return with status 0
	return 0;
}
