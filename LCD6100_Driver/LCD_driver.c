//************************************************************************
//*
//*				LCD_driver.c: Interface for Nokia LCD
//*
//************************************************************************
//*	Edit History
//*		<MLS>	= Mark Sproul msproul -at- jove.rutgers.edu
//************************************************************************
//*	Apr  2,	2010	<MLS> I received my Color LCD Shield sku: LCD-09363 from sparkfun
//*	Apr  2,	2010	<MLS> The code was written for WinAVR, I modified it to compile under Arduino
//*	Apr  3,	2010	<MLS> Changed LCDSetPixel to make it "RIGHT" side up
//*	Apr  3,	2010	<MLS> Made LCDSetPixel public
//*	Apr  3,	2010	<MLS> Working on MEGA, pin defs in nokia_tester.h
//*	Apr  4,	2010	<MLS> Removed delays from spi_LCD_command & spi_LCD_command, increased speed by 75%

// September 2011   <pls> modified for freeRTOS and using PRGMEM for font storage (save SRAM)
//                        added new xLCDPrint and xLCDPrintf style functions.
//************************************************************************


#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include "SPI9master.h"
#include "LCD_driver.h"


//************************************************************************
// Define some local functions. Try not to use these, as the xLDCPrint
// functions are much nicer.
//************************************************************************

// static void xLCDPutStr(   int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, uint8_t *pString);
// static void xLCDPutStr_P( int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, PGM_P pString); // from PRGMEM

static void xLCDPutChar(  int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, uint8_t c);

//************************************************************************
//Usage: LCDInit();
//Inputs: None
//Outputs: None
//Description:  Initializes the LCD regardless of if the controller is an EPSON or PHILLIPS.
//************************************************************************
void xLCDInit(void)
{

	spi_LCD_init();					// initialise the SPI bus for 9 bit LCD transfers.
	_delay_ms(100);

	cbi(SPI9_PORT, SPI9_SCK);			//output_low (SPI_CLK);//output_low (SPI_MOSI);
	cbi(SPI9_PORT, SPI9_MOSI);
	_delay_us(10);
	sbi(SPI9_PORT, LCD_SPI_SS);		//output_high (LCD_CS);
	_delay_us(10);
	cbi(SPI9_PORT, LCD_RESET);		//output_low (LCD_RESET);
	_delay_ms(100);
	sbi(SPI9_PORT, LCD_RESET);		//output_high (LCD_RESET);
	_delay_ms(100);
	sbi(SPI9_PORT, SPI9_SCK);
	sbi(SPI9_PORT, SPI9_MOSI);
	_delay_us(10);

	spi_LCD_command(DISCTL);	// display control(EPSON)
	spi_LCD_data(0x0C);			// 12 = 1100 - CL dividing ratio [don't divide] switching period 8H (default)
	spi_LCD_data(0x20);
	//spi_LCD_data(0x02);
	spi_LCD_data(0x00);

	spi_LCD_data(0x01);

	spi_LCD_command(COMSCN);	// common scanning direction(EPSON)
	spi_LCD_data(0x01);

	spi_LCD_command(OSCON);		// internal oscillator ON(EPSON)

	spi_LCD_command(SLPOUT);		// sleep out(EPSON)
	spi_LCD_command(P_SLEEPOUT);	//sleep out(PHILLIPS)

	spi_LCD_command(PWRCTR);	// power ctrl(EPSON)
	spi_LCD_data(0x0F);			//everything on, no external reference resistors
	spi_LCD_command(P_BSTRON);	//Booster voltage set ON (PHILLIPS)

	spi_LCD_command(DISINV);	// invert display mode (EPSON)
	spi_LCD_command(P_INVON);	// invert display mode (PHILLIPS)

	spi_LCD_command(DATCTL);	// data control(EPSON)
	spi_LCD_data(0x03);			// correct for normal sin7
	spi_LCD_data(0x00);			// normal RGB arrangement
	//spi_LCD_data(0x01);		// 8-bit Grayscale
	spi_LCD_data(0x02);			// 16-bit Grayscale Type A

	spi_LCD_command(P_MADCTL);	//Memory Access Control(PHILLIPS)
	spi_LCD_data(0xC8);

	spi_LCD_command(P_COLMOD);	// Set Color Mode(PHILLIPS)
	spi_LCD_data(0x02);

	spi_LCD_command(VOLCTR);	// electronic volume, this is the contrast/brightness(EPSON)
	//spi_LCD_data(0x18);		// volume (contrast) setting - fine tuning, original
	spi_LCD_data(0x24);			// volume (contrast) setting - fine tuning, original
	spi_LCD_data(0x03);			// internal resistor ratio - coarse adjustment
	spi_LCD_command(P_SETCON);	// Set Contrast(PHILLIPS)
	spi_LCD_data(0x30);

	spi_LCD_command(NOP);		// nop(EPSON)
	spi_LCD_command(P_NOP);		// nop(PHILLIPS)

	_delay_ms(100);

	spi_LCD_command(DISON);		// display on(EPSON)
	spi_LCD_command(P_DISPON);	// display on(PHILLIPS)
}


