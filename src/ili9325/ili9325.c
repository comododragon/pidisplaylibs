/* ********************************************************************************************* */
/* * Display Driver for ili9325-driven TFTs                                                    * */
/* * Author: André Bannwart Perina                                                             * */
/* *         with snippets adapted from Rinky-dink's UTFT                                      * */
/* *         (http://www.rinkydinkelectronics.com/library.php?id=52) and Sprite_tm's Raspberry * */
/* *         micro arcade machine (http://spritesmods.com/?art=rpi_arcade&page=2)              * */
/* ********************************************************************************************* */
/* * Copyright (c) 2017 André B. Perina                                                        * */
/* * UTFT: Copyright (c) 2015 Rinky-Dink Electronics, Henning Karlsen. All Rights Reserved.    * */
/* * Raspberry micro arcade machine: Copyright (c) 2012 Jeroen Domburg                         * */
/* *                                 Copyright (c) 2009 Jean-Christian de Rivaz                * */
/* *                                 Copyright (c) 2010-2011 Freescale Semiconductor, Inc.     * */
/* *                                 All Rights Reserved.                                      * */
/* *                                                                                           * */
/* * This file is part of PiDisplayLibs                                                        * */
/* *                                                                                           * */
/* * PiDisplayLibs is free software: you can redistribute it and/or modify it under the terms  * */
/* * of the GNU General Public License as published by the Free Software Foundation, either    * */
/* * version 3 of the License, or (at your option) any later version.                          * */
/* *                                                                                           * */
/* * PiDisplayLibs is distributed in the hope that it will be useful, but WITHOUT ANY          * */
/* * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A           * */
/* * PARTICULAR PURPOSE.  See the GNU General Public License for more details.                 * */
/* *                                                                                           * */
/* * You should have received a copy of the GNU General Public License along with Foobar.  If  * */
/* * not, see <http://www.gnu.org/licenses/>.                                                  * */
/* ********************************************************************************************* */

#include "display.h"

#include <unistd.h>

#include "bcmgpio.h"
#include "common.h"

/* Screen resolution macros */
#define DISPLAY_XRES 320
#define DISPLAY_YRES 240

/**
 * Used pins for this display on Raspberry Pi 3 Model B. Please note that these are the BCM2835 GPIO's numbering, not the
 * actual pins from Raspberry Pi. Refer to https://www.element14.com/community/servlet/JiveServlet/previewBody/73950-102-9-339300/pi3_gpio.png
 * for equivalence between BCM2835 pins (NAME) and actual header pins (Pin#).
 */
#define RS_PIN 2
#define RW_PIN 3
#define RD_PIN 4
#define DB_PIN0 17
#define DB_PIN1 27
#define DB_PIN2 22
#define DB_PIN3 10
#define DB_PIN4 9
#define DB_PIN5 11
#define DB_PIN6 5
#define DB_PIN7 6
#define DB_PINMASK ((1 << DB_PIN7) | (1 << DB_PIN6) | (1 << DB_PIN5) | (1 << DB_PIN4) | (1 << DB_PIN3) | (1 << DB_PIN2) | (1 << DB_PIN1) | (1 << DB_PIN0))
#define CS_PIN 13
#define RST_PIN 19

/**
 * @brief Scramble DB bits so that they can be written simultaneously using bcmgpio_write_mask.
 *        Since negative shift is undefined in C, all these preprocessor IF's are needed to invert shift directions.
 * @param val Unscrambled DB value.
 * @return Scrambled DB value that may be used by bcmgpio_write_mask along with the DB_PINMASK macro.
 */
inline unsigned int scrambleDB(unsigned int val) {
	return
#if DB_PIN0 != 0
		((val & 0x1) << DB_PIN0) |
#else
		(val & 0x1) |
#endif
#if DB_PIN1 > 1
		((val & 0x2) << (DB_PIN1 - 1)) |
#elif DB_PIN1 < 1
		((val & 0x2) >> (1 - DB_PIN1)) |
#else
		(val & 0x2) |
#endif
#if DB_PIN2 > 2
		((val & 0x4) << (DB_PIN2 - 2)) |
#elif DB_PIN2 < 2
		((val & 0x4) >> (2 - DB_PIN2)) |
#else
		(val & 0x4) |
#endif
#if DB_PIN3 > 3
		((val & 0x8) << (DB_PIN3 - 3)) |
#elif DB_PIN3 < 3
		((val & 0x8) >> (3 - DB_PIN3)) |
#else
		(val & 0x8) |
#endif
#if DB_PIN4 > 4
		((val & 0x10) << (DB_PIN4 - 4)) |
#elif DB_PIN4 < 4
		((val & 0x10) >> (4 - DB_PIN4)) |
#else
		(val & 0x10) |
#endif
#if DB_PIN5 > 5
		((val & 0x20) << (DB_PIN5 - 5)) |
#elif DB_PIN5 < 5
		((val & 0x20) >> (5 - DB_PIN5)) |
#else
		(val & 0x20) |
#endif
#if DB_PIN6 > 6
		((val & 0x40) << (DB_PIN6 - 6)) |
#elif DB_PIN6 < 6
		((val & 0x40) >> (6 - DB_PIN6)) |
#else
		(val & 0x40) |
#endif
#if DB_PIN7 > 7
		((val & 0x80) << (DB_PIN7 - 7));
#elif DB_PIN7 < 7
		((val & 0x80) >> (7 - DB_PIN7));
#else
		(val & 0x80);
#endif
}

