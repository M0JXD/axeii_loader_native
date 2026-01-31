#  Makefile - Makefile for axeiiloader/axeiiloader-gui
# Copyright (C) 2025-2026 Jamie Drinkell
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

all: cli gui

cli: build_dir build_dir/axeiiloader

gui: build_dir build_dir/axeiiloader-gui

build_dir:
	-mkdir build_dir

build_dir/axeiiloader: build_dir/axeii_backend.o build_dir/axeii_loader.o src/cli.c
	NAME=`uname -s` ; \
	if [ $$NAME = "Linux" ]; then \
		cc src/cli.c build_dir/axeii_backend.o build_dir/axeii_loader.o -o build_dir/axeiiloader \
		-Wall -Werror -Wextra -Wpedantic -O2 -lasound ; \
	fi ; \
	if [ $$NAME = "FreeBSD" ]; then \
		cc src/cli.c build_dir/axeii_backend.o build_dir/axeii_loader.o -o build_dir/axeiiloader \
		-Wall -Werror -Wextra -Wpedantic -O2 ; \
	fi ;

build_dir/axeiiloader-gui: build_dir/axeii_backend.o build_dir/axeii_loader.o src/ui.c src/gtk/axeiiloader_gtk.c src/gtk/axeiiloader_gtk.h
	cc `pkg-config --cflags gtk+-3.0` src/ui.c src/gtk/axeiiloader_gtk.c build_dir/axeii_backend.o build_dir/axeii_loader.o \
	-o build_dir/axeiiloader-gui \
	-Wall -Werror -Wpedantic \
	`pkg-config --libs gtk+-3.0` -lasound

src/gtk/axeiiloader_gtk.c: src/gtk/builder.ui src/gtk/axeiiloader_gtk.gresource.xml
	glib-compile-resources --generate-source --target=src/gtk/axeiiloader_gtk.c src/gtk/axeiiloader_gtk.gresource.xml

src/gtk/axeiiloader_gtk.h: src/gtk/builder.ui src/gtk/axeiiloader_gtk.gresource.xml
	glib-compile-resources --generate-header --target=src/gtk/axeiiloader_gtk.h src/gtk/axeiiloader_gtk.gresource.xml

# gcc -O2 optimisations mess up calculating the sysex checksum.
# TODO: Does Clang have the issue?
build_dir/axeii_loader.o: src/axeii_loader.c
	cc -c src/axeii_loader.c -o build_dir/axeii_loader.o \
	-Wall -Werror -Wpedantic -O1

# Detect the platform and build ALSA on Linux, or OSS on FreeBSD
build_dir/axeii_backend.o: src/alsa/axeii_alsa.c src/oss/axeii_oss.c
	NAME=`uname -s` ; \
	if [ $$NAME = "Linux" ]; then \
		cc -c src/alsa/axeii_alsa.c -o build_dir/axeii_backend.o \
		-Wall -Werror -Wextra -Wpedantic -O2 -lasound ; \
	fi ; \
	if [ $$NAME = "FreeBSD" ]; then \
		cc -c src/alsa/axeii_oss.c -o build_dir/axeii_backend.o \
		-Wall -Werror -Wextra -Wpedantic -O2 ; \
	fi ;

# Build the ALSA CLI version with TCC bc why not? You should start clean for this!
cli-tcc: build_dir
	tcc -c src/alsa/axeii_alsa.c -o build_dir/axeii_backend.o
	tcc -c src/axeii_loader.c -o build_dir/axeii_loader.o
	tcc src/cli.c build_dir/axeii_loader.o build_dir/axeii_backend.o -o build_dir/axeiiloader -Wall -lasound

clean:
	rm -r build_dir
	rm *.syx src/gtk/axeiiloader_gtk.c src/gtk/axeiiloader_gtk.h

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
