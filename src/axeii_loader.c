/* Copyright 2025 Jamie Drinkell. MIT License. */

/* A simple utility to load presets and IRs in/out of an Axe-FX II
 * Only tested on Linux Mint 22.2 with an Axe-FX II MkII
 * Unsure if it will work for XL/XL+ units.
 * NO WARRANTY IS PROVIDED, USE AT YOUR OWN RISK.
 */

#include <stdio.h>
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
        /*puts("Error reading the file.");*/
        ret = -1;
    }
    fclose(file);

    if (buffer[5] == 0x7A) {
        /* IR */
        /*puts("IR File Detected.");*/
        ret = 1;

    } else if (buffer[5] == 0x77) {
        /* Preset */
        /*puts("Preset File Detected.");*/
        ret = 3;
    } else {
        /*puts("Could not detect file type.");*/
        ret = 0;
    }

    switch (buffer[4]) {
        case 0x03:
            ret |= 0b00100;
            /*puts("File is for/from Axe-Fx II Original/MkII.");*/
        break;

        case 0x06:
            ret |= 0b01000;
            /*puts("File is for/from Axe-Fx II XL.");*/
        break;

        case 0x07:
            ret |= 0b10000;
            /*puts("File is for/from Axe-Fx II XL+.");*/
        break;

        default:
            /*puts("Could not detect unit type.");*/
            ret &= 0b00010;
    }
    return ret;
}