//************************************************************************
//Usage: LCDClear(black);
//Inputs: char color: 8-bit color to be sent to the screen.
//Outputs: None
//Description: This function will clear the screen with "color" by writing the
//				color to each location in the RAM of the LCD.
//************************************************************************
void xLCDClear(int16_t color)
{
uint16_t i;

	#ifdef EPSON
		spi_LCD_command(PASET);
		spi_LCD_data(0);
		spi_LCD_data(131);

		spi_LCD_command(CASET);
		spi_LCD_data(0);
		spi_LCD_data(131);

		spi_LCD_command(RAMWR);
	#endif
	#ifdef	PHILLIPS
		spi_LCD_command(PASETP);
		spi_LCD_data(0);
		spi_LCD_data(131);

		spi_LCD_command(CASETP);
		spi_LCD_data(0);
		spi_LCD_data(131);

		spi_LCD_command(RAMWRP);
	#endif

	for (i=0; i < (131*131)/2; i++)
	{
		spi_LCD_data((color >> 4) & 0x00FF);
		spi_LCD_data(((color & 0x0F) << 4) | (color >> 8));
		spi_LCD_data(color & 0x0FF);		// nop(EPSON)
	}

//	x_offset = 0;
//	y_offset = 0;
}


void xLCDContrast(uint8_t setting) {
     spi_LCD_command(VOLCTR);       // electronic volume, this is the contrast/brightness(EPSON)
     spi_LCD_data(setting);         // volume (contrast) setting - course adjustment,  -- original was 24
//     spi_LCD_data(0x03);          // internal resistor ratio - coarse adjustment
//     spi_LCD_data(0x30);

     spi_LCD_command(NOP);        // nop (EPSON)
}



//************************************************************************
//Usage: LCDDrawPixel(white, 0, 0);
//Inputs: unsigned char color - desired color of the pixel
//			unsigned char x - Page address of pixel to be coloured
//			unsigned char y - column address of pixel to be coloured
//Outputs: None
//Description: Sets the starting page(row) and column (x & y) coordinates in ram,
//				then writes the colour to display memory.	The ending x & y are left
//				maxed out so one can continue sending colour data bytes to the 'open'
//				RAMWR command to fill further memory.
//				Issuing any read command finishes RAMWR.
//************************************************************************
void xLCDDrawPixel(int16_t color, uint8_t x, uint8_t y)
{
	y	=	(COL_HEIGHT - 1) - y;
	x   =   (ROW_LENGTH - 1) - x;

	#ifdef EPSON
		spi_LCD_command(PASET);	// page start/end ram
		spi_LCD_data(x);
		spi_LCD_data(x); //ENDPAGE

		spi_LCD_command(CASET);	// column start/end ram
		spi_LCD_data(y);
		spi_LCD_data(y); //ENDCOL

		spi_LCD_command(RAMWR);	// write
		spi_LCD_data((color>>4)&0x00FF);
		spi_LCD_data(((color&0x0F)<<4)|(color>>8));
		spi_LCD_data(color&0x0FF);		// nop(EPSON)
	#endif
	#ifdef	PHILLIPS
		spi_LCD_command(P_PASET);	// page start/end ram
		spi_LCD_data(x);
		spi_LCD_data(x); //ENDPAGE

		spi_LCD_command(P_CASET);	// column start/end ram
		spi_LCD_data(y);
		spi_LCD_data(y); // ENDCOL

		spi_LCD_command(P_RAMWR);	// write

		spi_LCD_data((uint8_t)((color>>4)&0x00FF));
		spi_LCD_data((uint8_t)(((color&0x0F)<<4)|0x00));
	#endif
}



