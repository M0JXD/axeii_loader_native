/*    cli.c - CLI interface to send/receive data from an Axe-FX II
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "axeii_utils.h"
#include "midi_devs.h"

/* ENUMS */
enum modes { SEND = 1, RECEIVE };

/* FUNCTIONS */
static void usage() {
    puts("=== Axe-FX II Loader Help Text ===");
    puts("The default mode is receive mode, and will save obtained files into the current directory.");
    puts("If the -d option is not given it will try to autodetect from up to the first five available devices.");
    puts("The unit type (-t option) does not autodetect, and transfers may fail if it's incorrect.");
    puts("");
    puts("=== Options ===");
    puts("-d <device>      ALSA device string. Use 'amidi -l' and pass the \"Device\", e.g. \"hw:2,0\".");
    puts("-i <file name>   Set send mode with given input file (whether it's a preset or IR will be autodetected).");
    puts("-o <file name>   Override the provided file name for receive mode.");
    puts("-m               Set to get IRs in receive mode (ignored for send mode).");
    puts("-p <integer>     Set Preset or IR location, defaults to 0. Ignored when sending presets as they're loaded to the edit buffer.");
    puts("-t <o/x/p>       Set connected unit as Original/MKII (o), XL (x) or XL Plus (p). Defaults to Original/MKII." );
    puts("-h               Show this help text.");
    puts("=== END OF HELP ===");
}

static char getDevice(char* devString) {
    dev_info_t **devs;
    int amount, index;
    char ret = 0;
    devs = get_axe_midi_devs(&amount, &index);

    if (index >= 0) {
        strcpy(devString, devs[index]->hw_string);
    } else {
        ret = -1;
    }
    free_axe_midi_devs(devs);
    return ret;
}

/* axeii_loader needs us to implement this */
void progressCallback(int currentProgress) {
    static int oldProgress = 0;
    if (currentProgress <= -1) {
        puts("Trying to lock onto header...");
    } else if (currentProgress == 0) {
        printf("Progress: 0%% ...");
        fflush(stdout);
    } else if (currentProgress == 100) {
        printf(" 100%%\n");
    } else {
        if (currentProgress > (oldProgress + 2)) {
            printf(".");
            fflush(stdout);
            oldProgress = currentProgress;
        }
    }
}

void nameProvider(char *name) {
    printf("File saved as %s\n", name);
}

/* MAIN ENTRY */
int main(int argc, char *argv[]) {
    unsigned char properties = OG_PRESET | IS_OG_UNIT; /* See the defines in axeii_loader.h */
    char mode                = RECEIVE;                /* See enum modes */
    int location             = 0;                      /* Preset or cab number */
    char path[256]           = "";                     /* Forgive me security gods... */
    char devString[32]       = { '\0', '\0' };         /* ALSA device string */
    int ret = 0;
    int opt = 0;

    if(getcwd(path, sizeof(path)) == NULL) {
        fprintf(stderr, "Failed to get current directory!\n");
        return FILE_ERROR;
    }
    path[strlen(path)] = '/'; path[strlen(path) + 1] = '\0';

    puts("===== AXE-FX II LOADER =====");
    while((opt = getopt(argc, argv, "d:i:o:p:t:smh")) != -1) {
        switch (opt) {
            /* MIDI Device */
            case 'd':
                strcpy(devString, optarg);
            break;

            /* Input file */
            case 'i':
                mode = SEND;
                strcpy(path, optarg);
            break;

            /* Output file */
            case 'o':
                if (mode == SEND) {
                    puts("Can't specify both send and receive mode.");
                    return 1;
                }
                strcpy(path, optarg);
            break;

            /* Set IR receive mode */
            case 'm':
                properties &= SET_IR;
                if (location == 0) location = 1;
            break;

            /* Preset or IR number */
            case 'p':
                location = atoi(optarg);
            break;

            case 't':
                properties &= CLEAR_UNIT;
                switch (optarg[0]) {
                    default:
                    case 'o':
                        properties |= IS_OG_UNIT;
                        puts("Axe-FX Type set to OG/MkII");
                    break;
                    case 'x':
                        properties |= IS_XL_UNIT;
                        puts("Axe-FX Type set to XL");
                    break;
                    case 'p':
                        properties |= IS_XLP_UNIT;
                        puts("Axe-FX Type set to XL Plus");
                    break;

                }
            break;

            /* Help */
            case 'h':
                usage();
                return 0;
            break;

            case '?':
                puts("Unknown option passed. Pass -h for help.");
                return 1;
            break;
        }
    }

    /* Setup MIDI */
    if (devString[0] == '\0') {
        puts("No device specified. Attempting to autodetect...");
        ret = getDevice(devString);
        if (ret != 0) {
            puts("Could not detect Axe-FX II. Please specify with -d option.");
            return 1;
        } else {
            printf("Located an Axe-FX II at %s\n", devString);
        }
    }

    if (setupRawMIDIHandles(devString) != 0) {
        puts("Error opening MIDI device!");
        return 1;
    }

    /* Run actions */
    if (ret == 0) {
        char* type = properties & IS_PRESET ? "Preset" : "IR";

        if (mode == SEND) {
            puts("=== SEND MODE ===");
            properties = detectFileProperties(path) | (properties & CLEAR_FILE);
            if (properties & IS_VALID) {
                char* unit = properties & IS_OG_FILE ? "OG/MKII" :
                             properties & IS_XL_FILE ? "XL" : "XL Plus";
                printf("Detected a %s file for a %s\n", type, unit);
                properties & IS_PRESET ? puts("Attempting to send to edit buffer...") :
                                         printf("Attempting to send to location %d...\n", location);
            }
            ret = sendFile(path, properties, location);
        } else if (mode == RECEIVE) {
            puts("=== RECEIVE MODE ===");
            char* unit = properties & IS_OG_UNIT ? "OG/MKII" :
                         properties & IS_XL_UNIT ? "XL" : "XL Plus";
            printf("Attempting to get a %s file from a %s from location %d...\n", type, unit, location);
            ret = getFile(path, properties, location);
        }

        if (ret == FILE_ERROR) {
            puts("Couldn't open file!");
        } else if (ret == DESTINATION_UNIT_INVALID) {
            puts("OG and XL/XL+ presets are not cross compatible! Please convert for your unit.");
        } else if (ret == HEADER_LOCK_ISSUE) {
            puts("Couldn't lock onto header!");
        } else if (ret == PROPERTIES_INVALID) {
            puts("File and/or values are not valid!");
        } else if (ret == LOCATION_OOB) {
            puts("Location is out of bounds!");
        }
        closeRawMIDIHandles();
    }
    if (ret == 0) puts("Transfer success!\nThank you :)");
    return ret;
}
