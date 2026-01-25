/*    axeii_oss.c - OSS backend for AxeII-Loader
 *    Copyright (C) 2026 Jamie Drinkell
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

#include "../midi_interface.h"

/* FUNCTIONS */

char initMIDI(char *devString) {
    return 0;
}

char closeMIDI(void) {
    return 0;
}

char sendMidi(unsigned char *data, unsigned int len) {
    return 0;
}

char getMidi(unsigned char *data, unsigned int len) {
    return 0;
}

char clearMidiInBuffer(void) {
    return 0;
}

char clearMidiOutBuffer(void) {
    return 0;
}

dev_info_t** getAxeMidiDevs(int *amount, int *axe_index)
{
    dev_info_t **devs;

    return devs;
}

void freeAxeMidiDevs(dev_info_t **devs)
{
    free(devs[0]);
    free(devs);
    devs = NULL;
}


