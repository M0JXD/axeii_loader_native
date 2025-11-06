/* Copyright 2025 Jamie Drinkell. MIT License. */

/* A simple utility to load presets and IRs in/out of an Axe-FX II
 * Only tested on Linux Mint 22.2 with an Axe-FX II MkII
 * Unsure if it will work for XL/XL+ units.
 * NO WARRANTY IS PROVIDED, USE AT YOUR OWN RISK.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "axeii_loader.h"

/* ENUMS */
enum modes { SEND = 1, RECEIVE };

/* FUNCTIONS */
static void usage() {
    puts("=== Axe-FX II Loader Help Text ===");
    puts("-d and either -i or -o with a file name must be provided. You can't provide both -i and -o.");
    puts("");
    puts("=== Options ===");
    puts("-d <device>      ALSA device string. Use 'amidi -l' and pass the \"Device\", e.g. \"hw:2,0\".");
    puts("-i <file name>   Set input (send mode) file (whether it's a preset or IR will be autodetected).");
    puts("-o <file name>   Set output (receive mode) file.");
    puts("-m               Set to get IRs in receive mode (ignored for send mode).");
    puts("-p <integer>     Set Preset or IR location, defaults to 0. Ignored when sending presets as they're loaded to the edit buffer.");
    puts("-t <o/x/p>       Set connected unit as Original/MKII (o), XL (x) or XL Plus (p) type unit. Defaults to Original/MkII." );
    puts("-h               Show this help text.");
    puts("=== END OF HELP ===");
}


/* axeii_loader needs us to implement this */
void progressCallback(int currentProgress) {
    (void)currentProgress; /* Suppress unused warning */
    printf(".");
    fflush(stdout);
}

/* Handy Util */ /*
static void printTenBytes(char *command) {
    printf("Bytes are: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
            command[0], command[1], command[2], command[3], command[4],
            command[5], command[6], command[7], command[8], command[9]
          );
} */

/* MAIN ENTRY */
int main(int argc, char *argv[]) {
    char properties                       /* See the defines in axeii_loader.h */
    char mode           = 0;              /* See enum modes */
    int location          = 0;            /* Preset or cab number */
    char path[256]      = "";             /* For file path. Forgive me security gods... */
    char devString[32] = { '\0', '\0' };  /* ALSA device string */

    int ret = 0;
    int opt = 0;

    puts("===== AXE-FX II LOADER =====");
    while((opt = getopt(argc, argv, "d:i:o:p:t:smh")) != -1) {
        switch (opt) {
            /* MIDI Device */
            case 'd':
                strcpy(devString, optarg);
            break;

            /* Input file */
            case 'i':
                if (!mode) {
                    mode = SEND;
                } else {
                    puts("Can't specify both send and receive mode.");
                    return 1;
                }
                strcpy(path, optarg);
            break;

            /* Output file */
            case 'o':
                if (!mode) {
                    mode = RECEIVE;
                } else {
                    puts("Can't specify both send and receive mode.");
                    return 1;
                }
                strcpy(path, optarg);
            break;

            /* IR or Preset receive mode */
            case 'm':
                type = IR;
            break;

            /* Preset or IR number */
            case 'p':
                number = atoi(optarg);
            break;

            case 't':
                switch (optarg[0]) {
                    default:
                    case 'o':
                        unitType = OG;
                        puts("Axe-FX Type set to OG/MkII");
                    break;
                    case 'x':
                        unitType = XL;
                        puts("Axe-FX Type set to XL");
                    break;
                    case 'p':
                        unitType = XLP;
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

    /* Sanity checks */
    if (mode == 0) {
        puts("No options specified. Pass -h for help.");
        ret = 1;
    } else {
        if (devString[0] == '\0') {
            puts("No device specified. Please specify with -d option.");
            ret = 1;
        } else {
            if (setupRawMIDIHandles(devString) != 0) {
                puts("Error opening MIDI device!");
                ret = 1;
            }
        }
    }

    /* Run actions */
    if (ret == 0) {
        if (mode == SEND) {
            puts("=== SEND MODE ===");
            type = detectFileProperties(path);

            switch (type) {

                case OG_IR:
                case OG_PRESET:



            }


            if (type == PRESET) {
                ret = sendPreset(path, unitType, fileUnit);
            } else if (type == IR) {
                ret = sendIR(number, path, unitType, fileUnit);
            } else {
                ret = 1;
            }
        } else if (mode == RECEIVE) {
            puts("=== RECEIVE MODE ===");
            if (type == PRESET) {
                ret = getPreset(number, path, unitType);
            } else if (type == IR) {
                ret = getIR(number, path, unitType);
            }
        }
        closeRawMIDIHandles();
    }
    if (ret == 0) puts("Thank you :)");
    return ret;
}
