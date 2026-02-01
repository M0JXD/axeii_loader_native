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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
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
    unsigned int l = 0;
    int ret;
    do {
        ret = read(input, &data[l], len - l);
        if (ret == -1) {
            perror("Read MIDI Error!");
            return -1;
        } else {
            len += ret;
        }
    } while (l != len);
    return 0;
}

char clearMidiInBuffer(void) {
    int ret;
    struct pollfd fds = {
        .fd = input,  /* file descriptor */
        .events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI,   /* requested events */
        .revents = 1, /* returned events */
    };

    while (1) {
        ret = poll(&fds, 1, 2);
        if (ret == 0) {
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

dev_info_t** getAxeMidiDevs(int *amount, int *axe_index) {
    /* Discovering MIDI devices on FreeBSD/OSS seems fundementally broken */
    /* e.g. ossinfo is not even shipped because it won't cover USB devices */

    /* I think the best bet is to query the existence of /dev/umidi* */
    /* and /dev/midi* (incase anyone is using MIDI direct) devices */
    /* and then if they exist, get their metainfo names */

    dev_info_t **devs;
    int fd;
    char devString[14] = "/dev/umidi";
    devs = (dev_info_t**)malloc(sizeof(dev_info_t*) * 5);
    devs[0] = (dev_info_t*)malloc(sizeof(dev_info_t) * 5);
    *amount = 0;
    *axe_index = 0;

    for (int i = 0; i < 3; i++) {
        char buf[4];
        sprintf(buf, "%d.0", i);
        strcat(devString, buf);

        if (access(devString, F_OK) == 0) {
            fd = open(devString, O_RDONLY, 0);
            oss_midi_info mi;
            mi.dev = -1;
            ioctl(fd, SNDCTL_MIDIINFO, &mi);
            strcpy(devs[*amount]->hw_string, devString);
            strcpy(devs[*amount]->hw_name, mi.name);
            close(fd);
            (*amount)++;
        }
        devString[10] = '\0';
    }

    strcpy(devString, "/dev/midi");
    for (int i = 0; i < 2; i++) {
        char buf[4];
        sprintf(buf, "%d", i);
        strcat(devString, buf);

        if (access(devString, F_OK) == 0) {
            fd = open(devString, O_RDONLY, 0);
            oss_midi_info mi;
            mi.dev = -1;
            ioctl(fd, SNDCTL_MIDIINFO, &mi);
            strcpy(devs[*amount]->hw_string, devString);
            strcpy(devs[*amount]->hw_name, devString);
            /*strcpy(devs[*amount]->hw_name, mi.name);*/
            close(fd);
            (*amount)++;
        }
        devString[9] = '\0';
    }

    if (*amount == 0) {
        perror("Could not find any devices!");
    }

    return devs;
}

void freeAxeMidiDevs(dev_info_t **devs) {
    free(devs[0]);
    free(devs);
    devs = NULL;
}


