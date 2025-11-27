/*    axeii_utils.c - Utility functions to send/receive data from an Axe-FX II
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

/*#include <stdio.h>*/
#include <libgen.h>
#include <alsa/asoundlib.h>
#include "axeii_utils.h"

/* GLOBALS */
static snd_rawmidi_t *handleIn, *handleOut;

/* FUNCTIONS */

/* Handy Util */ /*
static void printTenBytes(unsigned char *command) {
    printf("Bytes are: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
            command[0], command[1], command[2], command[3], command[4],
            command[5], command[6], command[7], command[8], command[9]
          );
} */

char setupRawMIDIHandles(char *devString) {
    /* TODO: This call leaks? */
    char err = snd_rawmidi_open(&handleIn, &handleOut, devString, 0);
    if (err != 0) {
        return 1;
    }
    /* Blocking mode */
    snd_rawmidi_nonblock(handleIn, 0);
    /*snd_rawmidi_nonblock(handleOut, 0);*/
    return 0;
}

char closeRawMIDIHandles(void) {
    snd_rawmidi_close(handleIn);
    snd_rawmidi_close(handleOut);
    return 0;
}

char detectFileProperties(char *pathToPreset) {
    char ret = 0;
    unsigned char buffer[6] = { 0, 0, 0, 0, 0, 0 };
    FILE *file = fopen(pathToPreset, "r");
    if (file == NULL) return FILE_ERROR;
    int read = fread(buffer, sizeof(char), 6, file);
    if (read != 6) {
        ret = FILE_ERROR;
    }
    fclose(file);

    if (buffer[5] == 0x7A) {
        /* IR */
        ret = 1;

    } else if (buffer[5] == 0x77) {
        /* Preset */
        ret = 3;
    } else {
        ret = 0;
    }

    switch (buffer[4]) {
        case 0x03:
            ret |= 0x04;  /* 0b00100 */
        break;

        case 0x06:
            ret |= 0x08;  /* 0b01000 */
        break;

        case 0x07:
            ret |= 0x10;  /* 0b10000 */
        break;

        default:
            ret &= 0x02;  /* 0b00010 */
    }
    return ret;
}

/* Internal function to get a timestamp for saving */
static const char* fileTrailingName(void) {
    static char timeStamp[32];
    time_t t = time(NULL);
    strftime(timeStamp, sizeof(timeStamp),
            "_%Hh%Mm-%d-%m-%y.syx", localtime(&t));
    return timeStamp;
}

/* Internal function to get a preset's name from the raw byte data */
static const char* getPresetName(unsigned char *presetData) {
    static char presetName[64];
    /* Name starts at address 1A, skip two bytes, 1D.. until 0x74 */
    for (int i = 0x1A, k = 0; i <= 0x74; i += 0x03, k++) {
        presetName[k] = presetData[i];
    }

    /* Terminate after first non space character from the end */
    for (int i = 30; i > 0; i--) {
        if (presetName[i] != ' ') {
            presetName[i + 1] = '\0';
            break;
        }

        if (i == 1) {
            strcpy(presetName, "UNNAMED");
        }
    }
    presetName[31] = '\0';


    /* Replace all spaces with underscores */
    for (int i = 0; i < (int)strlen(presetName); i++) {
        if (presetName[i] == ' ')
            presetName[i] = '_';
    }

    strcat(presetName, fileTrailingName());
    return presetName;
}

/* Internal function to recalc sysex on the fly */
static void recalcSysex(unsigned char properties, unsigned char *buffer, int len) {
    int checksumByte = 0xF0;
    if (properties & IS_XL_UNIT) {
        buffer[4] = 0x06;
    } else if (properties & IS_XLP_UNIT) {
        buffer[4] = 0x07;
    } else {
        buffer[4] = 0x03;
    }

    /* Add checksum and sysex end byte */
    /* NB: gcc O2 optimisations ruin this calculation */
    for (int i = 1; i < (len - 2); i++) {
        checksumByte ^= buffer[i];
    }
    checksumByte &= 0x7F;
    buffer[len - 2] = checksumByte;
    buffer[len - 1] = 0xF7;
}

