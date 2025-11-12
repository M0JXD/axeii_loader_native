/* Copyright 2025 Jamie Drinkell. MIT License. */

/* A simple utility to load presets and IRs in/out of an Axe-FX II
 * Only tested on Linux Mint 22.2 with an Axe-FX II MkII
 * Unsure if it will work for XL/XL+ units.
 * NO WARRANTY IS PROVIDED, USE AT YOUR OWN RISK.
 */

#include <alsa/asoundlib.h>
#include "axeii_loader.h"

/* GLOBALS */
static snd_rawmidi_t *handleIn = 0, *handleOut = 0;

/* FUNCTIONS */
char setupRawMIDIHandles(char* devString) {
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

char detectFileProperties(char* pathToPreset) {
    char ret = 0;
    unsigned char buffer[6] = { 0, 0, 0, 0, 0, 0 };
    FILE *file = fopen(pathToPreset, "r");
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
            ret |= 0b00100;
        break;

        case 0x06:
            ret |= 0b01000;
        break;

        case 0x07:
            ret |= 0b10000;
        break;

        default:
            ret &= 0b00010;
    }
    return ret;
}

/* Internal function to recalc sysex on the fly */
static void recalcSysex(char properties, unsigned char* buffer, int len) {
    int checksumByte = 0xF0;

    /* We only care for the type */
    properties &= 0x1C;  /* 0b11100 */
    switch (properties) {
        case IS_XL:
            buffer[4] = 0x06;
        break;

        case IS_XLP:
            buffer[4] = 0x07;
        break;

        default:
            buffer[4] = 0x03;
        break;
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

/* Internal function to calc the right command to send */
static void calcReqCommand(char properties, int location, char* command, int len) {
    /* HEADER BYTES */
    command[0] = 0xF0;
    command[1] = 0x00;
    command[2] = 0x01;
    command[3] = 0x74;

    /* TODO: How does all this work for XL/XL+? */
    if (properties & IS_IR) {
        command[5] = 0x7A;  /* IR Dump Req ID */
        command[6] = location - 1;
        command[7] = 0x0;
        command[8] = 0x10;
    } else if (properties & IS_PRESET) {
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
        }
    }
    recalcSysex(properties, command, len);
}

/* Internal function for when fetching from to lock on to the right header bytes */
static char fetchUntilHeaderCorrect(char* buffer, char properties) {
    char ret = HEADER_LOCK_ISSUE;
    char trys = 0;

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
        /*printf("Read header bytes are: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",*/
        /*    buffer[0], buffer[1], buffer[2],*/
        /*    buffer[3], buffer[4], buffer[5]*/
        /*);*/

        if ((buffer[0] == 0xF0) && (buffer[3] == 0x74) && buffer[5] == 0x7A) {
            ret = 0;
            break;
        } else {
            /* Discard wrong packet */
            snd_rawmidi_drop(handleIn);
            /* TODO: Make this work and only read more as needed on the next loop */
            /* Maybe another 0xF0 has already been read, move everything over */
            for (int nextF0 = 1; nextF0 < 6; nextF0++) {
                if (buffer[nextF0] == 0xF0) {
                    memmove(buffer, &buffer[nextF0], 6 - nextF0);
                }
            }
            buffer[0] = 10;
        }
        trys--;
        progressCallback(trys);
    } while (trys > -10);
    return ret;
}

