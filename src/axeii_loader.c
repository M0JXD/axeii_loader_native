/*    axeii_loader.c - agnostic implementation send/receive data from an Axe-FX II
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include "axeii_loader.h"

#define OG_PT_SIZE  6487   /* Size in bytes of OG Preset */
#define XL_PT_SIZE  12951  /* Size in bytes of XL Preset */
#define PT_START    12     /* Size in bytes of Preset Start Sysex */
#define PT_DATA     202    /* Size in bytes of Preset Data Sysex */
#define PT_END      11     /* Size in bytes of Preset End Sysex */
#define OG_PT_MSG   32     /* Number of Data Sysex Packets in OG Preset */
#define XL_PT_MSG   64     /* Number of Data Sysex Packets in XL Preset */

#define OG_IR_SIZE  10904  /* Size in bytes of OG IR */
#define XL_IR_SIZE  10905  /* Size in bytes of XL IR */
#define OG_IR_START 11     /* Size in bytes of OG IR Start Sysex */
#define XL_IR_START 12     /* Size in bytes of XL IR Start Sysex */
#define IR_DATA     170    /* Size in bytes of IR Data Sysex */
#define IR_END      13     /* Size in bytes of IR End Sysex */
#define IR_MSG      64     /* Number of Data Sysex Packets in IR */

/* Handy Util */ /*
static void printTenBytes(unsigned char *command) {
    printf("Bytes are: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
            command[0], command[1], command[2], command[3], command[4],
            command[5], command[6], command[7], command[8], command[9]
          );
} */

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

/* Internal function for when fetching from to lock on to the right header bytes */
static char fetchUntilHeaderCorrect(unsigned char *buffer, int length) {
    char ret = HEADER_LOCK_ISSUE;
    int trys = -1;
    buffer[0] = 0;
    do {
        /* Flush any trailing messages */
        while (buffer[0] != 0xF0) {
            getMidi(&buffer[0], 1);
        }
        /* Read the next 5 bytes to check it's the right message header */
        getMidi(&buffer[1], 5);
        if ((buffer[0] == 0xF0) && (buffer[3] == 0x74) &&
            (buffer[5] == 0x77 || buffer[5] == 0x7A)) {
            getMidi(&buffer[6], length - 6);  /* Get the remaining sysex packet */
            ret = 0;
            break;
        } else if (buffer[5] == 0x64) {
            /* The utility is really fast, so the response messages are not always fully dropped from previous runs */
            clearMidiInBuffer();
            buffer[0] = 0;
        } else {
            /* Discard wrong packet */
            clearMidiInBuffer();
            buffer[0] = 0;
            trys--;
            progressCallback(trys);
        }
    } while (trys > -20);
    return ret;
}

static char sendPreset(char *pathToPreset, unsigned char properties) {
    int read;
    char dataMessages;
    unsigned int endAddress;
    unsigned char buffer[XL_PT_SIZE];

    FILE *file = fopen(pathToPreset, "r");
    if (file == NULL) return FILE_ERROR;

    if (properties & IS_OG_FILE) {
        dataMessages = OG_PT_MSG;
        endAddress = OG_PT_SIZE - PT_END;
        read = fread(buffer, sizeof(char), OG_PT_SIZE, file);
    } else {
        dataMessages = XL_PT_MSG;
        endAddress = XL_PT_SIZE - IR_END;
        read = fread(buffer, sizeof(char), XL_PT_SIZE, file);
    }
    fclose(file);

    if (read < OG_PT_SIZE) {
        return FILE_ERROR;
    }

    /* Refuse to send XL presets to OG and vice versa */
    if (((properties & IS_OG_UNIT) && (endAddress > 6488)) ||
        (!(properties & IS_OG_UNIT) && (endAddress < 12930))) {
        return DESTINATION_UNIT_INVALID;
    }

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    clearMidiInBuffer();

    /* Send start message */
    progressCallback(0.0);
    recalcSysex(properties, buffer, PT_START);
    sendMidi(buffer, PT_START);
    clearMidiInBuffer();

    /* Send data messages */
    for (int i = 0; i < dataMessages; i++) {
        recalcSysex(properties, &buffer[PT_START+(PT_DATA * i)], PT_DATA);
        sendMidi(&buffer[PT_START+(PT_DATA * i)], PT_DATA);
        clearMidiInBuffer();
        progressCallback(((double)i + 1) / ((double)dataMessages + 2));
    }

    /* Send end message */
    recalcSysex(properties,  &buffer[endAddress], PT_END);
    sendMidi(&buffer[endAddress], PT_END);
    clearMidiInBuffer();
    progressCallback(1.0);
    return 0;
}

static char sendIR(char *pathToIR, unsigned char properties, int location) {
    int read;
    char startLength;
    unsigned int endAddress;
    unsigned char buffer[XL_IR_SIZE];

    FILE *file = fopen(pathToIR, "r");
    if (file == NULL) return FILE_ERROR;

    if (properties & IS_OG_FILE) {
        startLength = OG_IR_START;
        endAddress = OG_IR_SIZE - IR_END;
        read = fread(buffer, sizeof(char), OG_IR_SIZE, file);
    } else {
        startLength = XL_IR_START;
        endAddress = XL_IR_SIZE - IR_END;
        read = fread(buffer, sizeof(char), XL_IR_SIZE, file);
    }
    fclose(file);

    if (read < OG_IR_SIZE) {
        return FILE_ERROR;
    }

    unsigned char command[XL_IR_START];
    /* HEADER BYTES */
    command[0] = 0xF0;
    command[1] = 0x00;
    command[2] = 0x01;
    command[3] = 0x74;
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
    recalcSysex(properties, command, startLength);

    /* Axe-FX II sends midi tempo ticks. */
    /* Incase the buffer has them, force it to clear */
    clearMidiInBuffer();

    /* Inform we're sending an IR dump */
    sendMidi(command, startLength);
    clearMidiInBuffer();

    progressCallback(0.0);
    /* Send data messages */
    for (int i = 0; i < IR_MSG; i++) {
        recalcSysex(properties, &buffer[startLength+(IR_DATA * i)], IR_DATA);
        sendMidi(&buffer[startLength + (IR_DATA * i)], IR_DATA);
        clearMidiInBuffer();
        progressCallback(((double)i + 1) / ((double)IR_MSG + 2));
    }

    /* Send end message */
    recalcSysex(properties, &buffer[endAddress], IR_END);
    sendMidi(&buffer[endAddress], IR_END);
    clearMidiInBuffer();
    progressCallback(1.0);
    return 0;
}