//************************************************************************
//***                        Draw line                                 ***
//************************************************************************
void xLCDDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t color)
{
	int16_t dy = y1 - y0;// Difference between y0 and y1
	int16_t dx = x1 - x0;// Difference between x0 and x1
	int16_t stepx, stepy;
	if (dy < 0) { dy = -dy; stepy = -1; } else { stepy = 1; }
	if (dx < 0) { dx = -dx; stepx = -1; } else { stepx = 1; }
	dy <<= 1;// dy is now 2*dy
	dx <<= 1;// dx is now 2*dx

	xLCDDrawPixel(color, x0, y0);
	if (dx > dy)
	{
		int16_t fraction = dy - (dx >> 1);
		while (x0 != x1)
		{
			if (fraction >= 0)
			{
				y0 += stepy;
				fraction -= dx;
			}
			x0 += stepx;
			fraction += dy;
			xLCDDrawPixel(color, x0, y0);
		}
	}
	else
	{
		int16_t fraction = dx - (dy >> 1);
		while (y0 != y1)
		{
			if (fraction >= 0)
			{
				x0 += stepx;
				fraction -= dy;
			}
			y0 += stepy;
			fraction += dx;
			xLCDDrawPixel(color, x0, y0);
		}
	}
}
//************************************************************************
//***                   Draw rectangle                                 ***
//************************************************************************
void xLCDDrawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t fill, int16_t color)
{
	// check if the rectangle is to be filled
	if (fill == 1)
	{
		int16_t xDiff;
		if(x0 > x1)	xDiff = x0 - x1; //Find the difference between the x vars
		else        xDiff = x1 - x0;

		while(xDiff > 0)
		{
			xLCDDrawLine(x0, y0, x0, y1, color);
			if(x0 > x1) x0--;
			else x0++;

			xDiff--;
		}
	}
	else
	{
		// best way to draw an unfilled rectangle is to draw four lines
		xLCDDrawLine(x0, y0, x1, y0, color);
		xLCDDrawLine(x0, y1, x1, y1, color);
		xLCDDrawLine(x0, y0, x0, y1, color);
		xLCDDrawLine(x1, y0, x1, y1, color);
	}
}


// *************************************************************************************************
// LCDDrawCircle
//
// Draws a full circle, half a circle, or a quarter of a circle
// Inputs:
// xCenter the x for the center of the circle
// yCenter the y for the center of the circle
// radius the radius for the circle
// circleType the type of circle see LCD_driver.h for types of circles
// colour the colour of the circle
//
//  This routine can be used to draw a complete circle, half circle or a quarter of a circle
//
// Returns: Nothing
//
// I found the original code at http://www.arduino.cc/playground/Code/GLCDks0108 and modified it for
// this library
//
// *************************************************************************************************

void xLCDDrawCircle (int16_t xCenter, int16_t yCenter, int16_t radius, int16_t circleType, int16_t color)
{
	int16_t tSwitch, x1 = 0, y1 = radius;
	int16_t Width = 2*radius, Height=Width;
	tSwitch = 3 - 2 * radius;

	while (x1 <= y1)
	{
		if (circleType == FULLCIRCLE||circleType == OPENSOUTH||circleType == OPENEAST||circleType == OPENSOUTHEAST)
		{
			xLCDDrawPixel(color,xCenter+radius - x1, yCenter+radius - y1);
			xLCDDrawPixel(color,xCenter+radius - y1, yCenter+radius - x1);
		}
		if (circleType == FULLCIRCLE||circleType == OPENNORTH||circleType == OPENEAST||circleType == OPENNORTHEAST)
		{
			xLCDDrawPixel(color,xCenter+Width-radius + x1, yCenter+radius - y1);
			xLCDDrawPixel(color,xCenter+Width-radius + y1, yCenter+radius - x1);
		}
		if (circleType == FULLCIRCLE||circleType == OPENNORTH||circleType == OPENWEST||circleType == OPENNORTHWEST)
		{
			xLCDDrawPixel(color,xCenter+Width-radius + x1, yCenter+Height-radius + y1);
			xLCDDrawPixel(color,xCenter+Width-radius + y1, yCenter+Height-radius + x1);
		}
		if (circleType == FULLCIRCLE||circleType == OPENSOUTH||circleType == OPENWEST||circleType == OPENSOUTHWEST)
		{
			xLCDDrawPixel(color,xCenter+radius - x1, yCenter+Height-radius + y1);
			xLCDDrawPixel(color,xCenter+radius - y1, yCenter+Height-radius + x1);
		}

		if (tSwitch < 0) tSwitch += (4 * x1 + 6);
		else
		{
			tSwitch += (4 * (x1 - y1) + 10);
			y1--;
		}
		x1++;
	}
}


/*-----------------------------------------------------------------*/

// xLCDPrintf_P( x, y, font, foregroundColor, backgroundColor, PSTR("\r\nMessage %u %u %u"), var1, var2, var2);

void xLCDPrintf( int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, char * format, ...)
{
	va_list arg;
	char temp[ROW_LENGTH/8+1];

	va_start(arg, format);
	vsnprintf(temp, ROW_LENGTH/8+1, format, arg);
	xLCDPrint( x, y, font, fColor,  bColor, temp);
	va_end(arg);
}

