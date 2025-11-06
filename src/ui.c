/* Copyright 2025 Jamie Drinkell. MIT License. */

/* A simple utility to load presets and IRs in/out of an Axe-FX II
 * Only tested on Linux Mint 22.2 with an Axe-FX II MkII
 * Unsure if it will work for XL/XL+ units.
 * NO WARRANTY IS PROVIDED, USE AT YOUR OWN RISK.
 */

#include <stdlib.h>
#include <iup.h>
#include "axeii_loader.h"

/* This has to be file global to give the backend the ability to update */
static Ihandle *progressbar;

/* axeii_loader needs us to implement this */
void progressCallback(int currentProgress) {
    (void)currentProgress;
    return;
}

int main(int argc, char **argv)
{
    char filename[256];
    Ihandle *dlg;
    IupOpen(&argc, &argv);

    IupGetFile(filename);

    /*IupMainLoop();*/

    IupClose();
    return EXIT_SUCCESS;
}
