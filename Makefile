# #############################################################################################
# # PiDisplayLibs Makefile                                                                    #
# # Author: André Bannwart Perina                                                             #
# #############################################################################################
# # Copyright (c) 2017 André B. Perina                                                        #
# #                                                                                           #
# # This file is part of PiDisplayLibs                                                        #
# #                                                                                           #
# # PiDisplayLibs is free software: you can redistribute it and/or modify it under the terms  #
# # of the GNU General Public License as published by the Free Software Foundation, either    #
# # version 3 of the License, or (at your option) any later version.                          #
# #                                                                                           #
# # PiDisplayLibs is distributed in the hope that it will be useful, but WITHOUT ANY          #
# # WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A           #
# # PARTICULAR PURPOSE.  See the GNU General Public License for more details.                 #
# #                                                                                           #
# # You should have received a copy of the GNU General Public License along with Foobar.  If  #
# # not, see <http://www.gnu.org/licenses/>.                                                  #
# #############################################################################################

ifeq ($(DEBUG),yes)
    DEBUGFLAG=-g
endif

bin/test2: src/tests/test2.c
	mkdir -p bin
	$(CC) $< -Iinclude `freetype-config --cflags` -o $@ -ldl $(DEBUGFLAG) -O3 `freetype-config --libs`

bin/test%: src/tests/test%.c
	mkdir -p bin
	$(CC) $< -Iinclude -o $@ -ldl $(DEBUGFLAG) -O3

lib/ili9325.so: src/ili9325/ili9325.c include/display.h obj/bcmgpio.o
	mkdir -p lib
	$(CC) -fpic -shared -Iinclude src/ili9325/ili9325.c obj/bcmgpio.o -o $@ $(DEBUGFLAG) -O3

obj/bcmgpio.o: src/bcmgpio.c include/bcmgpio.h
	mkdir -p obj
	$(CC) -c src/bcmgpio.c -Iinclude -o obj/bcmgpio.o $(DEBUGFLAG) -O3

clean:
	rm -rf obj
	rm -rf bin
	rm -rf lib
