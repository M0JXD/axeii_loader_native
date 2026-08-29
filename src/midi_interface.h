/*    midi_interface.h - Stupid simple abstracted MIDI interface
 *    Copyright (C) 2026  Jamie Drinkell
 *    This project is dual licensed under the BSD-2-Clause and GPLv2-or-later
 *    depending on the backend it uses. Please see README.
 *
 ********************************* GPL HEADER *********************************
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
    char type;  /* Axe-FX II Type */
} dev_info_t;

/* FUNCTIONS */

/** Do any MIDI initialisation, only needed for send/get MIDI
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

/** Get a list of MIDI devices, attempting to autodetect Axe-FX II. MIDI does not need initialised.
 * @param amount returns the amount of devices
 * @param axe_index the index of an Axe-FX II MIDI device
 * @return array of midi device info of length "amount"
 */
dev_info_t** getAxeMidiDevs(int *amount, int *axe_index);

/** Free function for getAxeMidiDevs should the OS implementation need it
 * @param devs array of MIDI devices to be freed
 */
void freeAxeMidiDevs(dev_info_t **devs);

/** Detect the unit type
 * @return char The unit type as 'o' (OG), 'x' (XL), 'p' (XL Plus) or 'u' (Unknown)
 */
char detectUnitType(char *devString) {
    /* Yeah yeah code in the header but how else can I do this cleanly? */
    char type = 'u';
    unsigned char response[8];
    unsigned char og_command[8]  = { 0xF0, 0x00, 0x01, 0x74, 0x03, 0x08, 0x0E, 0xF7 };
    unsigned char xl_command[8]  = { 0xF0, 0x00, 0x01, 0x74, 0x06, 0x08, 0x0B, 0xF7 };
    unsigned char xlp_command[8] = { 0xF0, 0x00, 0x01, 0x74, 0x07, 0x08, 0x0A, 0xF7 };
    unsigned char og_disconnect[8]  = { 0xF0, 0x00, 0x01, 0x74, 0x03, 0x42, 0x44, 0xF7 };
    unsigned char xl_disconnect[8]  = { 0xF0, 0x00, 0x01, 0x74, 0x06, 0x42, 0x41, 0xF7 };
    unsigned char xlp_disconnect[8] = { 0xF0, 0x00, 0x01, 0x74, 0x07, 0x42, 0x40, 0xF7 };

    initMIDI(devString);
    /* Send all three commands */
    sendMidi(og_command, 8);
    sendMidi(xl_command, 8);
    sendMidi(xlp_command, 8);

    /* Get which response was obtained, understand the unit type and drop everything else */
    getMidi(response, 8);
    if (response[4] == 0x03) {
        type = 'o';
        sendMidi(og_disconnect, 8);
    } else if (response[4] == 0x06) {
        type = 'x';
        sendMidi(xl_disconnect, 8);
    } else if (response[4] == 0x07) {
        type = 'p';
        sendMidi(xlp_disconnect, 8);
    }
    clearMidiInBuffer();
    closeMIDI();
    return type;
}

#endif