void xLCDPrintf_P( int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, PGM_P format, ...)
{
	va_list arg;
	char temp[ROW_LENGTH/8+1];

	va_start(arg, format);
	vsnprintf_P(temp, ROW_LENGTH/8+1, format, arg);
	xLCDPrint( x, y, font, fColor,  bColor, temp);
	va_end(arg);
}

void xLCDPrint( int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, char * str)
{
	int16_t i = 0;
	size_t stringlength;

 	x += 16; // adjust for the right location for 0,0
	y += 8;

	stringlength = strlen(str);

	while(i < stringlength)
	{
		xLCDPutChar( x, y, font, fColor, bColor, str[i++] );

		// advance the y position
		y += 8;

		// bail out if y exceeds 131
		if (y > ENDCOL) break;

	}
}

void xLCDPrint_P( int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, PGM_P str)
{
	uint16_t i = 0;
	size_t stringlength;

 	x += 16; // adjust for the right location for 0,0
	y += 8;

	stringlength = strlen_P(str);

	while(i < stringlength)
	{
		xLCDPutChar( x, y, font, fColor, bColor, pgm_read_byte(&str[i++]) );

		// advance the y position
		y += 8;

		// bail out if y exceeds 131
		if (y > ENDCOL) break;
	}
}


//*****************************************************************************
//LCDPutChar.c
//Draws an ASCII character at the specified (x,y) address and color
//Inputs:
//c      = character to be displayed
//x      = row address (0 .. 131)
//y      = column address (0 .. 131)
//fcolor = 12-bit foreground color value
//bcolor = 12-bit background color value
//Returns: nothing
// Author: James P Lynch, August 30, 2007
// Edited by Peter Davenport on August 23, 2010
//For more information on how this code does it's thing look at this
//"http://www.sparkfun.com/tutorial/Nokia%206100%20LCD%20Display%20Driver.pdf"
//*****************************************************************************
static void xLCDPutChar( int16_t x, int16_t y, uint8_t font, int16_t fColor, int16_t bColor, uint8_t c )
{
	y	=	(COL_HEIGHT - 1) - y; // make display "right" side up for Sparkfun
	x	=	(ROW_LENGTH - 2) - x;

	extern const uint8_t PROGMEM FONT8x8[];  // The fonts are stored in PRGMEM
	extern const uint8_t PROGMEM FONT8x16[];

	int16_t     i,j;
	uint8_t		nCols;
	uint8_t		nRows;
	uint8_t		nBytes;
	uint8_t     PixelRow;
	uint8_t     Mask;
	uint16_t    Word0;
	uint16_t    Word1;
	uint16_t	charOffset;
	PGM_P		pFont;

	// get pointer to the beginning of the font table
	if (font == BOLD) pFont = (PGM_P) &FONT8x16;
	else pFont = (PGM_P) &FONT8x8;

	// get the nColumns, nRows and nBytes
	nCols  = pgm_read_byte( pFont );	// number of columns in FONT. The fonts are stored in PRGMEM
	nRows  = pgm_read_byte( pFont + 1); // number of rows in FONT
	nBytes = pgm_read_byte( pFont + 2); // number of bytes in FONT

	// get offset of the first byte of the desired character
	charOffset = (nBytes * ((uint16_t)c - 0x1F)) - 1; //+ nBytes - 1;

	// Row address set (command 0x2B)
	spi_LCD_command(PASET);
	spi_LCD_data(x);
	spi_LCD_data(x + nRows - 1);

	// Column address set (command 0x2A)
	spi_LCD_command(CASET);
	spi_LCD_data(y);
	spi_LCD_data(y + nCols - 1);

	// WRITE MEMORY
	spi_LCD_command(RAMWR);
	// loop on each row, working backwards from the bottom to the top
	for (i = nRows - 1; i >= 0; i--)
	{
		// copy a pixel row from font table and then increment row
		PixelRow = pgm_read_byte( pFont + charOffset++); //  The fonts are stored in PRGMEM

		// loop on each pixel in the row (left to right)
		// Note: we do two pixels each loop
		Mask = 0x80;
		for (j = 0; j < nCols; j += 2)
		{
			// if pixel bit set, use foreground color; else use the background color
			// now get the pixel color for two successive pixels
			if ((PixelRow & Mask) == 0) Word0 = bColor;
			else Word0 = fColor;

			Mask = Mask >> 1;

			if ((PixelRow & Mask) == 0) Word1 = bColor;
			else  Word1 = fColor;

			Mask = Mask >> 1;

			// use this information to output three data bytes
			spi_LCD_data((Word0 >> 4) & 0xFF);
			spi_LCD_data(((Word0 & 0xF) << 4) | ((Word1 >> 8) & 0xF));
			spi_LCD_data(Word1 & 0xFF);
		}
	}

	// terminate the Write Memory command
	spi_LCD_command(NOP);
}

