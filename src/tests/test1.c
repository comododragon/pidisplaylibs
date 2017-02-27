/* ********************************************************************************************* */
/* * Example 1 of PiDisplayLibs usage: Print colour gradients                                  * */
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

#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"

int main(int argc, char *argv[]) {
	char *driverLibPath = argv[1];
	void *driverLibrary = NULL;
	int (* display_init)(void *, int) = NULL;
	int (* display_set_xy)(unsigned int, unsigned int) = NULL;
	int (* display_draw)(unsigned char, unsigned char,  unsigned char) = NULL;
	int (* display_finish)(void) = NULL;
	int i, j;
	int colour = 0;

	/* Check if drivers .so file was informed */
	ASSERT(2 == argc, fprintf(stderr, "Error: Driver library path was not informed. Aborting.\n"));

	/* Attempt to load driver library */
	driverLibrary = dlopen(driverLibPath, RTLD_LAZY);
	ASSERT(driverLibrary != NULL, fprintf(stderr, "Error: dlopen(): %s\n", dlerror()));

	/* Retrieve display_init() */
	display_init = dlsym(driverLibrary, "display_init");
	ASSERT(display_init != NULL, fprintf(stderr, "Error: dlsym(\"display_init\"): %s\n", dlerror()));

	/* Retrieve display_set_xy() */
	display_set_xy = dlsym(driverLibrary, "display_set_xy");
	ASSERT(display_set_xy != NULL, fprintf(stderr, "Error: dlsym(\"display_set_xy\"): %s\n", dlerror()));

	/* Retrieve display_draw() */
	display_draw = dlsym(driverLibrary, "display_draw");
	ASSERT(display_draw != NULL, fprintf(stderr, "Error: dlsym(\"display_draw\"): %s\n", dlerror()));

	/* Retrieve display_finish() */
	display_finish = dlsym(driverLibrary, "display_finish");
	ASSERT(display_finish != NULL, fprintf(stderr, "Error: dlsym(\"display_finish\"): %s\n", dlerror()));

	/* Initialise display */
	display_init(NULL, 0);

	/* Set coordinate to (0,0) */
	display_set_xy(0, 0);

	/* Set screen to black */
	for(i = 0; i < 240; i++)
		for(j = 0; j < 320; j++)
			display_draw(0, 0, 0);

	/* Set coordinate to (0,0) */
	display_set_xy(0, 0);

	for(i = 0; i < 240; i++) {
		for(j = 0; j < 320; j++) {
			/* Draw gradients */
			if(j < 80)
				display_draw(0, 0, colour);
			else if(j < 160)
				display_draw(0, colour, 0);
			else if(j < 240)
				display_draw(colour, 0, 0);
			else
				display_draw(colour, 0, colour);
		}

		colour += 5;
	}

_err:

	if(display_finish)
		display_finish();

	if(driverLibrary)
		dlclose(driverLibrary);

	return 0;
}
