#include <dlfcn.h>
#include <errno.h>
#include <png.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* jpeglib.h must come after stdio.h */
#include <jpeglib.h>

#include "common.h"
#include "display.h"

typedef struct {
	struct jpeg_error_mgr stdErrorMgr;
	jmp_buf jmpBuffer;
} silent_error_mgr;

METHODDEF(void) silent_error_jump(j_common_ptr jpegInfo) {
	silent_error_mgr *errorMgr = (silent_error_mgr *) jpegInfo->err;
	longjmp(errorMgr->jmpBuffer, 1);
}

int pngMode(int argc, char *argv[]) {
	int rv = 0;
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
	unsigned int pngComponents;
	unsigned int pngWidth, pngHeight;
	int pngDepth, pngColorType;
	//double pngDisplayExponent = 2.2;
	int pngRowbytes;
	int pngChannels;
	unsigned char *pngData = NULL;
	unsigned char **pngRowpointers = NULL;
	unsigned char r, g, b;

	/* Check if drivers .so file was informed */
	ASSERT(4 == argc, rv = -1; fprintf(stderr, "Usage: %s DRIVERSOFILE IMGFILE ORIENTATION\n", argv[0]));
	driverLibPath = argv[1];
	pngPath = argv[2];
	orientation = atoi(argv[3]) % 4;

	/* Attempt to load driver library */
	driverLibrary = dlopen(driverLibPath, RTLD_LAZY);
	ASSERT(driverLibrary != NULL, rv = -1; fprintf(stderr, "Error: dlopen(): %s\n", dlerror()));

	/* Retrieve display_init() */
	display_init = dlsym(driverLibrary, "display_init");
	ASSERT(display_init != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_init\"): %s\n", dlerror()));

	/* Retrieve display_set_xy() */
	display_set_xy = dlsym(driverLibrary, "display_set_xy");
	ASSERT(display_set_xy != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_set_xy\"): %s\n", dlerror()));

	/* Retrieve display_draw() */
	display_draw = dlsym(driverLibrary, "display_draw");
	ASSERT(display_draw != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_draw\"): %s\n", dlerror()));

	/* Retrieve display_finish() */
	display_finish = dlsym(driverLibrary, "display_finish");
	ASSERT(display_finish != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_finish\"): %s\n", dlerror()));

	pngFile = fopen(argv[2], "rb");
	ASSERT(pngFile, rv = -1; fprintf(stderr, "Error: %s: %s\n", strerror(errno), pngPath));

	fread(pngSignature, 1, 8, pngFile);
	ASSERT(png_check_sig(pngSignature, 8), rv = -2; fprintf(stderr, "Error: Invalid PNG file: %s\n", pngPath));

	pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	ASSERT(pngStruct, rv = -1; fprintf(stderr, "Error: Failed to create png struct\n"));

	pngInfo = png_create_info_struct(pngStruct);
	ASSERT(pngInfo, rv = -1; fprintf(stderr, "Error: Failed to create png info\n"));

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
	pngComponents = png_get_channels(pngStruct, pngInfo);

	pngData = malloc(pngRowbytes * pngHeight);
	ASSERT(pngData, rv = -1; fprintf(stderr, "Error: Out of memory!\n"));
	pngRowpointers = malloc(pngHeight * sizeof(unsigned char *));
	ASSERT(pngData, rv = -1; fprintf(stderr, "Error: Out of memory!\n"));
	for(i = 0; i < pngHeight; i++)
		pngRowpointers[i] = pngData + i * pngRowbytes;
	png_read_image(pngStruct, pngRowpointers);
	png_read_end(pngStruct, NULL);

	/* Initialise display */
	retVal = display_init(NULL, 0);
	ASSERT(DISPLAY_OK == retVal, rv = -1; fprintf(stderr, "Error: display_init() failed with code %d\n", retVal));

	/* Set coordinate to (0,0) */
	display_set_xy(0, 0);

	for(i = 0; i < 240; i++) {
		for(j = 0; j < 320; j++) {
			if(0 == orientation) {
				if(j < pngWidth && i < pngHeight) {
					r = pngRowpointers[i][(pngComponents * j)];
					g = pngRowpointers[i][(pngComponents * j) + 1];
					b = pngRowpointers[i][(pngComponents * j) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(1 == orientation) {
				if(i < pngWidth && j < pngHeight) {
					r = pngRowpointers[pngHeight - j - 1][(pngComponents * i)];
					g = pngRowpointers[pngHeight - j - 1][(pngComponents * i) + 1];
					b = pngRowpointers[pngHeight - j - 1][(pngComponents * i) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(2 == orientation) {
				if(j < pngWidth && i < pngHeight) {
					r = pngRowpointers[pngHeight - i - 1][(pngComponents * (pngWidth - j - 1))];
					g = pngRowpointers[pngHeight - i - 1][(pngComponents * (pngWidth - j - 1)) + 1];
					b = pngRowpointers[pngHeight - i - 1][(pngComponents * (pngWidth - j - 1)) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(3 == orientation) {
				if(i < pngWidth && j < pngHeight) {
					r = pngRowpointers[j][(pngComponents * (pngWidth - i - 1))];
					g = pngRowpointers[j][(pngComponents * (pngWidth - i - 1)) + 1];
					b = pngRowpointers[j][(pngComponents * (pngWidth - i - 1)) + 2];
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

	return rv;
}

int jpgMode(int argc, char *argv[]) {
	int rv = 0;
	int i, j;
	int orientation;
	char *driverLibPath;
	void *driverLibrary = NULL;
	int (* display_init)(void *, int) = NULL;
	int (* display_set_xy)(unsigned int, unsigned int) = NULL;
	int (* display_draw)(unsigned char, unsigned char,  unsigned char) = NULL;
	int (* display_finish)(void) = NULL;
	int retVal = DISPLAY_OK;
	char *jpgPath;
	FILE *jpgFile = NULL;
	struct jpeg_decompress_struct jpegInfo;
	silent_error_mgr errorMgr;
	bool jpegInfoCreated = false;
	JSAMPARRAY jpgBuffer;
	unsigned int jpgComponents;
	unsigned int jpgWidth, jpgHeight;
	unsigned char *jpgData = NULL;
	unsigned char **jpgRowpointers = NULL;
	unsigned char r, g, b;

	/* Check if drivers .so file was informed */
	ASSERT(4 == argc, rv = -1; fprintf(stderr, "Usage: %s DRIVERSOFILE IMGFILE ORIENTATION\n", argv[0]));
	driverLibPath = argv[1];
	jpgPath = argv[2];
	orientation = atoi(argv[3]) % 4;

	/* Attempt to load driver library */
	driverLibrary = dlopen(driverLibPath, RTLD_LAZY);
	ASSERT(driverLibrary != NULL, rv = -1; fprintf(stderr, "Error: dlopen(): %s\n", dlerror()));

	/* Retrieve display_init() */
	display_init = dlsym(driverLibrary, "display_init");
	ASSERT(display_init != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_init\"): %s\n", dlerror()));

	/* Retrieve display_set_xy() */
	display_set_xy = dlsym(driverLibrary, "display_set_xy");
	ASSERT(display_set_xy != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_set_xy\"): %s\n", dlerror()));

	/* Retrieve display_draw() */
	display_draw = dlsym(driverLibrary, "display_draw");
	ASSERT(display_draw != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_draw\"): %s\n", dlerror()));

	/* Retrieve display_finish() */
	display_finish = dlsym(driverLibrary, "display_finish");
	ASSERT(display_finish != NULL, rv = -1; fprintf(stderr, "Error: dlsym(\"display_finish\"): %s\n", dlerror()));

	jpgFile = fopen(argv[2], "rb");
	ASSERT(jpgFile, rv = -1; fprintf(stderr, "Error: %s: %s\n", strerror(errno), jpgPath));

	jpegInfo.err = jpeg_std_error(&(errorMgr.stdErrorMgr));
	errorMgr.stdErrorMgr.error_exit = silent_error_jump;
	if(setjmp(errorMgr.jmpBuffer)) {
		(*jpegInfo.err->output_message)((j_common_ptr) &jpegInfo);
		ASSERT(0, rv = -1; fprintf(stderr, "JPEG error\n"));
	}

	jpeg_create_decompress(&jpegInfo);
	jpegInfoCreated = true;
	jpeg_stdio_src(&jpegInfo, jpgFile);

	jpeg_read_header(&jpegInfo, true);
	jpeg_start_decompress(&jpegInfo);

	jpgComponents = jpegInfo.output_components;
	jpgWidth = jpegInfo.output_width * jpgComponents;
	jpgHeight = jpegInfo.output_height;
	jpgBuffer = (*jpegInfo.mem->alloc_sarray)((j_common_ptr) &jpegInfo, JPOOL_IMAGE, jpgWidth, 1);

	jpgData = malloc(jpgWidth * jpgHeight);
	ASSERT(jpgData, rv = -1; fprintf(stderr, "Error: Out of memory!\n"));
	jpgRowpointers = malloc(jpgHeight * sizeof(unsigned char *));
	ASSERT(jpgData, rv = -1; fprintf(stderr, "Error: Out of memory!\n"));
	for(i = 0; i < jpgHeight; i++)
		jpgRowpointers[i] = jpgData + i * jpgWidth;

	for(i = 0; jpegInfo.output_scanline < jpgHeight; i++) {
		jpeg_read_scanlines(&jpegInfo, jpgBuffer, 1);
		memcpy(jpgRowpointers[i], jpgBuffer[0], jpgWidth);
	}

	/* Initialise display */
	retVal = display_init(NULL, 0);
	ASSERT(DISPLAY_OK == retVal, rv = -1; fprintf(stderr, "Error: display_init() failed with code %d\n", retVal));

	/* Set coordinate to (0,0) */
	display_set_xy(0, 0);

	for(i = 0; i < 240; i++) {
		for(j = 0; j < 320; j++) {
			if(0 == orientation) {
				if(j < jpgWidth && i < jpgHeight) {
					r = jpgRowpointers[i][(jpgComponents * j)];
					g = jpgRowpointers[i][(jpgComponents * j) + 1];
					b = jpgRowpointers[i][(jpgComponents * j) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(1 == orientation) {
				if(i < jpgWidth && j < jpgHeight) {
					r = jpgRowpointers[jpgHeight - j - 1][(jpgComponents * i)];
					g = jpgRowpointers[jpgHeight - j - 1][(jpgComponents * i) + 1];
					b = jpgRowpointers[jpgHeight - j - 1][(jpgComponents * i) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(2 == orientation) {
				if(j < jpgWidth && i < jpgHeight) {
					r = jpgRowpointers[jpgHeight - i - 1][(jpgComponents * (jpgWidth - j - 1))];
					g = jpgRowpointers[jpgHeight - i - 1][(jpgComponents * (jpgWidth - j - 1)) + 1];
					b = jpgRowpointers[jpgHeight - i - 1][(jpgComponents * (jpgWidth - j - 1)) + 2];
				}
				else {
					r = 0;
					g = 0;
					b = 0;
				}
			}
			else if(3 == orientation) {
				if(i < jpgWidth && j < jpgHeight) {
					r = jpgRowpointers[j][(jpgComponents * (jpgWidth - i - 1))];
					g = jpgRowpointers[j][(jpgComponents * (jpgWidth - i - 1)) + 1];
					b = jpgRowpointers[j][(jpgComponents * (jpgWidth - i - 1)) + 2];
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

	if(jpgRowpointers)
		free(jpgRowpointers);

	if(jpgData);
		free(jpgData);

	if(jpegInfoCreated)
		jpeg_destroy_decompress(&jpegInfo);

	if(display_finish)
		display_finish();

	if(driverLibrary)
		dlclose(driverLibrary);

	if(jpgFile)
		fclose(jpgFile);

	return rv;
}

int main(int argc, char *argv[]) {
	int rv;

	rv = pngMode(argc, argv);
	if(-2 == rv)
		rv = jpgMode(argc, argv);

	return 0;
}