/**
 * @brief Select a register for write/read.
 * @param vl Register.
 */
void _write_com(char vl) {
	unsigned int vlScrambled = scrambleDB((unsigned int) vl);

	bcmgpio_write_uns(RS_PIN, 0);

	/* Write 8 MSBs. Since there are less than 256 registers, DB doesn't need to be zeroed in this step */
	//bcmgpio_write_mask_uns(DB_PINMASK, 0);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 1);
	/* Write 8 LSBs */
	bcmgpio_write_mask_uns(DB_PINMASK, vlScrambled);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 1);
}

/**
 * @brief Write 16-bit data to the selected register.
 * @param vh Value MSBs.
 * @param vl Value LSBs.
 */
void _write_data(char vh, char vl) {
	unsigned int vhScrambled = scrambleDB((unsigned int) vh);
	unsigned int vlScrambled = scrambleDB((unsigned int) vl);

	bcmgpio_write_uns(RS_PIN, 1);

	/* Write 8 MSBs */
	bcmgpio_write_mask_uns(DB_PINMASK, vhScrambled);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 1);

	/* Write 8 LSBs */
	bcmgpio_write_mask_uns(DB_PINMASK, vlScrambled);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 0);
	bcmgpio_write_uns(RW_PIN, 1);
}

/**
 * @brief Select a register and write 16-bit data.
 * @param com Register.
 * @param data Value.
 */
void _write_comdata(char com, int data) {
	_write_com(com);
	_write_data(data >> 8, data);
}

/**
 * @brief Initialise display.
 * @param args Pointer to arguments. See specific notes for each driver.
 * @param argc Number of elements in args. See specific notes for each driver.
 * @return Return code. See specific notes for each driver.
 *
 * @note For the ili9325 driver, there are no arguments needed. Simply use as display_init(NULL, 0);
 *       Possible return codes:
 *           DISPLAY_OK: No errors occurred.
 *           DISPLAY_GPIO_ERROR: An error occurred while initialising bcmgpio. The specific error code is
 *                               masked on the first 2 bytes of the return value.
 */
