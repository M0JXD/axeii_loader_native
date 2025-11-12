/* Copyright 2025 Jamie Drinkell. MIT License. */

/* A simple utility to load presets and IRs in/out of an Axe-FX II
 * Only tested on Linux Mint 22.2 with an Axe-FX II MkII
 * Unsure if it will work for XL/XL+ units.
 * NO WARRANTY IS PROVIDED, USE AT YOUR OWN RISK.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iup.h>
#include "axeii_loader.h"

/* Made global so progessCallback can update them */
static Ihandle *messagelabel, *progressbar;
static char filepath[256];  /* forgive me security gods */

void progressCallback(int currentProgress) {
    char valAsString[16];
    sprintf(valAsString, "%d", currentProgress);
    if (currentProgress >= 0) {
        IupSetAttribute(messagelabel, "VALUE", "Doing transfer...");
        IupSetAttribute(progressbar, "VALUE", valAsString);
    } else {
        IupSetAttribute(messagelabel, "VALUE", "Trying to capture header...");
    }
}

static int openFile_cb(void) {
    if (IupGetFile(filepath))
        IupSetAttribute(IupGetHandle("send_file"), "VALUE", filepath);
    return IUP_DEFAULT;
}

static int openDir_cb(void) {
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg, "DIALOGTYPE", "DIR");
    if (IupPopup(dlg, IUP_CENTER, IUP_CENTER) == IUP_NOERROR) {
        char *dir = IupGetAttribute(dlg, "VALUE");
        if (dir) IupSetAttribute(IupGetHandle("recv_dir"), "VALUE", dir);
    }
    IupDestroy(dlg);
    return IUP_DEFAULT;
}

static int start_cb(void) {
    char *midiDevice = IupGetAttribute(IupGetHandle("list_dev"), "VALUE");
    if (!midiDevice || midiDevice[0] == 0) return IUP_DEFAULT;

    /* Setup unit type */
    int loc = atoi(IupGetAttribute(IupGetHandle("send_loc"), "VALUE"));
    char *filePath = IupGetAttribute(IupGetHandle("send_file"), "VALUE");
    char prop = detectFileProperties(filePath);

    /* Call your library sendFile/getFile as needed here */
    sendFile(filePath, prop, loc);

    return IUP_DEFAULT;
}

static int exit_cb(void) { return IUP_CLOSE; }

static void show_notes_cb(void) {
    Ihandle *note1 = IupLabel("Note: Preset location is ignored when sending to edit buffer.");
    Ihandle *note2 = IupLabel("Note: Scratchpad presets start at 101+ on MkII units.");
    Ihandle *box   = IupVbox(
        IupSetAttributes(note1, "EXPAND=HORIZONTAL"),
        IupSetAttributes(note2, "EXPAND=HORIZONTAL"),
        NULL);
    Ihandle *dlg = IupDialog(box);
    IupSetAttributes(dlg, "TITLE=\"Notes\", SIZE=300x100");
    IupPopup(dlg, IUP_CENTER, IUP_CENTER);
    IupDestroy(dlg);
}

/* Get MIDI devices using `amidi -l` */
static void getMidiDevices(Ihandle *list) {
    FILE *fp = popen("amidi -l", "r");
    if (!fp) return;
    char line[256];
    int idx = 1;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "Dir") || strstr(line, "Device")) continue;
        char *dev = strstr(line, "hw:");
        if (dev) {
            IupSetAttributeId(list, "", idx, dev);
            idx++;
        }
    }
    pclose(fp);
    if (idx == 1) IupSetAttributeId(list, "", 1, ""); /* at least one */
}

