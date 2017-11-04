/* ********************************************************************************************* */
/* * Display Driver Generic Header                                                             * */
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

#ifndef DISPLAY_H
#define DISPLAY_H

/* Return codes */
#define DISPLAY_OK 0x0
#define DISPLAY_GPIO_ERROR 0x10000

#ifdef DISPLAY_NOFUNCS

/**
 * @brief Initialise display.
 * @param args Pointer to arguments. See specific notes for each driver.
 * @param argc Number of elements in args. See specific notes for each driver.
 * @return Return code. See specific notes for each driver.
 */
int display_init(void *args, int argc);

/**
 * @brief Set (x,y) coordinate for start of drawing.
 * @param x First coordinate.
 * @param y Second coordinate.
 * @return Return code. See specific notes for each driver.
 */
int display_set_xy(int x, int y);

/**
 * @brief Draw a pixel on current memory position.
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return Return code. See specific notes for each driver.
 */
int display_draw(unsigned char r, unsigned char g, unsigned char b);

/**
 * @brief Close handles, free memory, finish use.
 * @return Return code. See specific notes for each driver.
 */
int display_finish(void);

#endif

#endif
