# A simple Preset/IR loader for the Axe-FX II on Linux

This is a simple loading/exporting utility for the Axe-FX II. It offers both the CLI app, and a native GUI (using IUP and the GTK3 back-end).
You do not need the CLI app to use the GUI app, they are completely separate but are built from the same back-end code.

I previously made a [GUI version of this utility in Flutter](https://github.com/M0JXD/axeii_loader), but at the moment the FlutterMidiCommand package it uses for the back-end has bugs on Linux. However I want to keep that version "as is" with the package (instead of replacing it with a FFI back-end), as there is the option of easily porting to other platforms in the future should the MIDI package be picked up and maintained again.

## CLI usage

Without arguments, the CLI attempts to receive from the first preset location.
You must specify a file with -i to change to send mode.
If you'd prefer to set your own name for received files than the one provided by the tool, you can do so with -o.
You can get this help text by passing -h:

```
===== AXE-FX II LOADER =====
=== Axe-FX II Loader Help Text ===
The default mode is receive mode, and will save obtained files into the current directory.
If the -d option is not given it will try to autodetect from up to the first five available devices.
The unit type (-t option) does not autodetect, and transfers may fail if it's incorrect.

=== Options ===
-d <device>      ALSA device string. Use 'amidi -l' and pass the "Device", e.g. "hw:2,0".
-i <file name>   Set send mode with given input file (whether it's a preset or IR will be autodetected).
-o <file name>   Override the provided file name for receive mode.
-m               Set to get IRs in receive mode (ignored for send mode).
-p <integer>     Set Preset or IR location, defaults to 0. Ignored when sending presets as they're loaded to the edit buffer.
-t <o/x/p>       Set connected unit as Original/MKII (o), XL (x) or XL Plus (p). Defaults to Original/MKII.
-h               Show this help text.
=== END OF HELP ===
```

### Examples

- Send a preset to the edit buffer, specifying the device string:
`axeii-loader -d "hw:2,0" -i mypreset.syx`

- Get a preset from location 100:
`axeii-loader -p 100`

- Send an IR to Cabinet location 15:
`axeii-loader -i myir.syx -p 15`

- Get an IR from Cabinet location 34:
`axeii-loader -m -p 34`

- Note: Scratchpad locations are just the ones after the last available slot, e.g. to send to scratchpad 2 on an OG/MKII:
`axeii-loader -i myir.syx -p 102`

  - And likewise on an XL:
`axeii-loader -i myir.syx -p 1026 -t x`


## GUI

The GUI should hopefully be quite obvious to use. Here's a screenshot:

![screenshot](assets/screenshot.png)

## Building

Run `make` and it will build into a folder it creates called `build_dir`. By default it will build both the CLI and GUI programs, although you can specify either `cli` or `gui` targets.

The only dependencies are ALSA (on Debian based this can be got with `sudo apt install libasound2-dev`) and IUP (if building the GUI frontend).

IUP's pre-compiled library and include headers are expected to be present under the lib/iup directory. It is not checked into git so you will need to add it.
Download IUP [here](https://sourceforge.net/projects/iup/files/3.32/Linux%20Libraries/iup-3.32_Linux68_64_lib.tar.gz/download) and extract it into the folder, so that:

```
src/
    axeii_utils.c
    cli.c
    ui.c
    ...
lib/
    iup/
        libiup.a
        libiup.so
        include/
            iup.h
...
```

## License

The original license was MIT (which is my preference), but this had to be changed to the GPLv2.
When creating the UI I realised I needed to get the MIDI device hardware strings/names.
One option is to call and parse `amidi -l` but that is poor and not future-proof (if the output layout changes in a future version) and parsing strings in C is a nightmare. So instead the better option is to do the RawMIDI calls that amidi itself does. To do this right I essentially refactored amidi's source to just what the -l option does. This makes my code a derivative of amidi's and hence the project license had to be updated.
An added benefit however is that the CLI version can now attempt to work out the correct device without needing to pass it.

## Notes

I have only tested on Linux Mint 22.2 with an Axe-FX II MkII. It has also been used on Bazzite with an XL+.
A GCC bug with -02 optimisations messes up the sysex checksum calculation, so the base is built with -01.
When detecting MIDI devices in both the CLI and GUI versions, it will stop looking/counting after five devices.
I can improve this to check all devices but will require reworking some aspects. Please open an issue if you need this fixed, otherwise I'm happy to leave as is.

## Credits

Thanks to:

- Geert Bevin's very handy ReceiveMIDI tool - https://github.com/gbevin/ReceiveMIDI
- The Wine contributors, which has let me run Fractal-Bot and Axe-Edit fairly well.
- The contributors to the Axe-FX Wiki's
- @Wepeell for taking the time to provide me with the information to support XL(+) units.
