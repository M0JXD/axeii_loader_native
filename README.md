# A simple Preset/IR loader for the Axe-FX II on Linux

This repo replaces the my old axeii_loader_cli repo. It offers both the CLI app, and a native GUI (using IUP and the GTK3 backend).
You do not need the CLI app to use the GUI app, they are completely seperate but are built from the same backend code.

This is created for two reasons:

- To learn about native UI libraries (in this case IUP) as I want to make and contribute to "truly native" apps.
- I previously made a GUI version of this utility in Flutter, but at the moment the FlutterMidiCommand package it uses for the backend has bugs on Linux. However I want to keep that version "as is" and didn't want to create an FFI backend to ALSA, as there is the option of easily porting to other platforms in the future should the MIDI package be picked up and maintained again. Flutter continues to be my primary GUI library choice, and the Flutter version can still considered maintained and used instead.


## CLI usage

CLI usage is unchanged from before. You can get this help text by passing -h:

```
===== AXE-FX II LOADER =====
=== Axe-FX II Loader Help Text ===
-d and either -i or -o with a file name must be provided. You can't provide both -i and -o.

=== Options ===
-d <device>      ALSA device string. Use 'amidi -l' and pass the "Device", e.g. "hw:2,0".
-i <file name>   Set input (send mode) file (whether it's a preset or IR will be autodetected).
-o <file name>   Set output (receive mode) file.
-m               Set to get IRs in receive mode (ignored for send mode).
-p <integer>     Set Preset or IR location, defaults to 0. Ignored when sending presets as they're loaded to the edit buffer.
-t <o/x/p>       Set connected unit as Original/MKII (o), XL (x) or XL Plus (p) type unit. Defaults to Original/MkII.
-h               Show this help text.
=== END OF HELP ===
```

### Examples

- Send a preset to the edit buffer:
`axeii-loader -d "hw:2,0" -i mypreset.syx`

- Get a preset from location 100:
`axeii-loader -d "hw:2,0" -o presetname.syx -p 100`

- Send an IR to Cabinet location 15:
`axeii-loader -d "hw:2,0" -i myir.syx -p 15`

- Get an IR to Cabinet location 34:
`axeii-loader -d "hw:2,0" -o irname.syx -m -p 34`

- Note: On the Axe-FX II OG/MKII, the scratchpad locations are just the ones after 100, e.g. to send to scratchpad 2:
`axeii-loader -d "hw:2,0" -i myir.syx -p 102`


## GUI

The GUI should hopefully be quite obvious to use. Here's a screenshot:

[screenshot](assets/screenshot.png)

## Building

The makefile can be used with the `make` and will build into the root of the repo. By default it will build both the CLI and GUI programs.
The only dependencies are ALSA (which chance to be installed already) and IUP (if building the GUI frontend).

IUP precompiled library and include headers are expected to be present under the lib/iup directory. It is not checked into git so you will need to add it.
Download IUP [here](https://sourceforge.net/projects/iup/files/3.32/Linux%20Libraries/iup-3.32_Linux68_64_lib.tar.gz/download) and extract it into the folder, so that:

```
src/
    axeiiloader.c
    cli.c
    ui.c
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
When creating the UI I realised I needed to get the MIDI Device hw strings/names.
One option is to parse `amidi -l` with a a pcall but that is poor and not future-proof (if the output layout changes in a future version) and parsing strings in C is a nightmare. So instead the better option is to do the RawMIDI calls that amidi itself does. To do this right I essentially refactored amidi's source to just what the -l option does. This makes my code a derivative of amidi's and hence the project license had to be updated.
An added benefit however is now the CLI version can attempt to work out the correct device without needing to pass it.

## Notes

I have no clue how if this works with the extra locations in the XL/XL+. Nor do I know for sure how presets after 383 work.
If you're an XL/XL+ owner and want to help me out in implementing that let me know!
Eavesdropping on FractalBot (via Wine) and the Axe-FX II is pretty easy with the ReceiveMIDI tool, and I should only need a handful of information to know how it works.
- Only tested on Linux Mint 22.2 with an Axe-FX II MkII
- Unsure if it will work for XL/XL+ units.
