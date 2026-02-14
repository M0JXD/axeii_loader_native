/*    axeii_loader.h - API header for frontends to send/receive data from an Axe-FX II
 *    Copyright (C) 2025-2026  Jamie Drinkell
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

#include "midi_interface.h"

/* DEFINES */
/* Transfer Properties */
/* From LSB to MSB, the bits represent:
 * 0: True for valid file  (must be set for receive mode to run)
 * 1: True for preset file, false for IR file (used for both send and receive)
 * 2: File is OG/MkII
 * 3: File is XL
 * 4: File is XL+
 * 5: Unit is OG/MkII
 * 6: Unit is XL
 * 7: Unit is XL+
 */
#define OG_IR      0x05  /* 0b00000101 */
#define OG_PRESET  0x07  /* 0b00000111 */
#define XL_IR      0x09  /* 0b00001001 */
#define XL_PRESET  0x0B  /* 0b00001011 */
#define XLP_IR     0x11  /* 0b00010001 */
#define XLP_PRESET 0x13  /* 0b00010011 */

/* MASKS */
#define IS_VALID    0x01  /* 0b00000001 */
#define IS_PRESET   0x02  /* 0b00000010 */  /* If not preset and valid, must be IR */
#define IS_OG_FILE  0x04  /* 0b00000100 */
#define IS_XL_FILE  0x08  /* 0b00001000 */
#define IS_XLP_FILE 0x10  /* 0b00010000 */
#define IS_OG_UNIT  0x20  /* 0b00100000 */
#define IS_XL_UNIT  0x40  /* 0b01000000 */
#define IS_XLP_UNIT 0x80  /* 0b10000000 */
#define CLEAR_FILE  0xE0  /* 0b11100000 */
#define CLEAR_UNIT  0x1F  /* 0b00011111 */
#define SET_IR      0xFD  /* 0b11111101 */

/* ENUMS */
enum errors {
    FILE_ERROR = -1,
    DESTINATION_UNIT_INVALID = -2,
    HEADER_LOCK_ISSUE = -3,
    PROPERTIES_INVALID = -4,
    LOCATION_OOB = -5
};

/* FUNCTION DECLARATIONS */

/** Detects the properties of the file at the given path
 * @param char* pathToPreset  Path to the preset file (library will open and close it)
 * @return char File properties, see the defines
 */
char detectFileProperties(char *pathToPreset);

/** User implemented function to allow the library to provide transaction progress.
 * @param double currentProgress the current progress of the library, from 0.0 to 1.0. Negative numbers are passed for header locking.
 */
void progressCallback(double currentProgress);

/** User implemented function to allow library to provide the name it saved with at save time.
 * @param char* name Name of the file that was just saved
 */
void nameProvider(char *name);

/** Send a file of the given properties to a location
 * @param char* pathToFile Path to the file to send
 * @param unsigned char properties Properties of the file specified
 * @param int location Axe-FX II location to send to (ignored if sending preset)
 * @return char 0 on success, or error code
 */
char sendFile(char *pathToFile, unsigned char properties, int location);

/** Get a file of the given properties from a location. Name provider is called prior to returning and after the last progress call.
 * @param char* pathToSave Path to a directory (which must end in a '/'), or a file name to give the file (overriding the provided one).
 * @param unsigned char properties Properties of the unit and file wanted
 * @param int location Axe-FX II location to get from
 * @return char 0 on success, or error code
 */
char getFile(char *pathToSave, unsigned char properties, int location);


#endif /* AXEII_UTILS_H */
