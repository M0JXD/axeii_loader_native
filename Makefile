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
UNAME_S := $(shell uname -s)

all: cli gui

cli: build_dir build_dir/axeiiloader

gui: build_dir build_dir/axeiiloader-gui

# TODO: Download IUP to lib
build_dir:
	-mkdir build_dir

build_dir/axeiiloader: build_dir/axeii_utils.o src/cli.c
	cc src/cli.c build_dir/axeii_utils.o -o build_dir/axeiiloader \
	-Wall -Werror -Wextra -Wpedantic -lasound -O2

build_dir/axeiiloader-gui: build_dir/axeii_utils.o src/ui.c
	cc src/ui.c build_dir/axeii_utils.o -o build_dir/axeiiloader-gui \
	-Wall -Werror -Wpedantic \
	-I./lib/iup/include -L./lib/iup -Wl,-Bstatic -liup \
	-Wl,-Bdynamic $(gtk3libs) -lasound -O2

ifeq ($(UNAME_S),Linux)
# ALSA BACKEND
build_dir/axeii_utils.o: src/alsa/axeii_utils_alsa.c src/axeii_utils.h
	# cc -O2 optimisations mess up calculating the sysex checksum.
	cc -c src/alsa/axeii_utils_alsa.c -o build_dir/axeii_utils.o \
	-Wall -Werror -Wextra -Wpedantic -lasound -O1
endif

ifeq ($(UNAME_S),FreeBSD)
# OSS BACKEND
build_dir/axeii_utils.o: src/oss/axeii_utils_alsa.c src/axeii_utils.h
	# TODO: Does Clang have this issue?
	cc -c src/alsa/axeii_utils_oss.c -o build_dir/axeii_utils.o \
	-Wall -Werror -Wextra -Wpedantic -lasound -O2
endif

cli-tcc: build_dir
	# Build the ALSA CLI version with TCC bc why not? You should start clean for this!
	tcc -c src/axeii_utils.c -o build_dir/axeii_utils.o
	tcc -c src/midi_devs.c -o build_dir/midi_devs.o
	tcc src/cli.c build_dir/axeii_utils.o build_dir/midi_devs.o -o build_dir/axeiiloader -Wall -lasound

clean:
	rm -r build_dir
	rm *.syx

# Run various scenarios that should pass
run:
	# Send a preset to the edit buffer
	./build_dir/axeiiloader -i test_files/presets/og/BulbRhythmPatch_og.syx
	@echo -e "\n"
	@sleep 1
	# Get a preset from position 150
	./build_dir/axeiiloader -p 150
	@echo -e "\n"
	@sleep 1
	# Send a short (OG captured) IR to position 68
	./build_dir/axeiiloader -i test_files/irs/short_mad_oak_basketweave_r121.syx -p 68
	@echo -e "\n"
	@sleep 1
	# Send a long (XL/XL+ captured) IR to position 69
	./build_dir/axeiiloader -i test_files/irs/LT_MARV412_Mix_9.syx -p 69
	@echo -e "\n"
	@sleep 1
	# Get an IR from position 70
	./build_dir/axeiiloader -m -p 70
	@echo -e "\n"
	@sleep 1
	# Clear edit buffer and IRs 68/69
	./build_dir/axeiiloader -i assets/empty_preset.syx
	@echo -e "\n"
	@sleep 1
	./build_dir/axeiiloader -i assets/empty_ir.syx -p 68
	@echo -e "\n"
	@sleep 1
	./build_dir/axeiiloader -i assets/empty_ir.syx -p 69
	@echo -e "\n"
	@sleep 1
	# Run the GUI version
	./build_dir/axeiiloader-gui

# Run various scenarios that should fail
tests:
	# Try to send an XL preset to an OG
	-./build_dir/axeiiloader -i test_files/presets/xl/MarkDay90sEVHSolo_xl.syx
	@echo -e "\n"
	@sleep 1
	# Try to fetch an IR from position 0
	-./build_dir/axeiiloader -m -p 0
	@echo -e "\n"
	@sleep 1
	# Try to fetch a preset location 500 on an OG
	-./build_dir/axeiiloader -p 500