//*******************************************************
//				   Font Definitions
//
//           The fonts are stored in PRGMEM
//*******************************************************



const uint8_t FONT8x8[] PROGMEM = {

0x08,0x08,0x08,0x00,0x00,0x00,0x00,0x00,        //  columns, rows, num_bytes_per_char
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,        //      space   0x20
0x30,0x78,0x78,0x30,0x30,0x00,0x30,0x00,        //      !
0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,        //      "
0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00,        //      #
0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00,        //      $
0x00,0x63,0x66,0x0C,0x18,0x33,0x63,0x00,        //      %
0x1C,0x36,0x1C,0x3B,0x6E,0x66,0x3B,0x00,        //      &
0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00,        //      '
0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00,        //      (
0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00,        //      )
0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,        //      *
0x00,0x30,0x30,0xFC,0x30,0x30,0x00,0x00,        //      +
0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30,        //      ,
0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,        //      -
0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,        //      .
0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00,        //      / (forward slash)
0x3E,0x63,0x63,0x6B,0x63,0x63,0x3E,0x00,        //      0               0x30
0x18,0x38,0x58,0x18,0x18,0x18,0x7E,0x00,        //      1
0x3C,0x66,0x06,0x1C,0x30,0x66,0x7E,0x00,        //      2
0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00,        //      3
0x0E,0x1E,0x36,0x66,0x7F,0x06,0x0F,0x00,        //      4
0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00,        //      5
0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00,        //      6
0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00,        //      7
0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00,        //      8
0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00,        //      9
0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00,        //      :
0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30,        //      ;
0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00,        //      <
0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00,        //      =
0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00,        //      >
0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00,        //      ?
0x3E,0x63,0x6F,0x69,0x6F,0x60,0x3E,0x00,        //      @               0x40
0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00,        //      A
0x7E,0x33,0x33,0x3E,0x33,0x33,0x7E,0x00,        //      B
0x1E,0x33,0x60,0x60,0x60,0x33,0x1E,0x00,        //      C
0x7C,0x36,0x33,0x33,0x33,0x36,0x7C,0x00,        //      D
0x7F,0x31,0x34,0x3C,0x34,0x31,0x7F,0x00,        //      E
0x7F,0x31,0x34,0x3C,0x34,0x30,0x78,0x00,        //      F
0x1E,0x33,0x60,0x60,0x67,0x33,0x1F,0x00,        //      G
0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00,        //      H
0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,        //      I
0x0F,0x06,0x06,0x06,0x66,0x66,0x3C,0x00,        //      J
0x73,0x33,0x36,0x3C,0x36,0x33,0x73,0x00,        //      K
0x78,0x30,0x30,0x30,0x31,0x33,0x7F,0x00,        //      L
0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00,        //      M
0x63,0x73,0x7B,0x6F,0x67,0x63,0x63,0x00,        //      N
0x3E,0x63,0x63,0x63,0x63,0x63,0x3E,0x00,        //      O
0x7E,0x33,0x33,0x3E,0x30,0x30,0x78,0x00,        //      P               0x50
0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00,        //      Q
0x7E,0x33,0x33,0x3E,0x36,0x33,0x73,0x00,        //      R
0x3C,0x66,0x30,0x18,0x0C,0x66,0x3C,0x00,        //      S
0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00,        //      T
0x66,0x66,0x66,0x66,0x66,0x66,0x7E,0x00,        //      U
0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,        //      V
0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00,        //      W
0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00,        //      X
0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00,        //      Y
0x7F,0x63,0x46,0x0C,0x19,0x33,0x7F,0x00,        //      Z
0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,        //      [
0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00,        //      \  (back slash)
0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,        //      ]
0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00,        //      ^
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,        //      _
0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00,        //      `               0x60
0x00,0x00,0x3C,0x06,0x3E,0x66,0x3B,0x00,        //      a
0x70,0x30,0x3E,0x33,0x33,0x33,0x6E,0x00,        //      b
0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00,        //      c
0x0E,0x06,0x3E,0x66,0x66,0x66,0x3B,0x00,        //      d
0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00,        //      e
0x1C,0x36,0x30,0x78,0x30,0x30,0x78,0x00,        //      f
0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x7C,        //      g
0x70,0x30,0x36,0x3B,0x33,0x33,0x73,0x00,        //      h
0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00,        //      i
0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3C,        //      j
0x70,0x30,0x33,0x36,0x3C,0x36,0x73,0x00,        //      k
0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,        //      l
0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00,        //      m
0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00,        //      n
0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00,        //      o
0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78,        //      p               0x70
0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F,        //      q
0x00,0x00,0x6E,0x3B,0x33,0x30,0x78,0x00,        //      r
0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00,        //      s
0x08,0x18,0x3E,0x18,0x18,0x1A,0x0C,0x00,        //      t
0x00,0x00,0x66,0x66,0x66,0x66,0x3B,0x00,        //      u
0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00,        //      v
0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00,        //      w
0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00,        //      x
0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x7C,        //      y
0x00,0x00,0x7E,0x4C,0x18,0x32,0x7E,0x00,        //      z
0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00,        //      {
0x0C,0x0C,0x0C,0x00,0x0C,0x0C,0x0C,0x00,        //      |
0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00,        //      }
0x3B,0x6E,0x00,0x00,0x00,0x00,0x00,0x00,        //      ~
0x1C,0x36,0x36,0x1C,0x00,0x00,0x00,0x00};       //      DEL