static char sendPreset(char* pathToPreset, char properties) {
    int read;
    char dataMessages;
    unsigned int endAddress;
    unsigned char buffer[12951];
    FILE * file = fopen(pathToPreset, "r");
    if (properties & IS_OG) {
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

    /* Refuse to send XL presets to OG */
    if ((properties & IS_OG) && endAddress == 12940) {
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
        progressCallback((100 / dataMessages + 2) *  i);
    }

    /* Send end message */
    recalcSysex(properties,  &buffer[endAddress], 11);
    snd_rawmidi_write(handleOut, &buffer[endAddress], 11);
    snd_rawmidi_drop(handleIn);
    progressCallback(100);
    return 0;
}

static char getPreset(char* pathToSave, char properties, int location) {
    char ret;
    unsigned char command[10];
    unsigned char buffer[12951];
    unsigned int readBackAmount;
    calcReqCommand(properties, location, command, 10);

    if (properties & IS_OG) {
        readBackAmount = 12951;
    } else {
        readBackAmount = 6487;
    }

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Request a preset dump */
    snd_rawmidi_write(handleOut, command, 10);

    ret = fetchUntilHeaderCorrect(buffer, properties);

    if (ret == 0) {
        progressCallback(0);
        /* Grab everything else... */
        for (unsigned int i = 6; i < readBackAmount; i++) {
            snd_rawmidi_read(handleIn, &buffer[i], 1);
            if ((i % 100) == 0) {
                progressCallback((100 / readBackAmount + 1) *  i);
            }
        }
        progressCallback(100);

        /* Save the preset */
        FILE * file = fopen(pathToSave, "wb");
        fwrite(buffer, sizeof(unsigned char), 6487, file);
        fclose(file);
    }
    return ret;
}

static char sendIR(char* pathToIR, char properties, int location) {
    char irInfoStart;
    unsigned int endAddress;
    unsigned char buffer[10905];
    int read;
    FILE *file = fopen(pathToIR, "r");

    if (properties & IS_OG) {
        irInfoStart = 11;
        endAddress = 10891;
        read = fread(buffer, sizeof(char), 10904, file);
    } else {
        irInfoStart = 12;
        endAddress = 10892;
        read = fread(buffer, sizeof(char), 10904, file);
    }
    fclose(file);
    if (read < 10904) {
        return FILE_ERROR;
    }

    unsigned char command[12];
    calcReqCommand(properties, location, command, 11);

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Inform we're sending an IR dump */
    progressCallback(0);
    snd_rawmidi_write(handleOut, command, 11);
    snd_rawmidi_drop(handleIn);

    /* Send data messages */
    for (int i = 0; i < 64; i++) {
        recalcSysex(properties, &buffer[irInfoStart+(170 * i)], 170);
        snd_rawmidi_write(handleOut, &buffer[irInfoStart+(170 * i)], 170);
        snd_rawmidi_drop(handleIn);
        progressCallback((100 / 66) * i);
    }

    /* Send end message */
    recalcSysex(properties,  &buffer[endAddress], 13);
    snd_rawmidi_write(handleOut, &buffer[endAddress], 13);
    snd_rawmidi_drop(handleIn);
    progressCallback(100);
    return 0;
}

static char getIR(char* pathToSave, char properties, int location) {
    char ret;
    unsigned char command[9] = { 0xF0, 0x00, 0x01, 0x74, 0x03, 0x19, 0x00, 0x1F, 0xF7 };
    unsigned char buffer[10905];
    const int lengthOfFile = properties & IS_OG ? 10904 : 10905;
    command[6] = location - 1;
    recalcSysex(properties, command, 9);

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Request a preset dump */
    snd_rawmidi_write(handleOut, command, 9);

    ret = fetchUntilHeaderCorrect(buffer, properties);

    if (ret == 0) {
        progressCallback(0);
        /* Grab everything else... */
        for (int i = 6; i < lengthOfFile; i++) {
            snd_rawmidi_read(handleIn, &buffer[i], 1);
            if ((i % 200) == 0) {
                progressCallback((100 / lengthOfFile) * i);
            }
        }
        progressCallback(100);

        /* Save the IR */
        FILE * file = fopen(pathToSave, "wb");
        if (properties & IS_OG) {
            fwrite(buffer, sizeof(unsigned char), 10904, file);
        } else {
            fwrite(buffer, sizeof(unsigned char), 10905, file);
        }
        fclose(file);
    }
    return ret;
}

char sendFile(char* pathToFile, char properties, int location) {
    char ret = 0;
    if (properties & IS_PRESET) {
        (void)location;
        ret = sendPreset(pathToFile, properties);
    } else if (properties & IS_IR) {
        ret = sendIR(pathToFile, properties, location);
    } else {
        ret = PROPERTIES_INVALID;
    }
    return ret;
}

char getFile(char* pathToSave, char properties, int location) {
    char ret = 0;
    if (properties & IS_PRESET) {
        ret = getPreset(pathToSave, properties, location);
    } else if (properties & IS_IR) {
        ret = getIR(pathToSave, properties, location);
    } else {
        ret = PROPERTIES_INVALID;
    }
    return ret;
}
