/*    midi_interface.h - Stupid simple abstracted MIDI interface
 *    Copyright (C) 2026  Jamie Drinkell
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

#ifndef MIDI_INTERFACE_H
#define MIDI_INTERFACE_H

/* A MIDI backend is expected to implement these functions */

/* STRUCTS */
typedef struct DEV_INFO_S {
    char hw_string[32];
    char hw_name[120];
} dev_info_t;

/* FUNCTIONS */

/** Do any MIDI initialisation
 * @param char* string of desired MIDI device
 * @return char 0 on success, or error code
 */
char initMIDI(char *devString);

/** Close down MIDI initialised by initMIDI()
 * @return char 0 on success, or error code
 */
char closeMIDI(void);

/** Send precalculated bytes to the opened device
 * @param data buffer containing bytes to send
 * @param len length of the buffer (amount to send)
 * @return char 0 on success, or error code
 */
char sendMidi(unsigned char *data, unsigned int len);

/** Get an amount of raw MIDI bytes
 * @param data buffer to receive bytes into
 * @param len length of the buffer (amount to get)
 * @return char 0 on success, or error code
 */
char getMidi(unsigned char *data, unsigned int len);

/** Clear the input buffer (i.e. drop anything it might contain)
 * @return char 0 on success, or error code
 */
char clearMidiInBuffer(void);

/** Clear the output buffer (i.e. force everything to sync)
 * @return char 0 on success, or error code
 */
char clearMidiOutBuffer(void);

/** Utility to get a list of MIDI devices, which attempts to detect which one is an Axe-FX II
 * @param amount returns the amount of devices
 * @param axe_index the index of an Axe-FX II MIDI device
 * @return array of midi device info of length "amount"
 */
dev_info_t** getAxeMidiDevs(int *amount, int *axe_index);

/** Free function for getAxeMidiDevs should the OS implementation need it
 * @param devs array of MIDI devices to be freed
 */
void freeAxeMidiDevs(dev_info_t **devs);

#endif