/* MAIN */
int main(int argc, char **argv) {
    Ihandle *dlg, *frame_1, *frame_2, *box_1, *box_2, *box_3,
            *topmenu_1, *topmenu_2, *menu_1, *menu_2,
            *label_1, *label_2, *label_3,
            *entry_1, *entry_2, *entry_3,
            *select_1, *select_2, *button_1,
            *sendtab, *recievetab, *tabs;

    IupOpen(&argc, &argv);

    /* DEVICE SETUP */
    label_1 = IupLabel("MIDI Device:");
    label_2 = IupLabel("AXE-FX II Type:");
    entry_1 = IupList(NULL);
    entry_2 = IupList(NULL);

    IupSetAttributes(entry_1, "DROPDOWN=YES, EXPAND=HORIZONTAL");
    IupSetAttributes(entry_2, "1=OG/MKII, 2=XL, 3=XL+, VALUE=1, DROPDOWN=YES, EXPAND=HORIZONTAL");
    IupSetHandle("list_dev", entry_1);
    IupSetHandle("list_type", entry_2);
    getMidiDevices(entry_1);

    box_1 = IupGridBox(label_1, entry_1, label_2, entry_2, NULL);
    IupSetAttributes(box_1, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT, GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=2");
    IupSetAttribute(box_1, "RASTERSIZE", "x80");  /* GridBox seems to underestimate it's size a bit */
    frame_1 = IupFrame(box_1);
    IupSetAttributes(frame_1, "TITLE=\"Device Setup\"");

    /* SEND TAB */
    label_1  = IupLabel("File:");
    label_2  = IupLabel("Location:");
    entry_1  = IupText(NULL);
    button_1 = IupButton("Browse...", NULL);
    entry_2  = IupText(NULL);

    IupSetAttributes(entry_1, "EXPAND=HORIZONTAL");
    IupSetAttributes(entry_2, "EXPAND=HORIZONTAL, SPIN=YES, SPINMIN=0, SPINMAX=384, SPININC=1");
    IupSetHandle("send_file", entry_1);
    IupSetHandle("send_loc", entry_2);
    IupSetCallback(button_1, "ACTION", (Icallback)openFile_cb);

    sendtab = IupGridBox(label_1, entry_1, button_1, label_2, entry_2, IupSpace(), NULL);
    IupSetAttributes(sendtab, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT, GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=3, CMARGIN=5x5");
    IupSetAttribute(sendtab, "TABTITLE", "SEND");

    /* RECEIVE TAB */
    label_1 = IupLabel("Directory:");
    label_2 = IupLabel("Location:");
    label_3 = IupLabel("Type:");
    entry_1 = IupText(NULL);
    button_1 = IupButton("Browse...", NULL);
    entry_2 = IupText(NULL);
    IupSetAttributes(entry_1, "EXPAND=HORIZONTAL");
    IupSetAttributes(entry_2, "EXPAND=HORIZONTAL, SPIN=YES, SPINMIN=0, SPINMAX=384, SPININC=1");
    IupSetHandle("recv_dir", entry_1);
    IupSetHandle("recv_loc", entry_2);
    IupSetCallback(button_1, "ACTION", (Icallback)openDir_cb);

    select_1 = IupSetAttributes(IupToggle("PRESET", NULL), "EXPAND=HORIZONTAL");
    select_2 = IupSetAttributes(IupToggle("IR", NULL), "EXPAND=HORIZONTAL");
    entry_3  = IupRadio(IupHbox(select_1, select_2, NULL));

    recievetab = IupGridBox(label_1, entry_1, button_1,
                            label_2, entry_2, IupSpace(),
                            label_3, entry_3, IupSpace(), NULL);

    IupSetAttribute(recievetab, "RASTERSIZE", "x110");  /* GridBox seems to underestimate it's size a bit */
    IupSetAttributes(recievetab, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT, GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=3, CMARGIN=5x5");
    IupSetAttribute(recievetab, "TABTITLE", "RECEIVE");
    tabs = IupTabs(sendtab, recievetab, NULL);

    /* TRANSFER DETAILS */
    label_1 = IupLabel("Status:");
    label_2 = IupLabel("Progress:");
    messagelabel = IupLabel("Messages Will Display Here.");
    progressbar  = IupProgressBar();
    IupSetAttributes(messagelabel, "EXPAND=HORIZONTAL");
    IupSetAttributes(progressbar, "MIN=0, MAX=100, VALUE=0, EXPAND=HORIZONTAL, SIZE=120x14");
    box_1 = IupGridBox(label_1, messagelabel, label_2, progressbar, NULL);
    IupSetAttributes(box_1, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT, GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=2, CMARGIN=5x5");
    IupSetAttribute(box_1, "RASTERSIZE", "x80");  /* GridBox seems to underestimate it's size a bit */
    frame_2 = IupFrame(box_1);
    IupSetAttributes(frame_2, "TITLE=\"Transfer Details\", EXPAND=HORIZONTAL");

    /* START BUTTON */
    button_1 = IupButton("Start", NULL);
    IupSetAttributes(button_1, "SIZE=80x20, ACTIVE=NO");
    IupSetCallback(button_1, "ACTION", (Icallback)start_cb);
    box_2 = IupHbox(IupFill(), button_1, NULL);
    IupSetAttributes(box_2, "MARGIN=0x5, EXPAND=HORIZONTAL");

    /* MAIN LAYOUT */
    box_1 = IupVbox(frame_1, tabs, frame_2, IupFill(), box_2, NULL);
    IupSetAttributes(box_1, "GAP=10, MARGIN=6x6");

    /* MENUS */
    menu_1  = IupItem("Exit", NULL);
    menu_2 = IupItem("Notes", NULL);
    IupSetCallback(menu_1, "ACTION", (Icallback)exit_cb);
    IupSetCallback(menu_2, "ACTION", (Icallback)show_notes_cb);
    topmenu_1 = IupMenu(menu_1, NULL);
    topmenu_2 = IupMenu(menu_2, NULL);
    menu_1 = IupMenu(IupSubmenu("File", topmenu_1), IupSubmenu("Help", topmenu_2), NULL);

    /* DIALOG */
    dlg = IupDialog(box_1);
    IupSetAttributes(dlg, "TITLE=\"AXE-FX II LOADER\", MAXSIZE=350x500, RESIZE=NO");
    IupSetAttributeHandle(dlg, "MENU", menu_1);

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();
    IupClose();
    return EXIT_SUCCESS;
}