/* Internal function to calc the right command to send for getPreset and sendIR */
static void calcReqCommand(unsigned char properties, int location, unsigned char *command, int len) {
    /* HEADER BYTES */
    command[0] = 0xF0;
    command[1] = 0x00;
    command[2] = 0x01;
    command[3] = 0x74;

    if (properties & IS_PRESET) {
        command[5] = 0x03;  /* Patch Dump Req ID */
        /* Banks and preset number */
        if (location < 128) {
            command[6] = 0x00;
            command[7] = location;
        } else if (location < 256) {
            command[6] = 0x01;
            command[7] = location - 128;
        } else if (location < 384) {
            command[6] = 0x02;
            command[7] = location - 256;
        } else if (location < 512) {
            command[6] = 0x03;
            command[7] = location - 384;
        } else if (location < 640) {
            command[6] = 0x04;
            command[7] = location - 512;
        } else if (location < 768) {
            command[6] = 0x05;
            command[7] = location - 640;
        } else {
            /* Catch all, should never happen */
            command[6] = 0x00;
            command[7] = 1;
        }

    } else if (properties & IS_VALID) {
        /* TODO: How does this work for XL/XL+? */
        command[5] = 0x7A;  /* IR Dump Req ID */
        location -= 1;
        if (properties & IS_OG_UNIT) {
            command[6] = location;
            command[7] = 0x0;
            command[8] = 0x10;
        } else {
            if (location < 128) {
                command[6] = 0x00;
                command[7] = location;
            } else if (location < 256) {
                command[6] = 0x01;
                command[7] = location - 128;
            } else if (location < 384) {
                command[6] = 0x02;
                command[7] = location - 256;
            } else if (location < 512) {
                command[6] = 0x03;
                command[7] = location - 384;
            } else if (location < 640) {
                command[6] = 0x04;
                command[7] = location - 512;
            } else if (location < 768) {
                command[6] = 0x05;
                command[7] = location - 640;
            } else if (location < 896) {
                command[6] = 0x06;
                command[7] = location - 768;
            } else if (location < 1024) {
                command[6] = 0x07;
                command[7] = location - 896;
            } else if (location < 1029) {
                /* Scratchpads */
                command[6] = 0x08;
                command[7] = location - 1024;
            } else {
                /* Catch all, should never happen */
                command[6] = 0x08;
                command[7] = 0;
            }
            command[8] = 0x0;
            command[9] = 0x10;
        }
    }
    recalcSysex(properties, command, len);
}

/* Internal function for when fetching from to lock on to the right header bytes */
static char fetchUntilHeaderCorrect(unsigned char *buffer) {
    char ret = HEADER_LOCK_ISSUE;
    int trys = -1;
    buffer[0] = 0;
    do {
        /* Flush any trailing messages */
        while (buffer[0] != 0xF0) {
            snd_rawmidi_read(handleIn, &buffer[0], 1);
        }

        /* Read the next 5 bytes to check it's the right message header */
        snd_rawmidi_read(handleIn, &buffer[1], 1);
        snd_rawmidi_read(handleIn, &buffer[2], 1);
        snd_rawmidi_read(handleIn, &buffer[3], 1);
        snd_rawmidi_read(handleIn, &buffer[4], 1);
        snd_rawmidi_read(handleIn, &buffer[5], 1);
        /* Was handy for debugging, note stdio is not included */
        /*printf("Read header bytes are: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
            buffer[0], buffer[1], buffer[2],
            buffer[3], buffer[4], buffer[5]
        );*/
        if ((buffer[0] == 0xF0) && (buffer[3] == 0x74) &&
            (buffer[5] == 0x77 || buffer[5] == 0x7A)) {
            ret = 0;
            break;
        } else if (buffer[5] == 0x64) {
            /* The utility is really fast, so the response messages are not alway fully dropped from previous runs */
            snd_rawmidi_drop(handleIn);
            buffer[0] = 0;
            continue;
        } else {
            /* Discard wrong packet */
            snd_rawmidi_drop(handleIn);
            /* TODO: Allow tempo to be runnning. Only read more as needed on the next loop */
            /* Maybe another 0xF0 has already been read, move everything over */
            /*for (int nextF0 = 1; nextF0 < 6; nextF0++) {*/
            /*    if (buffer[nextF0] == 0xF0) {*/
            /*        memmove(buffer, &buffer[nextF0], 6 - nextF0);*/
            /*    }*/
            /*}*/
            buffer[0] = 0;
        }
        trys--;
        progressCallback(trys);
    } while (trys > -20);
    return ret;
}