int display_init(void *args, int argc) {
	int rv = DISPLAY_OK;
	int irv;

	/* Initialise bcmgpio */
	irv = bcmgpio_init();
	ASSERT(irv == BCMGPIO_OK, rv = DISPLAY_GPIO_ERROR | irv);

	/* Set outputs */
	bcmgpio_set_direction(RS_PIN, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(RW_PIN, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(RD_PIN, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN0, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN1, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN2, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN3, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN4, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN5, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN6, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(DB_PIN7, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(CS_PIN, BCMGPIO_DIR_OUT);
	bcmgpio_set_direction(RST_PIN, BCMGPIO_DIR_OUT);

	/* Reset display */
	bcmgpio_write_uns(RST_PIN, 1);
	usleep(5000);
	bcmgpio_write_uns(RST_PIN, 0);
	usleep(15000);
	bcmgpio_write_uns(RST_PIN, 1);
	usleep(15000);

	/* Select device. Since PiDisplayLibs expects a non-shared bus, this device is kept as enabled */
	bcmgpio_write_uns(CS_PIN, 0);

	/* Set internal timing */
	_write_comdata(0x00E3, 0x3008);
	_write_comdata(0x00E7, 0x0012);
	_write_comdata(0x00EF, 0x1231);
	usleep(20000);
	/* Set SS and SM bits */
	_write_comdata(0x0001, 0x0100);
	usleep(20000);
	/* Set 1 line inversion */
	_write_comdata(0x0002, 0x0700);
	/* Set GRAM write direction and BGR = 1 */
	_write_comdata(0x0003, 0x1018);
	usleep(10000);
	/* Resize register */
	_write_comdata(0x0004, 0x0000);
	usleep(10000);
	/* Set the back and front porch */
	_write_comdata(0x0008, 0x0207);
	usleep(10000);
	/* Set non-display area refresh cycle ISC[3:0] */
	_write_comdata(0x0009, 0x0000);
	usleep(10000);
	/* FMARK function */
	_write_comdata(0x000A, 0x0000);
	usleep(10000);
	/* RGB interface setting */
	_write_comdata(0x000C, 0x0000);
	usleep(10000);
	/* Frame marker position */
	_write_comdata(0x000D, 0x0000);
	usleep(10000);
	/* RGB interface polarity */
	_write_comdata(0x000F, 0x0000);

	/* POWER ON SEQUENCE */
	usleep(10000);
	_write_comdata(0x0010, 0x0000);
	usleep(10000);
	_write_comdata(0x0011, 0x0007);
	usleep(10000);
	/* VERG1OUT voltage */
	_write_comdata(0x0012, 0x0000);
	usleep(10000);
	/* VCOM amplitude */
	_write_comdata(0x0013, 0x0000);
	/* Discharge capacitor power voltage */
	usleep(40000);
	_write_comdata(0x0010, 0x1490);
	_write_comdata(0x0011, 0x0227);
	usleep(10000);
	/* External reference voltage Vci */
	_write_comdata(0x0012, 0x001c);
	usleep(10000);
	/* VCOM amplitude */
	_write_comdata(0x0013, 0x0A00);
	/* VCOMH */
	_write_comdata(0x0029, 0x000F);
	/* Frame rate 91Hz */
	_write_comdata(0x002B, 0x000D);
	usleep(10000);
	/* GRAM horizontal address */
	_write_comdata(0x0020, 0x0000);
	/* GRAM vertical address */
	_write_comdata(0x0021, 0x0000);
	/* Adjust gamma curve */
	_write_comdata(0x0030, 0x0000);
	_write_comdata(0x0031, 0x0203);
	_write_comdata(0x0032, 0x0001);
	_write_comdata(0x0035, 0x0205);
	_write_comdata(0x0036, 0x030C);
	_write_comdata(0x0037, 0x0607);
	_write_comdata(0x0038, 0x0405);
	_write_comdata(0x0039, 0x0707);
	_write_comdata(0x003C, 0x0502);
	_write_comdata(0x003D, 0x1008);
	usleep(10000);
	/* Set GRAM area */
	/* Horizontal start address */
	_write_comdata(0x0050, 0x0000);
	/* Horizontal end address */
	_write_comdata(0x0051, 0x00EF);
	/* Vertical start address */
	_write_comdata(0x0052, 0x0000);
	/* Vertical end address */
	_write_comdata(0x0053, 0x013F);
	/* Gate scan line */
	_write_comdata(0x0060, 0xA700);
	_write_comdata(0x0061, 0x0001);
	/* Set scrolling line */
	_write_comdata(0x006A, 0x0000);
	usleep(50000);
	/* Partial display control */
	_write_comdata(0x0080, 0x0000);
	_write_comdata(0x0081, 0x0000);
	_write_comdata(0x0082, 0x0000);
	_write_comdata(0x0083, 0x0000);
	_write_comdata(0x0084, 0x0000);
	_write_comdata(0x0085, 0x0000);
	usleep(10000);
	/* Panel control */
	_write_comdata(0x0090, 0x0010);
	_write_comdata(0x0092, 0x0600);
	_write_comdata(0x0093, 0x0003);
	_write_comdata(0x0095, 0x0110);
	_write_comdata(0x0097, 0x0000);
	_write_comdata(0x0098, 0x0000);
	usleep(10000);
	/* 262k color, display ON */
	_write_comdata(0x0007, 0x0133);
	usleep(20000);
	/* Select register for memory write */
	_write_com(0x0022);

	//bcmgpio_write_uns(CS_PIN, 1);

_err:

	return rv;
}

/**
 * @brief Set (x,y) coordinate for start of drawing.
 * @param x First coordinate.
 * @param y Second coordinate.
 * @return Return code. See specific notes for each driver.
 *
 * @note Possible return codes:
 *           DISPLAY_OK: No error checking is performed.
 */
int display_set_xy(int x, int y) {
	//bcmgpio_write_uns(CS_PIN, 0);

	/* Horizontal GRAM start address */
	_write_comdata(0x0020, y);
	/* Vertical GRAM start address */
	_write_comdata(0x0021, (DISPLAY_XRES - 1) - x);
	/* Select register for memory write */
	_write_com(0x0022);

	//bcmgpio_write_uns(CS_PIN, 1);

	return DISPLAY_OK;
}

/**
 * @brief Draw a pixel on current memory position.
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return Return code. See specific notes for each driver.
 *
 * @note Possible return codes:
 *           DISPLAY_OK: No error checking is performed.
 */
int display_draw(unsigned char r, unsigned char g, unsigned char b) {
	/**
	 * Colour structure (16 bits):
	 * 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00
	 * R7 | R6 | R5 | R4 | R3 | G7 | G6 | G5 | G4 | G3 | G2 | B7 | B6 | B5 | B4 | B3
	 */
	int colour = ((r << 8) & 0xF800) | ((g << 3) & 0x7E0) | ((b >> 3) & 0x1F);

	//bcmgpio_write_uns(CS_PIN, 0);

	/* Write colour to current memory position (i.e. draw pixel) */
	_write_data((colour >> 8), (colour & 0xFF));

	//bcmgpio_write_uns(CS_PIN, 1);

	return DISPLAY_OK;
}

/**
 * @brief Close handles, free memory, finish use.
 * @return Return code. See specific notes for each driver.
 *
 * @note Possible return codes:
 *           DISPLAY_OK: No error checking is performed.
 */
int display_finish(void) {
	bcmgpio_finish();

	return DISPLAY_OK;
}
