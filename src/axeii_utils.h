/*    axeii_utils.h - Defines and declarations to send/receive data from an Axe-FX II
 *    Copyright (C) 2025  Jamie Drinkell
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef AXEII_UTILS_H
#define AXEII_UTILS_H

/* DEFINES */
/* File Properties */

/* From LSB to MSB, the bits represent:
 * 0: True for valid file
 * 1: True for preset file, false for IR file
 * 2: Is OG/MkII file
 * 3: Is XL file
 * 4: Is XL+ file
 */
#define OG_IR      0x05  /* 0b00101 */
#define OG_PRESET  0x07  /* 0b00111 */
#define XL_IR      0x09  /* 0b01001 */
#define XL_PRESET  0x0B  /* 0b01011 */
#define XLP_IR     0x11  /* 0b10001 */
#define XLP_PRESET 0x13  /* 0b10011 */

/* MASKS */
#define IS_VALID   0x01  /* 0b00001 */
#define IS_IR      0x01  /* 0b00001 */  /* Same as valid check */
#define IS_PRESET  0x02  /* 0b00010 */
#define IS_OG      0x04  /* 0b00100 */
#define IS_XL      0x08  /* 0b01000 */
#define IS_XLP     0x10  /* 0b10000 */

/* ENUMS */
enum errors {
    FILE_ERROR = -1,
    DESTINATION_UNIT_INVALID = -2,
    HEADER_LOCK_ISSUE = -3,
    PROPERTIES_INVALID = -4
};

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
 * @param int currentProgress the current progress of the library, from 0 to 100. Negative numbers are passed for header locking.
 */
void progressCallback(int currentProgress);

#endif /* AXEII_UTILS_H */