static char sendPreset(char *pathToPreset, unsigned char properties) {
    int read;
    char dataMessages;
    unsigned int endAddress;
    unsigned char buffer[12951];
    FILE *file = fopen(pathToPreset, "r");
    if (file == NULL) return FILE_ERROR;

    if (properties & IS_OG_FILE) {
        dataMessages = 32;
        endAddress = 6476;
        read = fread(buffer, sizeof(char), 6487, file);
    } else {
        dataMessages = 64;
        endAddress = 12940;
        read = fread(buffer, sizeof(char), 12951, file);
    }
    fclose(file);

    if (read < 6487) {
        return FILE_ERROR;
    }

    /* Refuse to send XL presets to OG and vice versa */
    if (((properties & IS_OG_UNIT) && (endAddress > 6488)) ||
        (!(properties & IS_OG_UNIT) && (endAddress < 12930))) {
        return DESTINATION_UNIT_INVALID;
    }

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Send start message */
    progressCallback(0);
    recalcSysex(properties, buffer, 12);
    snd_rawmidi_write(handleOut, buffer, 12);
    snd_rawmidi_drop(handleIn);

    /* Send data messages */
    for (int i = 0; i < dataMessages; i++) {
        recalcSysex(properties, &buffer[12+(202 * i)], 202);
        snd_rawmidi_write(handleOut, &buffer[12+(202 * i)], 202);
        snd_rawmidi_drop(handleIn);
        progressCallback((100 / (dataMessages + 2)) *  i + 2);
    }

    /* Send end message */
    recalcSysex(properties,  &buffer[endAddress], 11);
    snd_rawmidi_write(handleOut, &buffer[endAddress], 11);
    snd_rawmidi_drop(handleIn);
    progressCallback(100);
    return 0;
}

static char getPreset(char *pathToSave, unsigned char properties, int location) {
    char ret;
    unsigned char command[10];
    unsigned char buffer[12951];
    unsigned int readBackAmount;
    calcReqCommand(properties, location, command, 10);

    if (properties & IS_OG_UNIT) {
        readBackAmount = 6487;
    } else {
        readBackAmount = 12951;
    }

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Request a preset dump */
    snd_rawmidi_write(handleOut, command, 10);
    ret = fetchUntilHeaderCorrect(buffer);

    if (ret == 0) {
        progressCallback(0);
        /* Grab everything else... */
        for (unsigned int i = 6; i < readBackAmount; i++) {
            snd_rawmidi_read(handleIn, &buffer[i], 1);
            double prog = ((double)i / (double)readBackAmount) * 100;
            if (prog > 1)
                progressCallback((int)prog);
        }
        progressCallback(100);

        /* Save the preset */
        if (pathToSave[strlen(pathToSave) - 1] == '/') {
            const char *name = getPresetName(buffer);
            strcat(pathToSave, name);
        }
        FILE *file = fopen(pathToSave, "wb");
        if (file == NULL) return FILE_ERROR;
        fwrite(buffer, sizeof(unsigned char), readBackAmount, file);
        fclose(file);
        nameProvider(basename(pathToSave));
    }
    snd_rawmidi_drop(handleIn);
    return ret;
}

