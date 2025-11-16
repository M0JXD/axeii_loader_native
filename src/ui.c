/*    ui.c - GUI interface to send/receive data from an Axe-FX II
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
#include <iup.h>
#include "axeii_utils.h"
#include "midi_devs.h"

static enum {
    SEND_MODE = 1,
    RECEIVE_MODE
} mode = SEND_MODE;

static Ihandle *startbutton, *messagelabel, *progressbar;
static char filepath[256], midiDevice[20];  /* Forgive me security gods */
static char properties = 0;
static int location;

void progressCallback(int currentProgress) {
    char valAsString[16];
    sprintf(valAsString, "%d", currentProgress);
    if (currentProgress >= 0) {
        IupSetAttribute(messagelabel, "TITLE", "Doing transfer...");
        IupSetAttribute(progressbar, "VALUE", valAsString);
    } else {
        IupSetAttribute(messagelabel, "TITLE", "Trying to capture header...");
    }
}

/* Get MIDI devices using `amidi -l` */
static void getMidiDevices(Ihandle *list) {
    FILE *fp = popen("amidi -l", "r");
    if (!fp) exit(-1);
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

static char detectAndEnable(char* filepath) {
    if(midiDevice) {
        if (mode == SEND_MODE) {
            properties = detectFileProperties(filepath);
        } else if (mode == RECEIVE_MODE) {
            ;
        }
        if (properties)
            IupSetAttribute(startbutton, "ACTIVE", "YES");
    }
    return 0;
}

static int setMode_cb(Ihandle* ih, int new_pos, int old_pos) {
    mode = new_pos + 1;
    detectAndEnable(filepath);
    return IUP_DEFAULT;
}


static int openFile_cb(void) {
    if (IupGetFile(filepath) + 1) {
        IupSetAttribute(IupGetHandle("send_file"), "VALUE", filepath);
        detectAndEnable(filepath);
    }
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
    detectAndEnable(filepath);
    return IUP_DEFAULT;
}

static int start_cb(void) {
    char *midiDevice = IupGetAttribute(IupGetHandle("list_dev"), "VALUE");
    int location = atoi(IupGetAttribute(IupGetHandle("send_loc"), "VALUE"));
    IupSetAttribute(progressbar, "VALUE", "0");

    setupRawMIDIHandles("");
    if (mode == SEND_MODE) {
        char prop = detectFileProperties(filepath);
        sendFile(filepath, properties, location);
    } else if (mode == RECEIVE_MODE) {
        /* TODO: Append a name to the path */
        getFile(filepath, properties, location);
    }

    closeRawMIDIHandles();
    return IUP_DEFAULT;
}

static void show_notes_cb(void) {
    Ihandle *note1 = IupLabel("Note 1: Preset location is ignored when sending to edit buffer.");
    Ihandle *note2 = IupLabel("Note 2: Scratchpad presets start at 101+ on MkII units.");
    Ihandle *note3 = IupLabel("Note 3: XL/XL+ usage is untested, please see README and send feedback!");
    Ihandle *box   = IupVbox(
        IupSetAttributes(note1, "EXPAND=HORIZONTAL"),
        IupSetAttributes(note2, "EXPAND=HORIZONTAL"),
        IupSetAttributes(note3, "EXPAND=HORIZONTAL"),
        NULL);
    Ihandle *dlg = IupDialog(box);
    IupSetAttributes(dlg, "TITLE=\"Notes\", SIZE=320x50, RESIZE=NO, MINBOX=NO");
    IupPopup(dlg, IUP_CENTER, IUP_CENTER);
    IupDestroy(dlg);
}

static int exit_cb(void) { return IUP_CLOSE; }

/* MAIN */
int main(int argc, char **argv) {
    Ihandle *dlg, *frame_1, *frame_2, *box_1, *box_2, *box_3,
            *topmenu_1, *topmenu_2, *menu_1, *menu_2,
            *label_1, *label_2, *label_3,
            *entry_1, *entry_2, *entry_3,
            *select_1, *select_2, *button_1,
            *sendtab, *receivetab, *tabs;

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
    IupSetAttributes(box_1, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT,"
                            "GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=2");
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
    IupSetAttributes(entry_2, "EXPAND=HORIZONTAL, SPIN=YES, SPINMIN=0, SPINMAX=384");
    IupSetHandle("send_file", entry_1);
    IupSetHandle("send_loc", entry_2);
    IupSetCallback(button_1, "ACTION", (Icallback)openFile_cb);

    sendtab = IupGridBox(label_1, entry_1, button_1, label_2, entry_2, IupSpace(), NULL);
    IupSetAttributes(sendtab, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT,"
                              "GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=3, CMARGIN=5x5");
    IupSetAttribute(sendtab, "TABTITLE", "SEND");

    /* RECEIVE TAB */
    label_1 = IupLabel("Directory:");
    label_2 = IupLabel("Location:");
    label_3 = IupLabel("Type:");
    entry_1 = IupText(NULL);
    button_1 = IupButton("Browse...", NULL);
    entry_2 = IupText(NULL);
    IupSetAttributes(entry_1, "EXPAND=HORIZONTAL");
    IupSetAttributes(entry_2, "EXPAND=HORIZONTAL, SPIN=YES, SPINMIN=0, SPINMAX=384");
    IupSetHandle("recv_dir", entry_1);
    IupSetHandle("recv_loc", entry_2);
    IupSetCallback(button_1, "ACTION", (Icallback)openDir_cb);

    select_1 = IupSetAttributes(IupToggle("PRESET", NULL), "EXPAND=HORIZONTAL");
    select_2 = IupSetAttributes(IupToggle("IR", NULL), "EXPAND=HORIZONTAL");
    entry_3  = IupRadio(IupHbox(select_1, select_2, NULL));

    receivetab = IupGridBox(label_1, entry_1, button_1,
                            label_2, entry_2, IupSpace(),
                            label_3, entry_3, IupSpace(), NULL);

    IupSetAttribute(receivetab, "RASTERSIZE", "x110");  /* GridBox seems to underestimate it's size a bit */
    IupSetAttributes(receivetab, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT,"
                                 "GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=3, CMARGIN=5x5");
    IupSetAttribute(receivetab, "TABTITLE", "RECEIVE");
    tabs = IupTabs(sendtab, receivetab, NULL);
    IupSetCallback(tabs, "TABCHANGEPOS_CB", (Icallback)setMode_cb);

    /* TRANSFER DETAILS */
    messagelabel = IupLabel("Messages Will Display Here. Progress Bar Is Below.");
    progressbar  = IupProgressBar();
    IupSetAttributes(messagelabel, "EXPAND=HORIZONTAL");
    IupSetAttributes(progressbar, "MIN=0, MAX=100, VALUE=0, EXPAND=HORIZONTAL");
    box_1 = IupVbox(messagelabel, progressbar, NULL);
    IupSetAttributes(box_1, "GAP=1");
    frame_2 = IupFrame(box_1);
    IupSetAttributes(frame_2, "TITLE=\"Transfer Details\", EXPAND=HORIZONTAL");

    /* START BUTTON AND LAYOUT */
    startbutton = IupButton("Start", NULL);
    IupSetAttributes(startbutton, "SIZE=60x18, ACTIVE=NO");
    IupSetCallback(startbutton, "ACTION", (Icallback)start_cb);
    box_2 = IupHbox(IupFill(), startbutton, NULL);
    IupSetAttributes(box_2, "MARGIN=0x5, EXPAND=HORIZONTAL");
    box_1 = IupVbox(frame_1, tabs, frame_2, box_2, NULL);
    IupSetAttributes(box_1, "GAP=10, MARGIN=6x6");

    /* MENU */
    menu_1 = IupItem("Exit", NULL);
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
