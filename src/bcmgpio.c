/* ********************************************************************************************* */
/* * BCMGPIO Library for fast GPIO handling in BCM2835 devices                                 * */
/* * Author: André Bannwart Perina                                                             * */
/* ********************************************************************************************* */
/* * Copyright (c) 2017 André B. Perina                                                        * */
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

#include "bcmgpio.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"

/* Peripheral base address for Raspberry Pi 3 Model B */
#define PERI_BASE 0x3F000000
/* GPIO base address */
#define GPIO_BASE (PERI_BASE + 0x200000)
/* Sizes for mmap() */
#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

/* Offsets for managing GPIOs */
#define GPIO_SET_OFFSET 0x7
#define GPIO_CLEAR_OFFSET 0xA
#define GPIO_READ_OFFSET 0xD

/* Global GPIO handler */
static volatile unsigned *gpio = NULL;

/**
 * @brief Initialise library.
 */
int bcmgpio_init(void) {
	int rv = BCMGPIO_OK;
	int memFd = -1;
	void *gpioMap = NULL;

	ASSERT(gpio == NULL, rv = BCMGPIO_ALREADY_INIT);

	memFd = open("/dev/mem", O_RDWR | O_SYNC);
	ASSERT(memFd > 0, rv = BCMGPIO_DEV_INACCESSIBLE);

	gpioMap = mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memFd, GPIO_BASE);
	ASSERT(gpioMap != MAP_FAILED, rv = (BCMGPIO_MMAP_ERROR | (int) gpioMap));

	gpio = (volatile unsigned *) gpioMap;

_err:
	if(memFd != -1)
		close(memFd);

	return rv;
}

/**
 * @brief Set pin direction.
 */
int bcmgpio_set_direction(unsigned int pin, unsigned int direction) {
	int rv = BCMGPIO_OK;

	ASSERT((BCMGPIO_DIR_IN == direction) || (BCMGPIO_DIR_OUT == direction), rv = BCMGPIO_INVALID_ARGS);
	ASSERT(gpio != NULL, rv = BCMGPIO_NOT_INIT);

	*(gpio + (pin / 10)) &= ~(7 << ((pin % 10) * 3));

	if(BCMGPIO_DIR_OUT == direction)
		*(gpio + (pin / 10)) |=  (1 << ((pin % 10) * 3));

_err:

	return rv;
}

/**
 * @brief Write bit to a pin.
 */
int bcmgpio_write(unsigned int pin, unsigned char value) {
	int rv = BCMGPIO_OK;

	ASSERT(gpio != NULL, rv = BCMGPIO_NOT_INIT);

	if(value)
		*(gpio + GPIO_SET_OFFSET) = 1 << pin;
	else
		*(gpio + GPIO_CLEAR_OFFSET) = 1 << pin;

_err:

	return rv;
}

/**
 * @brief Write bits to the first 32 pins. The control to which bits modify and preserve is performed by the pin mask.
 *        e.g. if pinMask = 0x0FF0 and value = 0xF5AF:
 *        GPIO PIN | Behaviour
 *          31..12 | Nothing
 *              11 | Set 0
 *              10 | Set 1
 *              09 | Set 0
 *              08 | Set 1
 *              07 | Set 1
 *              06 | Set 0
 *              05 | Set 1
 *              04 | Set 0
 *          03..00 | Nothing
 */
int bcmgpio_write_mask(unsigned int pinMask, unsigned int value) {
	int rv = BCMGPIO_OK;

	ASSERT(gpio != NULL, rv = BCMGPIO_NOT_INIT);

	*(gpio + GPIO_SET_OFFSET) = pinMask & value;
	*(gpio + GPIO_CLEAR_OFFSET) = pinMask & ~value;

_err:

	return rv;
}

/**
 * @brief Write bit to a pin. Its behaviour is similar to bcmgpio_write(), but there are no error checks.
 *
 * @note Since there are no error checks, make sure bcmgpio_init() was executed before with success!
 */
inline void bcmgpio_write_uns(unsigned int pin, unsigned char value) {
	*(gpio + (value? GPIO_SET_OFFSET : GPIO_CLEAR_OFFSET)) = 1 << pin;
}

/**
 * @brief Write bits to the first 32 pins. Its behaviour is similar to bcmgpio_write_mask(), but there are no error checks.
 *
 * @note Since there are no error checks, make sure bcmgpio_init() was executed before with success!
 */
inline void bcmgpio_write_mask_uns(unsigned int pinMask, unsigned int value) {
	*(gpio + GPIO_SET_OFFSET) = pinMask & value;
	*(gpio + GPIO_CLEAR_OFFSET) = pinMask & ~value;
}

/**
 * @brief Read bit from a pin.
 */
unsigned char bcmgpio_read(unsigned int pin) {
	return *(gpio + GPIO_READ_OFFSET) & (1 << pin);
}

/**
 * @brief Read bits from the first 32 pins where pinMask is enabled.
 */
unsigned int bcmgpio_read_mask(unsigned int pinMask) {
	return *(gpio + GPIO_READ_OFFSET) & pinMask;
}

/**
 * @brief Free stuff and finish library.
 */
int bcmgpio_finish(void) {
	int rv = BCMGPIO_OK;

	ASSERT(gpio != NULL, rv = BCMGPIO_NOT_INIT);

	munmap((void *) gpio, BLOCK_SIZE);
	gpio = NULL;

_err:

	return rv;
}