static char sendIR(char *pathToIR, unsigned char properties, int location) {
    char irInfoStart;
    unsigned int endAddress;
    unsigned char buffer[10905];
    int read;
    FILE *file = fopen(pathToIR, "r");
    if (file == NULL) return FILE_ERROR;

    if (properties & IS_OG_FILE) {
        irInfoStart = 11;
        endAddress = 10891;
        read = fread(buffer, sizeof(char), 10904, file);
    } else {
        irInfoStart = 12;
        endAddress = 10892;
        read = fread(buffer, sizeof(char), 10905, file);
    }
    fclose(file);
    if (read < 10904) {
        return FILE_ERROR;
    }

    char startLen = (properties & IS_OG_UNIT) ? 11 : 12;
    unsigned char command[12];
    calcReqCommand(properties, location, command, startLen);

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Inform we're sending an IR dump */
    snd_rawmidi_write(handleOut, command, startLen);
    snd_rawmidi_drop(handleIn);

    progressCallback(0);
    /* Send data messages */
    for (int i = 0; i < 64; i++) {
        recalcSysex(properties, &buffer[irInfoStart+(170 * i)], 170);
        snd_rawmidi_write(handleOut, &buffer[irInfoStart+(170 * i)], 170);
        snd_rawmidi_drop(handleIn);
        double prog = ((double)i / (double)66.0) * 100;
        if (prog > 1)
            progressCallback((int)prog);
    }

    /* Send end message */
    recalcSysex(properties,  &buffer[endAddress], 13);
    snd_rawmidi_write(handleOut, &buffer[endAddress], 13);
    snd_rawmidi_drop(handleIn);
    progressCallback(100);
    return 0;
}

static char getIR(char *pathToSave, unsigned char properties, int location) {
    char ret;
    unsigned char command[9] = { 0xF0, 0x00, 0x01, 0x74, 0x03, 0x19, 0x00, 0x1F, 0xF7 };
    unsigned char buffer[10905];
    const int lengthOfFile = properties & IS_OG_UNIT ? 10904 : 10905;
    command[6] = location - 1;
    recalcSysex(properties, command, 9);

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);
    snd_rawmidi_drop(handleOut);

    /* Request a IR dump */
    snd_rawmidi_write(handleOut, command, 9);

    ret = fetchUntilHeaderCorrect(buffer);

    if (ret == 0) {
        progressCallback(0);
        /* Grab everything else... */
        for (int i = 6; i < lengthOfFile; i++) {
            snd_rawmidi_read(handleIn, &buffer[i], 1);
            double prog = ((double)i / (double)lengthOfFile) * 100;
            if (prog > 1)
                progressCallback((int)prog);

        }
        progressCallback(100);

        /* Save the IR */
        if (pathToSave[strlen(pathToSave) - 1] == '/') {
            char name[64];
            sprintf(name, "IR%d%s", location, fileTrailingName());
            strcat(pathToSave, name);
        }

        FILE *file = fopen(pathToSave, "wb");
        if (file == NULL) return FILE_ERROR;
        fwrite(buffer, sizeof(unsigned char), lengthOfFile, file);
        fclose(file);
        nameProvider(basename(pathToSave));
    }
    snd_rawmidi_drop(handleIn);
    return ret;
}

static char checkLocationValid(unsigned char properties, int location) {
    char ret = 0;

    if (properties & IS_PRESET) {
        if (properties & IS_OG_UNIT) {
            ret = location < 0 || location > 383 ? LOCATION_OOB : 0;
        } else {
            ret = location < 0 || location > 767 ? LOCATION_OOB : 0;
        }
    } else if (properties & IS_VALID) {
        if (properties & IS_OG_UNIT) {
            ret = location < 1 || location > 104 ? LOCATION_OOB : 0;
        } else {
            ret = location < 1 || location > 1028 ? LOCATION_OOB : 0;
        }
    }
    return ret;
}

char sendFile(char *pathToFile, unsigned char properties, int location) {
    char ret = checkLocationValid(properties, location);
    if (ret != 0) return ret;
    if (properties & IS_PRESET) {
        (void)location;
        ret = sendPreset(pathToFile, properties);
    } else if (properties & IS_VALID) {
        ret = sendIR(pathToFile, properties, location);
    } else {
        ret = PROPERTIES_INVALID;
    }
    snd_rawmidi_drop(handleIn);
    return ret;
}

char getFile(char *pathToSave, unsigned char properties, int location) {
    char ret = checkLocationValid(properties, location);
    if (ret != 0) return ret;
    if (properties & IS_PRESET) {
        ret = getPreset(pathToSave, properties, location);
    } else if (properties & IS_VALID) {
        ret = getIR(pathToSave, properties, location);
    } else {
        ret = PROPERTIES_INVALID;
    }
    snd_rawmidi_drop(handleIn);
    return ret;
}
