/* ********************************************************************************************* */
/* * Example 1 of PiDisplayLibs usage: Scroll text using TTF font                              * */
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
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "display.h"

int min(int a, int b) {
	return (a < b)? a : b;
}

int main(int argc, char *argv[]) {
	char *driverLibPath = NULL;
	char *fontPath = NULL;
	char *inputString = NULL;
	int inputStringSz = -1;
	unsigned int repeatAmt = 0;
	void *driverLibrary = NULL;
	int (* display_init)(void *, int) = NULL;
	int (* display_set_xy)(unsigned int, unsigned int) = NULL;
	int (* display_draw)(unsigned char, unsigned char,  unsigned char) = NULL;
	int (* display_finish)(void) = NULL;
	int retVal = DISPLAY_OK;
	FT_Error ftRet = FT_Err_Ok;
	FT_Library ftLibrary = NULL;
	FT_Face ftFontFace = NULL;
	FT_GlyphSlot slot;
	unsigned int *text[240];
	unsigned int textWidth = 0;
	int i, j, k, l;

	/* Initialise text matrix pointers */
	for(i = 0; i < 240; i++)
		text[i] = NULL;

	/* Check if drivers .so file, font file and string was informed */
	ASSERT(5 == argc, fprintf(stderr, "Usage: %s DRIVERSOFILE TTFFONTFILE STRING REPEATAMT\n", argv[0]));
	driverLibPath = argv[1];
	fontPath = argv[2];
	inputString = argv[3];
	inputStringSz = strnlen(inputString, 256);
	repeatAmt = atoi(argv[4]);

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
	ASSERT(DISPLAY_OK == retVal, fprintf(stderr, "Error: display_init() failed with code %d\n", retVal));

	/* Initialise freetype library */
	ftRet = FT_Init_FreeType(&ftLibrary);
	ASSERT(FT_Err_Ok == ftRet, fprintf(stderr, "Error: FT_Init_FreeType() failed with code %d\n", ftRet));

	/* Load font */
	ftRet = FT_New_Face(ftLibrary, fontPath, 0, &ftFontFace);
	slot = ftFontFace->glyph;
	ASSERT(FT_Err_Ok == ftRet, fprintf(stderr, "Error: FT_New_Face() failed with code %d\n", ftRet));

	/* Set font size */
	ftRet = FT_Set_Pixel_Sizes(ftFontFace, 0, 300);
	ASSERT(FT_Err_Ok == ftRet, fprintf(stderr, "Error: FT_Set_Pixel_Sizes() failed with code %d\n", ftRet));

	/* Allocate 320 columns on text matrix for black padding */
	for(i = 0; i < 240; i++)
		text[i] = calloc(320, sizeof(unsigned int));
	textWidth = 320;

	/* Renderise each character from string */
	for(i = 0; i < inputStringSz; i++) {
		/* Load character */
		ftRet = FT_Load_Glyph(ftFontFace, FT_Get_Char_Index(ftFontFace, inputString[i]), FT_LOAD_DEFAULT);
		ASSERT(FT_Err_Ok == ftRet, fprintf(stderr, "Error: FT_Load_Glyph() failed with code %d\n", ftRet));

		/* Render to bitmap if needed */
		if(ftFontFace->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			ftRet = FT_Render_Glyph(ftFontFace->glyph, FT_RENDER_MODE_NORMAL);
			ASSERT(FT_Err_Ok == ftRet, fprintf(stderr, "Error: FT_Render_Glyph() failed with code %d\n", ftRet));
		}

		for(j = 239, k = (slot->bitmap.rows - 1); j >= 0; j--, k--) {
			/* Allocate columns for character */
			text[j] = realloc(text[j], (textWidth + slot->bitmap.width + 50) * sizeof(unsigned int));
			for(l = 0; l < slot->bitmap.width; l++) {
				/* Draw character */
				if(k >= 0)
					text[j][l + textWidth] = slot->bitmap.buffer[k * slot->bitmap.width + l];
				/* Character is done, draw black space */
				else
					text[j][l + textWidth] = 0;
			}
			/* Add padding */
			for(l = 0; l < 50; l++)
				text[j][l + textWidth + slot->bitmap.width] = 0;
		}
		textWidth += slot->bitmap.width + 50;
	}

	/* Set coordinate to (0,0) */
	display_set_xy(0, 0);

	for(i = 0; i < (repeatAmt * textWidth); i += 4) {
		for(j = 0; j < 240; j++) {
			for(k = 0; k < 320; k++) {
				/* Retrieve pixel, add some fancy colouring and draw it */
				unsigned int gsVal = text[j][(k + i) % textWidth];
				unsigned int gsValGradient = gsVal? (gsVal / 256.0) * ((128 * k) / 320.0) + 128 : 0;
				display_draw(gsValGradient, 0, gsValGradient);
			}
		}
	}

_err:

	for(i = 0; i < 240; i++) {
		if(text[i])
			free(text[i]);
	}

	if(ftFontFace)
		FT_Done_Face(ftFontFace);

	if(ftLibrary)
		FT_Done_FreeType(ftLibrary);

	if(display_finish)
		display_finish();

	if(driverLibrary)
		dlclose(driverLibrary);

	return 0;
}
