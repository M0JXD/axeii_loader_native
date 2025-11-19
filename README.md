# A simple Preset/IR loader for the Axe-FX II on Linux

This repo replaces the my old axeii_loader_cli repo. It offers both the CLI app, and a native GUI (using IUP and the GTK3 backend).
You do not need the CLI app to use the GUI app, they are completely seperate but are built from the same back-end code.

This is created for two reasons:

- To learn about native UI libraries (in this case IUP) as I want to make and contribute to "truly native" apps.
- I previously made a GUI version of this utility in Flutter, but at the moment the FlutterMidiCommand package it uses for the back-end has bugs on Linux. However I want to keep that version "as is" with the package (instead of replacing it with a FFI back-end), as there is the option of easily porting to other platforms in the future should the MIDI package be picked up and maintained again. Flutter continues to be my primary GUI library choice, and the Flutter version can still considered maintained and used instead.

## CLI usage

CLI usage is mostly unchanged from before, but it can now autodetect the Axe-FX II.
You can get this help text by passing -h:

```
===== AXE-FX II LOADER =====
=== Axe-FX II Loader Help Text ===
Either -i or -o with a file name must be provided. You can't provide both -i and -o.
If the -d option is not given it will try to autodetect from up to the first five available devices.
The unit type (-t option) does not autodetect, and transfers may fail if it's incorrect.

=== Options ===
-d <device>      ALSA device string. Use 'amidi -l' and pass the "Device", e.g. "hw:2,0".
-i <file name>   Set input (send mode) file (whether it's a preset or IR will be autodetected).
-o <file name>   Set output (receive mode) file.
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
`axeii-loader -o presetname.syx -p 100`

- Send an IR to Cabinet location 15:
`axeii-loader -i myir.syx -p 15`

- Get an IR to Cabinet location 34:
`axeii-loader -o irname.syx -m -p 34`

- Note: On the Axe-FX II OG/MKII, the scratchpad locations are just the ones after 100, e.g. to send to scratchpad 2:
`axeii-loader -i myir.syx -p 102`


## GUI

The GUI should hopefully be quite obvious to use. Here's a screenshot:

![screenshot](assets/screenshot.png)

## Building

The makefile can be used with the `make` and will build into a folder it will create called `build_dir`. By default it will build both the CLI and GUI programs.
The only dependencies are ALSA (on Debian based this can be got with `sudo apt install libasound2-dev`) and IUP (if building the GUI frontend).

IUP's pre-compiled library and include headers are expected to be present under the lib/iup directory. It is not checked into git so you will need to add it.
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
When creating the UI I realised I needed to get the MIDI Device hardware strings/names.
One option is to call and parse `amidi -l` but that is poor and not future-proof (if the output layout changes in a future version) and parsing strings in C is a nightmare. So instead the better option is to do the RawMIDI calls that amidi itself does. To do this right I essentially refactored amidi's source to just what the -l option does. This makes my code a derivative of amidi's and hence the project license had to be updated.
An added benefit however is that the CLI version can now attempt to work out the correct device without needing to pass it.

## Notes

I have only tested on Linux Mint 22.2 with an Axe-FX II MkII.
A gcc bug with -02 optimisations messes up the sysex checksum calculation, so the base is built with -01.
When detecting MIDI devices in both the CLI and GUI versions, it will stop looking/counting after five devices.
I can improve this to check all devices but will require reworking some aspects. Please open an issue if you need this fixed, otherwise I'm happy to leave as is.

I also have no clue if this works with the extra locations in the XL/XL+, e.g. how presets after 383 work or IRs after 100.
I've made some best attempt guesses to support it. If you're an XL/XL+ owner and want to help me out in implementing that let me know!
Eavesdropping on FractalBot (via Wine) and the Axe-FX II is pretty easy with the ReceiveMIDI tool, and I should only need a handful of information to know how it works.

### AI Usage

I want to be clear and transparent about my AI usage. Often I see projects that the developer claims is their own work, when in fact they merely prompted it into existence. I like programming and am hoping to become good enough to be employed doing it full time, so I want anyone reviewing my work to understand what I'm actually capable of. Prior projects of mine (the Flutter & old CLI version, the morse trainer etc.) make very minimal usage of AI.

ChatGPT was used in the earlier stages of build the IUP GUI, but after it generated code with [the wrong attributes and even a syntax error](https://chatgpt.com/share/691b851f-0dc8-8001-83e1-f58034df940f) I completely lost faith in using the technology, and when reading back through the code found many places even a mediocre programmer like myself was able to improve (it often resorted to fixing sizes instead of using IUPs layout engine to it's advantage). Hence from the commit "Rewrite ChatGPT's dodgy layout code" I have done mostly everything myself, but even a Google search gives AI overviews these days.

As oppose to a code generator I find AI useful as a suped up Google search (and occasionally it surprises me, e.g. I tried to build the UI on Windows and it worked out from the very unclear error message that the problem is that IUP's pre-compiled MinGW (MSVCRT) binaries don't work with the UCRT environment, which I simply could not find an answer to with Google), and I make a genuine effort to truly understand what it gives me.

## Credits

Thanks to:

- Geert Bevin's very handy ReceiveMIDI tool - https://github.com/gbevin/ReceiveMIDI
- The Wine contributors, which has let me run Fractal-Bot and Axe-Edit fairly well.
- Fractal Audio for making the best modellers
- The contributors to the Axe-FX Wiki's
