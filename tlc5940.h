/************************************************************************
	tlc5940.h

    TLC5940 LED driver library
    Copyright (C) 2010 Simon Inns

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

	Email: simon.inns@gmail.com

************************************************************************/

#ifndef TLC5940_H
#define TLC5940_H

// TLC5940 Hardware mapping definitions

// Note: SIN, SCLK and GSCLK are tied to module functions on the PIC
// and you can't change these without changing the appropriate library
// functions.
#define TLC5940_SIN		RC7
#define TLC5940_SCLK	RB1
#define TLC5940_GSCLK	RC2

// These you can assign to other pins if required
#define TLC5940_XLAT	RA0
#define TLC5940_VPRG	RA1
#define TLC5940_BLANK	RA2

// XLAT timer period
#define XLATCOUNTH 		0x3F
#define XLATCOUNTL 		0xFF

// The number of cascaded TLC5940s
#define NUMBEROF5940	3

// Function prototypes
void setInitialDotCorrection(unsigned char *dotCorrectionValues);
void setInitialGrayScaleValues(void);
void processXLATinterrupt(void);
void initialiseTlc5940(void);
void setGrayScaleValue(unsigned char channel, int value);
unsigned char updateTlc5940(void);
void interrupt low_priority lpHandler(void);

#endif