/* Internal function to recalc sysex on the fly */
static void recalcSysex(char unitType, unsigned char* buffer, int len) {
    int checksumByte = 0xF0;
    switch (unitType) {
        case XL:
            buffer[4] = 0x06;
        break;

        case XLP:
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
static void calcReqCommand(int location, char type, char unitType,
                           unsigned char* command, int len) {
    /* HEADER BYTES */
    command[0] = 0xF0;
    command[1] = 0x00;
    command[2] = 0x01;
    command[3] = 0x74;

    /* TODO: How does all this work for XL/XL+? */
    if (type == IR) {
        command[5] = 0x7A;  /* IR Dump Req ID */
        command[6] = location - 1;
        command[7] = 0x0;
        command[8] = 0x10;
    } else if (type == PRESET) {
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
    recalcSysex(unitType, command, len);
}

char sendPreset(char* pathToPreset, char unitType, char fileUnit) {
    int read;
    char dataMessages;
    unsigned int endAddress;
    unsigned char buffer[12951];
    FILE * file = fopen(pathToPreset, "r");
    if (fileUnit == OG) {
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
        puts("Error reading the file.");
        return -1;
    }

    /* Refuse to send XL presets to OG */
    if (unitType == OG && endAddress == 12940) {
        puts("Can't send XL/XL+ presets to OG/MKII Axe-FX II");
        return -1;
    }

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);
    printf("Sending preset %s to edit buffer...\n", pathToPreset);

    /* Send start message */
    printf("Progress: 0%% ...");
    fflush(stdout);
    recalcSysex(unitType, buffer, 12);
    snd_rawmidi_write(handleOut, buffer, 12);
    snd_rawmidi_drop(handleIn);

    /* Send data messages */
    for (int i = 0; i < dataMessages; i++) {
        recalcSysex(unitType, &buffer[12+(202 * i)], 202);
        snd_rawmidi_write(handleOut, &buffer[12+(202 * i)], 202);
        snd_rawmidi_drop(handleIn);
        progressCallback(i);
        /*printf(".");*/
        /*fflush(stdout);*/
    }

    /* Send end message */
    recalcSysex(unitType,  &buffer[endAddress], 11);
    snd_rawmidi_write(handleOut, &buffer[endAddress], 11);
    snd_rawmidi_drop(handleIn);
    /*printf(" 100%%\n");*/
    /*puts("Preset sent to Axe-FX II.");*/
    return 0;
}

char getPreset(int location, char* pathToSave, char unitType) {
    unsigned char command[10];
    unsigned char buffer[12951];
    unsigned int readBackAmount;
    calcReqCommand(location, PRESET, unitType, command, 10);

    if (unitType != OG) {
        readBackAmount = 12951;
    } else {
        readBackAmount = 6487;
    }

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Request a preset dump */
    printf("Getting a preset from location %d...\n", location);
    snd_rawmidi_write(handleOut, command, 10);

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
        printf("Read header bytes are: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
            buffer[0], buffer[1], buffer[2],
            buffer[3], buffer[4], buffer[5]
        );

        if ((buffer[0] == 0xF0) && (buffer[3] == 0x74) && buffer[5] == 0x77) {
            puts("Correct header detected.");
            break;
        } else {
            /* Discard wrong packet */
            puts("Invalid packet, trying again...\n");
        }
    } while (1);

    printf("Progress: 0%% ...");
    fflush(stdout);
    /* Grab everything else... */
    for (unsigned int i = 6; i < readBackAmount; i++) {
        snd_rawmidi_read(handleIn, &buffer[i], 1);
        if ((i % 100) == 0) {
            progressCallback(i);
            /*printf(".");*/
            /*fflush(stdout);*/
        }
    }
    printf(" 100%%\n");

    /* Save the preset */
    FILE * file = fopen(pathToSave, "wb");
    fwrite(buffer, sizeof(unsigned char), 6487, file);
    fclose(file);
    puts("Got preset from Axe-FX II.");
    return 0;
}

char sendIR(int location, char* pathToIR, char unitType, char fileUnit) {
    char irInfoStart;
    unsigned int endAddress;
    unsigned char buffer[10905];
    int read;
    FILE *file = fopen(pathToIR, "r");

    if (fileUnit == OG) {
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
        puts("Error reading the file.");
        return -1;
    }

    unsigned char command[12];
    calcReqCommand(location, IR, unitType, command, 11);

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Inform we're sending an IR dump */
    printf("Sending IR %s to location %d...\n", pathToIR, location);
    printf("Progress: 0%% ...");
    fflush(stdout);
    snd_rawmidi_write(handleOut, command, 11);
    snd_rawmidi_drop(handleIn);

    /* Send data messages */
    for (int i = 0; i < 64; i++) {
        recalcSysex(unitType, &buffer[irInfoStart+(170 * i)], 170);
        snd_rawmidi_write(handleOut, &buffer[irInfoStart+(170 * i)], 170);
        snd_rawmidi_drop(handleIn);
        /*printf(".");*/
        /*fflush(stdout);*/
        progressCallback(i);
    }

    /* Send end message */
    recalcSysex(unitType,  &buffer[endAddress], 13);
    snd_rawmidi_write(handleOut, &buffer[endAddress], 13);
    snd_rawmidi_drop(handleIn);
    printf(" 100%%\n");
    puts("IR sent to Axe-FX II.");
    return 0;
}

char getIR(int location, char* pathToSave, char unitType) {
    unsigned char command[9] = { 0xF0, 0x00, 0x01, 0x74, 0x03, 0x19, 0x00, 0x1F, 0xF7 };
    unsigned char buffer[10905];
    const int lengthOfFile = unitType == OG ? 10904 : 10905;
    command[6] = location - 1;
    recalcSysex(unitType, command, 9);

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    snd_rawmidi_drop(handleIn);

    /* Request a preset dump */
    printf("Getting an IR from location %d...\n", location);
    snd_rawmidi_write(handleOut, command, 9);

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
        printf("Read header bytes are: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
            buffer[0], buffer[1], buffer[2],
            buffer[3], buffer[4], buffer[5]
        );

        if ((buffer[0] == 0xF0) && (buffer[3] == 0x74) && buffer[5] == 0x7A) {
            puts("Correct header detected.");
            break;
        } else {
            /* Discard wrong packet */
            puts("Invalid packet, trying again...\n");
            snd_rawmidi_drop(handleIn);
        }
    } while (1);

    printf("Progress: 0%% ...");
    fflush(stdout);
    /* Grab everything else... */
    for (int i = 6; i < lengthOfFile; i++) {
        snd_rawmidi_read(handleIn, &buffer[i], 1);
        if ((i % 200) == 0) {
            progressCallback(i);
        }
    }
    printf(" 100%%\n");

    /* Save the IR */
    FILE * file = fopen(pathToSave, "wb");
    if (unitType == OG) {
        fwrite(buffer, sizeof(unsigned char), 10904, file);
    } else {
        fwrite(buffer, sizeof(unsigned char), 10905, file);
    }
    fclose(file);
    puts("Got IR from Axe-FX II.");
    return 0;
}

char sendFile() {

}

char getFile() {

}
