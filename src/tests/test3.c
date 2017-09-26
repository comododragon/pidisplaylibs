#include <dlfcn.h>
#include <errno.h>
#include <png.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "display.h"

int main(int argc, char *argv[]) {
	int i, j;
	int orientation;
	char *driverLibPath;
	void *driverLibrary = NULL;
	int (* display_init)(void *, int) = NULL;
	int (* display_set_xy)(unsigned int, unsigned int) = NULL;
	int (* display_draw)(unsigned char, unsigned char,  unsigned char) = NULL;
	int (* display_finish)(void) = NULL;
	int retVal = DISPLAY_OK;
	char *pngPath;
	FILE *pngFile = NULL;
	unsigned char pngSignature[8];
	png_structp pngStruct = NULL;
	png_infop pngInfo = NULL;
	unsigned int pngWidth, pngHeight;
	int pngDepth, pngColorType;
	//double pngDisplayExponent = 2.2;
	int pngRowbytes;
	int pngChannels;
	unsigned char *pngData = NULL;
	unsigned char **pngRowpointers = NULL;
	unsigned char r, g, b;

	/* Check if drivers .so file was informed */
	ASSERT(4 == argc, fprintf(stderr, "Usage: %s DRIVERSOFILE PNGFILE ORIENTATION\n", argv[0]));
	driverLibPath = argv[1];
	pngPath = argv[2];
	orientation = atoi(argv[3]) % 4;

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

	pngFile = fopen(argv[2], "rb");
	ASSERT(pngFile, fprintf(stderr, "Error: %s: %s\n", strerror(errno), pngPath));

	fread(pngSignature, 1, 8, pngFile);
	ASSERT(png_check_sig(pngSignature, 8), fprintf(stderr, "Error: Invalid PNG file: %s\n", pngPath));

	pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	ASSERT(pngStruct, fprintf(stderr, "Error: Failed to create png struct\n"));

	pngInfo = png_create_info_struct(pngStruct);
	ASSERT(pngInfo, fprintf(stderr, "Error: Failed to create png info\n"));

	png_init_io(pngStruct, pngFile);
	png_set_sig_bytes(pngStruct, 8);
	png_read_info(pngStruct, pngInfo);
	png_get_IHDR(pngStruct, pngInfo, &pngWidth, &pngHeight, &pngDepth, &pngColorType, NULL, NULL, NULL);

	// TODO: Deal with BG?

	if(PNG_COLOR_TYPE_PALETTE == pngColorType)
		png_set_expand(pngStruct);
	if(PNG_COLOR_TYPE_GRAY == pngColorType && pngDepth < 8)
		png_set_expand(pngStruct);
	if(png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS))
		png_set_expand(pngStruct);

	if(16 == pngDepth)
		png_set_strip_16(pngStruct);
	if(PNG_COLOR_TYPE_GRAY == pngColorType || PNG_COLOR_TYPE_GRAY_ALPHA == pngColorType)
		png_set_gray_to_rgb(pngStruct);

	png_read_update_info(pngStruct, pngInfo);
	pngRowbytes = png_get_rowbytes(pngStruct, pngInfo);
	pngChannels = png_get_channels(pngStruct, pngInfo);

	pngData = malloc(pngRowbytes * pngHeight);
	ASSERT(pngData, fprintf(stderr, "Error: Out of memory!\n"));
	pngRowpointers = malloc(pngHeight * sizeof(unsigned char *));
	ASSERT(pngData, fprintf(stderr, "Error: Out of memory!\n"));
	for(i = 0; i < pngHeight; i++)
		pngRowpointers[i] = pngData + i * pngRowbytes;
	png_read_image(pngStruct, pngRowpointers);
	png_read_end(pngStruct, NULL);

	printf("%d %d %d %d %d %d %d %d\n", pngRowpointers[0][0] & 0xff, pngRowpointers[0][1] & 0xff,  pngRowpointers[0][2] & 0xff, pngRowpointers[0][3] & 0xff, pngRowpointers[0][4] & 0xff, pngRowpointers[0][5] & 0xff,  pngRowpointers[0][6] & 0xff, pngRowpointers[0][7] & 0xff);

	/* Initialise display */
	retVal = display_init(NULL, 0);
	ASSERT(DISPLAY_OK == retVal, fprintf(stderr, "Error: display_init() failed with code %d\n", retVal));

	/* Set coordinate to (0,0) */
	display_set_xy(0, 0);

	switch(orientation) {
		case 0:
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}

	for(i = 0; i < 240; i++) {
		for(j = 0; j < 320; j++) {
			if(0 == orientation) {
				if(j < pngWidth && i < pngHeight) {
					r = pngRowpointers[i][(4 * j)];
					g = pngRowpointers[i][(4 * j) + 1];
					b = pngRowpointers[i][(4 * j) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(1 == orientation) {
				if(i < pngWidth && j < pngHeight) {
					r = pngRowpointers[pngHeight - j][(4 * i)];
					g = pngRowpointers[pngHeight - j][(4 * i) + 1];
					b = pngRowpointers[pngHeight - j][(4 * i) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(2 == orientation) {
				if(j < pngWidth && i < pngHeight) {
					r = pngRowpointers[pngHeight - i][(4 * (pngWidth - j))];
					g = pngRowpointers[pngHeight - i][(4 * (pngWidth - j)) + 1];
					b = pngRowpointers[pngHeight - i][(4 * (pngWidth - j)) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(3 == orientation) {
				if(i < pngWidth && j < pngHeight) {
					r = pngRowpointers[j][(4 * (pngWidth - i))];
					g = pngRowpointers[j][(4 * (pngWidth - i)) + 1];
					b = pngRowpointers[j][(4 * (pngWidth - i)) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}

			display_draw(r, g, b);
		}
	}

_err:

	if(display_finish)
		display_finish();

	if(driverLibrary)
		dlclose(driverLibrary);

	if(pngRowpointers)
		free(pngRowpointers);

	if(pngData);
		free(pngData);

	if(pngStruct || pngInfo)
		png_destroy_read_struct(&pngStruct, &pngInfo, NULL);

	if(pngFile)
		fclose(pngFile);

	return 0;
}
