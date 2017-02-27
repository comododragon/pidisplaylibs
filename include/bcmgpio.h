/* ********************************************************************************************* */
/* * BCMGPIO Header for fast GPIO handling in BCM2835 devices                                  * */
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

#ifndef BCMGPIO
#define BCMGPIO

/* Return codes */
#define BCMGPIO_OK 0x0
#define BCMGPIO_ALREADY_INIT 0x100
#define BCMGPIO_NOT_INIT 0x200
#define BCMGPIO_DEV_INACCESSIBLE 0x300
#define BCMGPIO_MMAP_ERROR 0x400
#define BCMGPIO_INVALID_ARGS 0x500

/* Pin direction macros */
#define BCMGPIO_DIR_IN 0
#define BCMGPIO_DIR_OUT 1

/**
 * @brief Initialise library.
 * @return One of the following error codes:
 *         BCMGPIO_OK: No errors occurred.
 *         BCMGPIO_ALREADY_INIT: Library was already initialised.
 *         BCMGPIO_DEV_INACCESSIBLE: Open of "/dev/mem" failed (not enough permissions?).
 *         BCMGPIO_MMAP_ERROR: mmap() error. Its error code is masked on the first 2 bytes.
 */
int bcmgpio_init(void);

/**
 * @brief Set pin direction.
 * @param pin Pin number.
 * @param direction BCMGPIO_DIR_IN for input, BCMGPIO_DIR_OUT for output.
 * @return One of the following error codes:
 *         BCMGPIO_OK: No errors occurred.
 *         BCMGPIO_INVALID_ARGS: Invalid argument for direction.
 *         BCMGPIO_NOT_INIT: Library was not initialised. Run bcmgpio_init();
 */
int bcmgpio_set_direction(unsigned int pin, unsigned int direction);

/**
 * @brief Write bit to a pin.
 * @param pin Pin number.
 * @param value Bit to be written.
 * @return One of the following error codes:
 *         BCMGPIO_OK: No errors occurred.
 *         BCMGPIO_NOT_INIT: Library was not initialised. Run bcmgpio_init();
 */
int bcmgpio_write(unsigned int pin, unsigned char value);

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
 * @param pinMask Pin mask. If the i-th position is 1, the i-th GPIO pin will be set/reset according to the i-th bit of value.
 * @param value 32-bit value. The i-th bit will define the value of the i-th GPIO pin according to the mask.
 * @return One of the following error codes:
 *         BCMGPIO_OK: No errors occurred.
 *         BCMGPIO_NOT_INIT: Library was not initialised. Run bcmgpio_init();
 */
int bcmgpio_write_mask(unsigned int pinMask, unsigned int value);

/**
 * @brief Write bit to a pin. Its behaviour is similar to bcmgpio_write(), but there are no error checks.
 * @param pin Pin number.
 * @param value Bit to be written.
 *
 * @note Since there are no error checks, make sure bcmgpio_init() was executed before with success!
 */
inline void bcmgpio_write_uns(unsigned int pin, unsigned char value);

/**
 * @brief Write bits to the first 32 pins. Its behaviour is similar to bcmgpio_write_mask(), but there are no error checks.
 * @param pinMask Pin mask. If the i-th position is 1, the i-th GPIO pin will be set/reset according to the i-th bit of value.
 * @param value 32-bit value. The i-th bit will define the value of the i-th GPIO pin according to the mask.
 *
 * @note Since there are no error checks, make sure bcmgpio_init() was executed before with success!
 */
inline void bcmgpio_write_mask_uns(unsigned int pinMask, unsigned int value);

/**
 * @brief Read bit from a pin.
 * @param pin Pin number.
 * @return Bit value from the pin.
 */
unsigned char bcmgpio_read(unsigned int pin);

/**
 * @brief Read bits from the first 32 pins where pinMask is enabled.
 * @param pinMask Pin mask. Read description of bcmgpio_write_mask() for example.
 * @return 32-bit value of read pins.
 */
unsigned int bcmgpio_read_mask(unsigned int pinMask);

/**
 * @brief Free stuff and finish library.
 * @return One of the following error codes:
 *         BCMGPIO_OK: No errors occurred.
 *         BCMGPIO_NOT_INIT: Library was not initialised.
 */
int bcmgpio_finish(void);

#endif
