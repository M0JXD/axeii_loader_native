#  Makefile - Makefile for axeiiloader/axeiiloader-gui
# Copyright (C) 2025  Jamie Drinkell
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, see <https://www.gnu.org/licenses/>.

gtk3libs := -lgtk-3 -lgdk-3 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lXext -lX11 -lm

all: deps cli gui

# TODO: Download IUP to lib
deps:
	-mkdir build lib

cli: build/axeii_utils.o build/midi_devs.o src/cli.c
	gcc src/cli.c build/axeii_utils.o build/midi_devs.o -o build/axeiiloader \
	-Wall -Werror -Wextra -Wpedantic -lasound -O2

gui: build/axeii_utils.o build/midi_devs.o src/ui.c
	gcc src/ui.c build/axeii_utils.o build/midi_devs.o -o build/axeiiloader-gui \
	-Wall -Werror -Wpedantic \
	-I./lib/iup/include \
	-L./lib/iup \
	-Wl,-Bstatic -liup \
	-Wl,-Bdynamic $(gtk3libs) -lasound -O2

build/axeii_utils.o: src/axeii_utils.c src/axeii_utils.h
	# gcc O2 optimisations mess up calculating the sysex checksum.
	gcc -c src/axeii_utils.c -o build/axeii_utils.o \
	-Wall -Werror -Wextra -Wpedantic -lasound -O1

build/midi_devs.o: src/midi_devs.c src/midi_devs.h
	gcc -c src/midi_devs.c -o build/midi_devs.o \
	-Wall -Werror -Wextra -Wpedantic -lasound -O2

cli-tcc:
	# Build the CLI version with TCC bc why not? You should start clean for this!
	tcc -c src/axeii_utils.c -o build/axeii_utils.o
	tcc -c src/midi_devs.c -o build/midi_devs.o
	tcc src/cli.c build/axeii_utils.o build/midi_devs.o -o axeiiloader -Wall -lasound

clean:
	rm -r build

run:
	#./build/axeiiloader -i test_files/PRESETS/OG/BulbRhythmPatch_og.syx
	#./build/axeiiloader -i test_files/IRS/LT_MARV412_Mix9.syx
	#./build/axeiiloader -o preset.syx
	#./build/axeiiloader -o ir.syx -m
	./build/axeiiloader-gui
