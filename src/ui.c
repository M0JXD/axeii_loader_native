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

/* These need ot be global for the progessCallback to update them */
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
    Ihandle *dlg, *frame, *box_1, *box_2, *box_3,
            *topmenu_1, *topmenu_2, *menu_1, *menu_2,
            *label_1, *label_2, *label_3,
            *entry_1, *entry_2, *entry_3,
            *select_1, *select_2, *button_1,
            *sendtab, *recievetab, *tabs;

    IupOpen(&argc, &argv);

    /* Device Setup */
    label_1 = IupLabel("MIDI Device:");
    label_2 = IupLabel("AXE-FX II Type:");
    entry_1 = IupList(NULL);
    IupSetAttributes(label_1, "SIZE=75x, ALIGNMENT=ALEFT");
    IupSetAttributes(label_2, "SIZE=75x, ALIGNMENT=ALEFT");
    IupSetAttributes(entry_1, "DROPDOWN=YES, EXPAND=HORIZONTAL");
    getMidiDevices(entry_1);
    IupSetHandle("list_dev", entry_1);

    entry_2 = IupList(NULL);
    IupSetAttributes(entry_2, "1=OG/MKII, 2=XL, 3=XL+, VALUE=1, DROPDOWN=YES, EXPAND=HORIZONTAL");
    IupSetHandle("list_type", entry_2);

    box_1 = IupHbox(label_1, entry_1, NULL);
    box_2 = IupHbox(label_2, entry_2, NULL);
    box_3 = IupVbox(box_1, box_2, NULL);
    IupSetAttributes(box_3, "GAP=3, MARGIN=5x5, EXPAND=HORIZONTAL");
    frame = IupFrame(box_3);
    IupSetAttributes(frame, "TITLE=\"Device Setup\", EXPAND=HORIZONTAL");

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
    IupSetAttributes(sendtab, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT, GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=3");
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

    select_1 = IupToggle("PRESET", NULL);
    select_2 = IupToggle("IR", NULL);
    entry_3  = IupRadio(IupHbox(select_1, select_2, NULL));

    recievetab = IupGridBox(label_1, entry_1, button_1,
                            label_2, entry_2, IupSpace(),
                            label_3, entry_3, IupSpace(), NULL);

    IupSetAttributes(recievetab, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT, GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=3");
    IupSetAttribute(recievetab, "TABTITLE", "RECEIVE");
    tabs = IupTabs(sendtab, recievetab, NULL);

    /* TRANSFER DETAILS FRAME */
    Ihandle *status_label = IupLabel("Status:");
    IupSetAttributes(status_label, "SIZE=60x, ALIGNMENT=ALEFT");
    messagelabel = IupLabel("Messages displayed here");
    IupSetAttributes(messagelabel, "EXPAND=HORIZONTAL, ALIGNMENT=ALEFT");
    Ihandle *status_row = IupHbox(status_label, messagelabel, NULL);
    IupSetAttributes(status_row, "GAP=6, EXPAND=HORIZONTAL");

    Ihandle *progress_label = IupLabel("Progress:");
    IupSetAttributes(progress_label, "SIZE=60x, ALIGNMENT=ALEFT, PADDING=0x4");
    progressbar = IupProgressBar();
    IupSetAttributes(progressbar, "MIN=0, MAX=100, VALUE=0, EXPAND=HORIZONTAL, SIZE=120x14");
    Ihandle *progress_row = IupHbox(progress_label, progressbar, NULL);
    IupSetAttributes(progress_row, "GAP=6, EXPAND=HORIZONTAL");

    Ihandle *transfer_box = IupVbox(status_row, progress_row, NULL);
    IupSetAttributes(transfer_box, "GAP=6, MARGIN=8x8, EXPAND=HORIZONTAL");

    Ihandle *transfer_frame = IupFrame(transfer_box);
    IupSetAttributes(transfer_frame, "TITLE=\"Transfer Details\", EXPAND=HORIZONTAL");

    /* START BUTTON */
    button_1 = IupButton("Start", NULL);
    IupSetAttributes(button_1, "SIZE=80x20, ACTIVE=NO");
    IupSetCallback(button_1, "ACTION", (Icallback)start_cb);

    Ihandle *button_row = IupHbox(IupFill(), button_1, NULL);
    IupSetAttributes(button_row, "MARGIN=0x5, EXPAND=HORIZONTAL");

    Ihandle *bottom_box = IupVbox(
        transfer_frame,
        IupFill(),
        button_row,
        NULL);
    IupSetAttributes(bottom_box, "GAP=4, EXPAND=YES");

    /* MAIN LAYOUT */
    Ihandle *top_box = IupVbox(frame, tabs, NULL);
    box_1 = IupVbox(top_box, bottom_box, NULL);
    IupSetAttributes(box_1, "GAP=6, MARGIN=6x6, EXPAND=YES");

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
    IupSetAttributes(dlg, "TITLE=\"AXE-FX II LOADER\", MINSIZE=350x570, RESIZE=NO, SHRINK=YES");
    IupSetAttributeHandle(dlg, "MENU", menu_1);

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();
    IupClose();
    return EXIT_SUCCESS;
}
