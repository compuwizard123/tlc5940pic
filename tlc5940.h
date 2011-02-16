/************************************************************************
	tlc5940.h

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

#ifndef TLC5940_H
#define TLC5940_H

#include <p18f4520.h>

// TLC5940 Hardware mapping definitions

// Note: SIN, SCLK and GSCLK are tied to module functions on the PIC
#define TLC5940_SIN		PORTCbits.RC5	//SDO
#define TLC5940_SCLK	PORTCbits.RC3	//SCK/SCL
#define TLC5940_GSCLK	PORTCbits.RC2	//CCP1

// These you can assign to other pins if required
#define TLC5940_XLAT	PORTAbits.RA0
#define TLC5940_VPRG	PORTAbits.RA1
#define TLC5940_BLANK	PORTAbits.RA2

// XLAT timer period
#define XLATCOUNTTIMER	53247

// The number of cascaded TLC5940s
#define NUMBEROF5940	1

// Function prototypes
void setInitialDotCorrection(unsigned char *dotCorrectionValues);
void setInitialGrayScaleValues(void);
void processXLATinterrupt(void);
void initialiseTlc5940(void);
void setGrayScaleValue(unsigned char channel, int value);
unsigned char updateTlc5940(void);

#endif
