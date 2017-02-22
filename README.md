# PiDisplayLibs Library for Simple Screen Usage in Raspberry Pi

## Author

* André Bannwart Perina

## Contributions

This project has code snippets from:
* Rinky-dink's UTFT (http://www.rinkydinkelectronics.com/library.php?id=52);
* Sprite_tm's Raspberry micro arcade machine (http://spritesmods.com/?art=rpi_arcade&page=2).

## Introduction

PiDisplayLibs is a simple (and still quite short) collection for simple usage of TFT screens
in a Raspberry Pi using GPIO pins. It is based on shared libraries that has the following
functions:

* `display_init()`: Initialise display;
* `display_set_xy()`: Set coordinate for drawing;
* `display_draw()`: Draw a pixel;
* `display_finish()`: Free stuff and finish.

Further details about the functions can be found in the `include/display.h` file. For usage
example, see file `src/tests/test1.c`.

## Licence

This project is licensed under GPLv3 (see LICENSE file).

## Compatibility

### Supported boards

Currently, only Raspberry Pi 3 Model B is supported. It shouldn't be complicated to enable other boards, though:

* `PERI_BASE` in `src/bcmgpio.c` must be changed depending on board version:
	* For Raspberry Pi, use `0x20000000`;
	* For Raspberry Pi 3, use `0x3F000000` (default).
* Pin numbering may be different on older devices. The macros `RS_PIN`, `RW_PIN`, etc. in the drivers source files may be different for boards older than Raspberry Pi 3. ***Remember to use GPIO numbering according to BCM2835 and not according to pin header position!***

### Supported Screens

Refer to https://www.element14.com/community/servlet/JiveServlet/previewBody/73950-102-9-339300/pi3_gpio.png for
pin numbers in Raspberry Pi 3:

* ili9325 320x240 TFT screens:
```
----------------------------
| RPi3 Pin    | Screen Pin |
| 03 (GPIO02) | RS         |
| 04 (DC 5V)  | 5V         |
| 05 (GPIO03) | RW         |
| 06 (GND)    | GND        |
| 07 (GPIO04) | RD         |
| 11 (GPIO17) | DB10       |
| 13 (GPIO27) | DB11       |
| 15 (GPIO22) | DB12       |
| 19 (GPIO10) | DB13       |
| 21 (GPIO09) | DB14       |
| 23 (GPIO11) | DB15       |
| 29 (GPIO05) | DB17       |
| 31 (GPIO06) | DB19       |
| 33 (GPIO13) | CS         |
| 35 (GPIO19) | RST        |
----------------------------
```

## Repository structure

* ***bin***: Output folder for example binaries;
* ***include***: Includes folder;
	* ***bcmgpio.h***: Header for `bcmgpio` library;
	* ***common.h***: Header with general purpose macros for assertions and error checking;
	* ***display.h***: Generic header. Developers should include this file;
* ***lib***: Output folder for driver libraries;
* ***LICENSE***: Licence file;
* ***Makefile***: Project makefile;
* ***obj***: Output folder for object files;
* ***README.md***: This file, doh;
* ***src***: Sources folder;
	* ***bcmgpio.c***: Source for the `bcmgpio` library;
	* ***ili9325***: ili9325 driver folder;
		* ***ili9325.c***: ili9325 driver source;
	* ***tests***: Tests sources;
		* ***test1.c***: Print colour gradients.

## How to install and use

Under raspbian, all needed libraries are already provided. To perform a simple compile and use:

* Connect screen to Raspberry Pi board according to the previous section ***Supported Screens***;
* Clone repository;
* Run `make` for a specific library (e.g. `make lib/ili9325.so`);
* Run `make` for one of the example files (e.g. `make bin/test1`);
* Run example with superuser rights, like `sudo` (e.g. `sudo ./bin/test1 lib/ili9325.so`);

## Example programs

PiDisplayLibs comes supplied with some example source codes to give a glimpse of its usage:

* `test1.c`: Draw some gradient bars. Usage example:
```
sudo ./bin/test1 DRIVERPATH
	where DRIVERPATH is path to a display driver (*.so)
```
* `test2.c`: Scroll a simple text. Usage example:
```
sudo ./bin/test2 DRIVERPATH FONTPATH STRING REPEATAMT
	where DRIVERPATH is path to a display driver (*.so)
	      FONTPATH is path to a TTF font file (.ttf)
	      STRING is the string to be printed
          REPEATAMT the amount of times the string should be scrolled
```

## Future work

* Provide support for older boards automatically;
* Provide support for more TFT displays.
