# Copyright 2025 Jamie Drinkell. MIT License.

# A simple utility to load presets and IRs in/out of an Axe-FX II
# Only tested on Linux Mint 22.2 with an Axe-FX II MkII
# Unsure if it will work for XL/XL+ units.
# NO WARRANTY IS PROVIDED, USE AT YOUR OWN RISK.

gtk3libs := -lgtk-3 -lgdk-3 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lXext -lX11 -lm

all: cli gui

cli: libaxeii_loader.o src/cli.c
	gcc src/cli.c libaxeii_loader.o -o axeiiloader -Wall -Werror -Wextra -Wpedantic -lasound -O2

gui: libaxeii_loader.o src/ui.c
	gcc src/ui.c libaxeii_loader.o -o axeiiloader-gui \
	-I./lib/iup/include \
	-L./lib/iup \
	-Wl,-Bstatic -liup \
	-Wl,-Bdynamic $(gtk3libs) -lasound -O2

libaxeii_loader.o: src/axeii_loader.c src/axeii_loader.h
	# gcc O2 optimisations mess up calculating the sysex checksum.
	gcc -c src/axeii_loader.c -o libaxeii_loader.o -lasound -O1

midi_devs.o: src/midi_devs.c
	gcc -c src/midi_devs.c -o midi_devs.o -lasound -O2

cli-tcc:
	# Build the CLI version with TCC bc why not? You should start clean for this!
	tcc -c src/axeii_loader.c -o libaxeii_loader.o
	tcc src/cli.c libaxeii_loader.o -o axeiiloader -Wall -lasound

clean:
	rm axeiiloader axeiiloader-gui libaxeii_loader.o
