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

#include <stdio.h>
/*#include <fcntl.h>*/
#include <sys/soundcard.h>

#include "../midi_interface.h"

/* GLOBAL */

static int input, output;

/* FUNCTIONS */

char initMIDI(char *devString) {
    if ((input = open(devString, O_RDONLY, 0)) == -1) {
        perror("Open Input MIDI Error!");
        return -1;
    }

    if ((output = open(devString, O_WRONLY, 0)) == -1) {
        perror("Open Output MIDI Error!");
        return -1;
    }
    return 0;
}

char closeMIDI(void) {
    close(input);
    close(output);
    return 0;
}

char sendMidi(unsigned char *data, unsigned int len) {
    if (write(output, data, len) == -1) {
        perror("Send MIDI Error!");
        return -1;
    }

    return 0;
}

char getMidi(unsigned char *data, unsigned int len) {
    if (read(input, data, len) == -1) {
        perror("Read MIDI Error!");
        return -1;
    }
    return 0;
}

char clearMidiInBuffer(void) {
    int ret;
    struct pollfd fds = {
        fd = input;  /* file descriptor */
        events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI;   /* requested events */
        revents = 1; /* returned events */
    };

    while (1) {
        ret = poll(&fds, 1, 2);
        if (ret = 0) {
            /* Timed out */
            break;
        } else if (ret > 0) {
            /* Throwaway until it does timeout */
            char throwaway;
            read(input, &throwaway, 1);
        }
    }
    return 0;
}

char clearMidiOutBuffer(void) {
    /* OSS buffers everything and it will sync eventually, nothing to do */
    return 0;
}

dev_info_t** getAxeMidiDevs(int *amount, int *axe_index)
{
    /* Do I use /dev/sndstat or oss sysinfo? */



    oss_sysinfo sysinfo;

    dev_info_t **devs;

    return devs;
}

void freeAxeMidiDevs(dev_info_t **devs)
{
    /*free(devs[0]);*/
    /*free(devs);*/
    devs = NULL;
}


