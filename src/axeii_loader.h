/* Copyright 2025 Jamie Drinkell. MIT License. */

/* A simple utility to load presets and IRs in/out of an Axe-FX II
 * Only tested on Linux Mint 22.2 with an Axe-FX II MkII
 * Unsure if it will work for XL/XL+ units.
 * NO WARRANTY IS PROVIDED, USE AT YOUR OWN RISK.
 */

#ifndef AXEII_LOADER_H
#define AXEII_LOADER_H

/* DEFINES */
/* File Properties */

/* From LSB to MSB, the bits represent:
 * 0: True for valid file
 * 1: True for preset file, false for IR file
 * 2: Is OG/MkII file
 * 3: Is XL file
 * 4: Is XL+ file
 */
#define OG_IR          0b00000101
#define OG_PRESET      0b00000111
#define XL_IR          0b00001001
#define XL_PRESET      0b00001011
#define XL_PLUS_IR     0b00010001
#define XL_PLUS_PRESET 0b00010011

/* FUNCTION DECLARATIONS */

/** Sets up the libraries internal RawMIDI handles to the device
 * @param char* devString The ALSA device string as obtained from "amidi -l"
 * @return char 0 on success, -1 on failure
 */
char setupRawMIDIHandles(char* devString);

/** Closes the libraries previously opened RAWMIDI handles
 * @return char 0 on success
 */
char closeRawMIDIHandles(void);

/** Detects the properties of the file at the given path
 * @param char* pathToPreset  Path to the preset file (library will open and close it)
 * @return char File properties, see the defines
 */
char detectFileProperties(char* pathToPreset);

/** Send a file of the given properties to a location
 * @param char* pathToFile Path to the file to send
 * @param char properties Properties of the file specified
 * @param int location Axe-FX II location to send to (ignored if sending preset)
 * @return char 0 on success, or error code
 */
char sendFile(char* pathToFile, char properties, int location);


/** Get a file of the given properties from a location
 * @param char* pathToSave Path to save the obtained .syx file
 * @param char properties Properties of the unit and file wanted
 * @param int location Axe-FX II location to get from
 * @return char 0 on success, or error code
 */
char getFile(char* pathToSave, char properties, int location);

/** User implemented function to allow the library to provide transaction progress
 * @param int currentProgress the current progress of the library
 */
void progressCallback(int currentProgress);

#endif /* AXEII_LOADER_H */