const uint8_t FONT8x16[] PROGMEM = {

0x08,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//  columns, rows, num_bytes_per_char
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//	space	0x20
0x00,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00,	//	!
0x00,0x63,0x63,0x63,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//	"
0x00,0x00,0x00,0x36,0x36,0x7F,0x36,0x36,0x36,0x7F,0x36,0x36,0x00,0x00,0x00,0x00,	//	#
0x0C,0x0C,0x3E,0x63,0x61,0x60,0x3E,0x03,0x03,0x43,0x63,0x3E,0x0C,0x0C,0x00,0x00,	//	$
0x00,0x00,0x00,0x00,0x00,0x61,0x63,0x06,0x0C,0x18,0x33,0x63,0x00,0x00,0x00,0x00,	//	%
0x00,0x00,0x00,0x1C,0x36,0x36,0x1C,0x3B,0x6E,0x66,0x66,0x3B,0x00,0x00,0x00,0x00,	//	&
0x00,0x30,0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//	'
0x00,0x00,0x0C,0x18,0x18,0x30,0x30,0x30,0x30,0x18,0x18,0x0C,0x00,0x00,0x00,0x00,	//	(
0x00,0x00,0x18,0x0C,0x0C,0x06,0x06,0x06,0x06,0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,	//	)
0x00,0x00,0x00,0x00,0x42,0x66,0x3C,0xFF,0x3C,0x66,0x42,0x00,0x00,0x00,0x00,0x00,	//	*
0x00,0x00,0x00,0x00,0x18,0x18,0x18,0xFF,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,	//	+
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x18,0x30,0x00,0x00,	//	,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//	-
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,	//	.
0x00,0x00,0x01,0x03,0x07,0x0E,0x1C,0x38,0x70,0xE0,0xC0,0x80,0x00,0x00,0x00,0x00,	//	/ (forward slash)
0x00,0x00,0x3E,0x63,0x63,0x63,0x6B,0x6B,0x63,0x63,0x63,0x3E,0x00,0x00,0x00,0x00,	//	0		0x30
0x00,0x00,0x0C,0x1C,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3F,0x00,0x00,0x00,0x00,	//	1
0x00,0x00,0x3E,0x63,0x03,0x06,0x0C,0x18,0x30,0x61,0x63,0x7F,0x00,0x00,0x00,0x00,	//	2
0x00,0x00,0x3E,0x63,0x03,0x03,0x1E,0x03,0x03,0x03,0x63,0x3E,0x00,0x00,0x00,0x00,	//	3
0x00,0x00,0x06,0x0E,0x1E,0x36,0x66,0x66,0x7F,0x06,0x06,0x0F,0x00,0x00,0x00,0x00,	//	4
0x00,0x00,0x7F,0x60,0x60,0x60,0x7E,0x03,0x03,0x63,0x73,0x3E,0x00,0x00,0x00,0x00,	//	5
0x00,0x00,0x1C,0x30,0x60,0x60,0x7E,0x63,0x63,0x63,0x63,0x3E,0x00,0x00,0x00,0x00,	//	6
0x00,0x00,0x7F,0x63,0x03,0x06,0x06,0x0C,0x0C,0x18,0x18,0x18,0x00,0x00,0x00,0x00,	//	7
0x00,0x00,0x3E,0x63,0x63,0x63,0x3E,0x63,0x63,0x63,0x63,0x3E,0x00,0x00,0x00,0x00,	//	8
0x00,0x00,0x3E,0x63,0x63,0x63,0x63,0x3F,0x03,0x03,0x06,0x3C,0x00,0x00,0x00,0x00,	//	9
0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,	//	:
0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x18,0x30,0x00,0x00,	//	;
0x00,0x00,0x00,0x06,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x06,0x00,0x00,0x00,0x00,	//	<
0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,	//	=
0x00,0x00,0x00,0x60,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x60,0x00,0x00,0x00,0x00,	//	>
0x00,0x00,0x3E,0x63,0x63,0x06,0x0C,0x0C,0x0C,0x00,0x0C,0x0C,0x00,0x00,0x00,0x00,	//	?
0x00,0x00,0x3E,0x63,0x63,0x6F,0x6B,0x6B,0x6E,0x60,0x60,0x3E,0x00,0x00,0x00,0x00,	//	@		0x40
0x00,0x00,0x08,0x1C,0x36,0x63,0x63,0x63,0x7F,0x63,0x63,0x63,0x00,0x00,0x00,0x00,	//	A
0x00,0x00,0x7E,0x33,0x33,0x33,0x3E,0x33,0x33,0x33,0x33,0x7E,0x00,0x00,0x00,0x00,	//	B
0x00,0x00,0x1E,0x33,0x61,0x60,0x60,0x60,0x60,0x61,0x33,0x1E,0x00,0x00,0x00,0x00,	//	C
0x00,0x00,0x7C,0x36,0x33,0x33,0x33,0x33,0x33,0x33,0x36,0x7C,0x00,0x00,0x00,0x00,	//	D
0x00,0x00,0x7F,0x33,0x31,0x34,0x3C,0x34,0x30,0x31,0x33,0x7F,0x00,0x00,0x00,0x00,	//	E
0x00,0x00,0x7F,0x33,0x31,0x34,0x3C,0x34,0x30,0x30,0x30,0x78,0x00,0x00,0x00,0x00,	//	F
0x00,0x00,0x1E,0x33,0x61,0x60,0x60,0x6F,0x63,0x63,0x37,0x1D,0x00,0x00,0x00,0x00,	//	G
0x00,0x00,0x63,0x63,0x63,0x63,0x7F,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,0x00,	//	H
0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,	//	I
0x00,0x00,0x0F,0x06,0x06,0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,	//	J
0x00,0x00,0x73,0x33,0x36,0x36,0x3C,0x36,0x36,0x33,0x33,0x73,0x00,0x00,0x00,0x00,	//	K
0x00,0x00,0x78,0x30,0x30,0x30,0x30,0x30,0x30,0x31,0x33,0x7F,0x00,0x00,0x00,0x00,	//	L
0x00,0x00,0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x63,0x63,0x63,0x00,0x00,0x00,0x00,	//	M
0x00,0x00,0x63,0x63,0x73,0x7B,0x7F,0x6F,0x67,0x63,0x63,0x63,0x00,0x00,0x00,0x00,	//	N
0x00,0x00,0x1C,0x36,0x63,0x63,0x63,0x63,0x63,0x63,0x36,0x1C,0x00,0x00,0x00,0x00,	//	O
0x00,0x00,0x7E,0x33,0x33,0x33,0x3E,0x30,0x30,0x30,0x30,0x78,0x00,0x00,0x00,0x00,	//	P		0x50
0x00,0x00,0x3E,0x63,0x63,0x63,0x63,0x63,0x63,0x6B,0x6F,0x3E,0x06,0x07,0x00,0x00,	//	Q
0x00,0x00,0x7E,0x33,0x33,0x33,0x3E,0x36,0x36,0x33,0x33,0x73,0x00,0x00,0x00,0x00,	//	R
0x00,0x00,0x3E,0x63,0x63,0x30,0x1C,0x06,0x03,0x63,0x63,0x3E,0x00,0x00,0x00,0x00,	//	S
0x00,0x00,0xFF,0xDB,0x99,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,	//	T
0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x3E,0x00,0x00,0x00,0x00,	//	U
0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x36,0x1C,0x08,0x00,0x00,0x00,0x00,	//	V
0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x6B,0x6B,0x7F,0x36,0x36,0x00,0x00,0x00,0x00,	//	W
0x00,0x00,0xC3,0xC3,0x66,0x3C,0x18,0x18,0x3C,0x66,0xC3,0xC3,0x00,0x00,0x00,0x00,	//	X
0x00,0x00,0xC3,0xC3,0xC3,0x66,0x3C,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00,	//	Y
0x00,0x00,0x7F,0x63,0x43,0x06,0x0C,0x18,0x30,0x61,0x63,0x7F,0x00,0x00,0x00,0x00,	//	Z
0x00,0x00,0x3C,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3C,0x00,0x00,0x00,0x00,	//	[
0x00,0x00,0x80,0xC0,0xE0,0x70,0x38,0x1C,0x0E,0x07,0x03,0x01,0x00,0x00,0x00,0x00,	//	\  (back slash)
0x00,0x00,0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00,0x00,0x00,0x00,	//	]
0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//	^
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,	//	_
0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//	`		0x60
0x00,0x00,0x00,0x00,0x00,0x3C,0x46,0x06,0x3E,0x66,0x66,0x3B,0x00,0x00,0x00,0x00,	//	a
0x00,0x00,0x70,0x30,0x30,0x3C,0x36,0x33,0x33,0x33,0x33,0x6E,0x00,0x00,0x00,0x00,	//	b
0x00,0x00,0x00,0x00,0x00,0x3E,0x63,0x60,0x60,0x60,0x63,0x3E,0x00,0x00,0x00,0x00,	//	c
0x00,0x00,0x0E,0x06,0x06,0x1E,0x36,0x66,0x66,0x66,0x66,0x3B,0x00,0x00,0x00,0x00,	//	d
0x00,0x00,0x00,0x00,0x00,0x3E,0x63,0x63,0x7E,0x60,0x63,0x3E,0x00,0x00,0x00,0x00,	//	e
0x00,0x00,0x1C,0x36,0x32,0x30,0x7C,0x30,0x30,0x30,0x30,0x78,0x00,0x00,0x00,0x00,	//	f
0x00,0x00,0x00,0x00,0x00,0x3B,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00,0x00,	//	g
0x00,0x00,0x70,0x30,0x30,0x36,0x3B,0x33,0x33,0x33,0x33,0x73,0x00,0x00,0x00,0x00,	//	h
0x00,0x00,0x0C,0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00,0x00,0x00,0x00,	//	i
0x00,0x00,0x06,0x06,0x00,0x0E,0x06,0x06,0x06,0x06,0x06,0x66,0x66,0x3C,0x00,0x00,	//	j
0x00,0x00,0x70,0x30,0x30,0x33,0x33,0x36,0x3C,0x36,0x33,0x73,0x00,0x00,0x00,0x00,	//	k
0x00,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00,0x00,0x00,0x00,	//	l
0x00,0x00,0x00,0x00,0x00,0x6E,0x7F,0x6B,0x6B,0x6B,0x6B,0x6B,0x00,0x00,0x00,0x00,	//	m
0x00,0x00,0x00,0x00,0x00,0x6E,0x33,0x33,0x33,0x33,0x33,0x33,0x00,0x00,0x00,0x00,	//	n
0x00,0x00,0x00,0x00,0x00,0x3E,0x63,0x63,0x63,0x63,0x63,0x3E,0x00,0x00,0x00,0x00,	//	o
0x00,0x00,0x00,0x00,0x00,0x6E,0x33,0x33,0x33,0x33,0x3E,0x30,0x30,0x78,0x00,0x00,	//	p		0x70
0x00,0x00,0x00,0x00,0x00,0x3B,0x66,0x66,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00,0x00,	//	q
0x00,0x00,0x00,0x00,0x00,0x6E,0x3B,0x33,0x30,0x30,0x30,0x78,0x00,0x00,0x00,0x00,	//	r
0x00,0x00,0x00,0x00,0x00,0x3E,0x63,0x38,0x0E,0x03,0x63,0x3E,0x00,0x00,0x00,0x00,	//	s
0x00,0x00,0x08,0x18,0x18,0x7E,0x18,0x18,0x18,0x18,0x1B,0x0E,0x00,0x00,0x00,0x00,	//	t
0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3B,0x00,0x00,0x00,0x00,	//	u
0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x36,0x36,0x1C,0x1C,0x08,0x00,0x00,0x00,0x00,	//	v
0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x63,0x6B,0x6B,0x7F,0x36,0x00,0x00,0x00,0x00,	//	w
0x00,0x00,0x00,0x00,0x00,0x63,0x36,0x1C,0x1C,0x1C,0x36,0x63,0x00,0x00,0x00,0x00,	//	x
0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x63,0x63,0x63,0x3F,0x03,0x06,0x3C,0x00,0x00,	//	y
0x00,0x00,0x00,0x00,0x00,0x7F,0x66,0x0C,0x18,0x30,0x63,0x7F,0x00,0x00,0x00,0x00,	//	z
0x00,0x00,0x0E,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00,	//	{
0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,	//	|
0x00,0x00,0x70,0x18,0x18,0x18,0x0E,0x18,0x18,0x18,0x18,0x70,0x00,0x00,0x00,0x00,	//	}
0x00,0x00,0x3B,0x6E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	//	~
0x00,0x70,0xD8,0xD8,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};	//	DEL