static char getPreset(char *pathToSave, unsigned char properties, int location) {
    char ret;
    const int lengthOfFile = (properties & IS_OG_UNIT) ? OG_PT_SIZE : XL_PT_SIZE;
    const int messages = (properties & IS_OG_UNIT) ? OG_PT_MSG : XL_PT_MSG;
    /* Patch Dump Req Header */
    unsigned char command[10] = { 0xF0, 0x00, 0x01, 0x74, 0x03, 0x03 };
    unsigned char buffer[XL_PT_SIZE];

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
    recalcSysex(properties, command, 10);
    clearMidiInBuffer();

    /* Request a preset dump */
    sendMidi(command, 10);
    ret = fetchUntilHeaderCorrect(buffer, PT_START);

    if (ret == 0) {
        progressCallback(0.0);
        for (int i = 0; i < messages; i++) {
            getMidi(&buffer[PT_START + (i * PT_DATA)], PT_DATA);
            progressCallback(((double)i + 1) / ((double)messages + 2));
        }
        getMidi(&buffer[lengthOfFile - PT_END], PT_END);
        progressCallback(1.0);

        /* Save the preset */
        if (pathToSave[strlen(pathToSave) - 1] == '/') {
            const char *name = getPresetName(buffer);
            strcat(pathToSave, name);
        }
        FILE *file = fopen(pathToSave, "wb");
        if (file == NULL) return FILE_ERROR;
        fwrite(buffer, sizeof(unsigned char), (properties & IS_OG_UNIT) ? OG_PT_SIZE : XL_PT_SIZE, file);
        fclose(file);
        nameProvider(basename(pathToSave));
    }
    clearMidiInBuffer();
    return ret;
}

static char getIR(char *pathToSave, unsigned char properties, int location) {
    char ret;
    const int lengthOfFile = (properties & IS_OG_UNIT) ? OG_IR_SIZE : XL_IR_SIZE;
    const int startSize = (properties & IS_OG_UNIT) ? OG_IR_START : XL_IR_START;
    /* IR Dump Req Header */
    unsigned char command[10] = { 0xF0, 0x00, 0x01, 0x74, 0x03, 0x19 };
    unsigned char buffer[XL_IR_SIZE];

    /* Location number */
    if (properties & IS_OG_UNIT) {
        command[6] = location - 1;
    } else {
        location--;
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
        }
        location++;
    }
    recalcSysex(properties, command, (properties & IS_OG_UNIT) ? 9 : 10);
    clearMidiInBuffer();

    /* Request a IR dump */
    sendMidi(command, (properties & IS_OG_UNIT) ? 9 : 10);
    ret = fetchUntilHeaderCorrect(buffer, startSize);

    if (ret == 0) {
        progressCallback(0.0);
        for (int i = 0; i < IR_MSG; i++) {
            getMidi(&buffer[startSize + (i * IR_DATA)], IR_DATA);
            progressCallback(((double)i + 1) / (double)(IR_MSG + 2));
        }
        getMidi(&buffer[lengthOfFile - IR_END], IR_END);

        progressCallback(1.0);

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
    clearMidiInBuffer();
    return ret;
}

static char checkLocationValidTx(unsigned char properties, int location) {
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

static char checkLocationValidRx(unsigned char properties, int location) {
    char ret = 0;
    if (properties & IS_PRESET) {
        if (properties & IS_OG_UNIT) {
            ret = location < 0 || location > 383 ? LOCATION_OOB : 0;
        } else {
            ret = location < 0 || location > 767 ? LOCATION_OOB : 0;
        }
    } else if (properties & IS_VALID) {
        if (properties & IS_OG_UNIT) {
            ret = location < 1 || location > 100 ? LOCATION_OOB : 0;
        } else {
            ret = location < 1 || location > 1024 ? LOCATION_OOB : 0;
        }
    }
    return ret;
}

char sendFile(char *pathToFile, unsigned char properties, int location) {
    char ret = checkLocationValidTx(properties, location);
    if (ret != 0) return ret;
    if (properties & IS_PRESET) {
        (void)location;
        ret = sendPreset(pathToFile, properties);
    } else if (properties & IS_VALID) {
        ret = sendIR(pathToFile, properties, location);
    } else {
        ret = PROPERTIES_INVALID;
    }
    clearMidiInBuffer();
    return ret;
}

char getFile(char *pathToSave, unsigned char properties, int location) {
    char ret = checkLocationValidRx(properties, location);
    if (ret != 0) return ret;
    if (properties & IS_PRESET) {
        ret = getPreset(pathToSave, properties, location);
    } else if (properties & IS_VALID) {
        ret = getIR(pathToSave, properties, location);
    } else {
        ret = PROPERTIES_INVALID;
    }
    clearMidiInBuffer();
    return ret;
}
