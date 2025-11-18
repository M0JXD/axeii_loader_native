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
static dev_info_t **devs = NULL;

void progressCallback(int currentProgress) {
    char valAsString[16];
    sprintf(valAsString, "%d", currentProgress);
    if (currentProgress >= 0) {
        IupSetAttribute(messagelabel, "TITLE", "Doing transfer...");
        IupSetAttribute(progressbar, "VALUE", valAsString);
    } else if (currentProgress < 0) {
        IupSetAttribute(messagelabel, "TITLE", "Trying to capture header...");
    }
    if (currentProgress == 100) {
        IupSetAttribute(messagelabel, "TITLE", "Transfer complete!");
    }
}

static void getMidiDevices(Ihandle *list) {
    char buf[32];
    int amount = -1, index = -1;
    if (devs != NULL) free_axe_midi_devs(devs);
    devs = get_axe_midi_devs(&amount, &index);

    if (amount >= 0) {
        for (int i = 0; i < amount; i++) {
            sprintf(buf, "%d", i + 1);
            IupSetAttribute(list, buf, devs[i]->hw_name);
            if (i == 5) break;
        }
    }

    if (index >= 0) {
        sprintf(buf, "%d", index + 1);
        IupSetAttribute(list, "VALUE", buf);
    }
}

static char detectAndEnable() {
    char properties = 0;
    char filepath[256];
    if(IupGetAttribute(IupGetHandle("midi_dev"), "VALUE") != NULL) {
        if (mode == SEND_MODE) {
            strcpy(filepath, IupGetAttribute(IupGetHandle("send_file"), "VALUE"));
            properties = detectFileProperties(filepath);
            if ((properties == FILE_ERROR) || !(properties & IS_VALID)) {
                IupSetAttribute(IupGetHandle("send_loc"), "ACTIVE", "NO");
                IupSetAttribute(IupGetHandle("send_type"), "TITLE", "Type could not be detected");
            } else if (properties & IS_PRESET) {
                IupSetAttribute(IupGetHandle("send_loc"), "ACTIVE", "NO");
                IupSetAttribute(IupGetHandle("send_type"), "TITLE", "Preset File Detected");
            } else {
                IupSetAttribute(IupGetHandle("send_loc"), "ACTIVE", "YES");
                IupSetAttribute(IupGetHandle("send_type"), "TITLE", "IR File Detected");
            }
        } else if (mode == RECEIVE_MODE) {
            strcpy(filepath, IupGetAttribute(IupGetHandle("recv_dir"), "VALUE"));
            if (filepath[0] == '/') properties = 1;
        }
        if ((properties & IS_VALID) && (properties != FILE_ERROR))
            IupSetAttribute(startbutton, "ACTIVE", "YES");
        else
            IupSetAttribute(startbutton, "ACTIVE", "NO");
    }
    return 0;
}

static int setMode_cb(Ihandle* ih, int new_pos, int old_pos) {
    (void)ih; (void)old_pos;
    mode = new_pos + 1;
    IupSetAttribute(progressbar, "VALUE", "0");
    detectAndEnable();
    return IUP_DEFAULT;
}

static int openFile_cb(Ihandle* ih) {
    (void)ih;
    char filepath[256];
    IupSetAttribute(progressbar, "VALUE", "0");
    if (IupGetFile(filepath) + 1) {
        IupSetAttribute(IupGetHandle("send_file"), "VALUE", filepath);
        detectAndEnable();
    }
    return IUP_DEFAULT;
}

static int openDir_cb(Ihandle* ih) {
    (void)ih;
    IupSetAttribute(progressbar, "VALUE", "0");
    Ihandle *dlg = IupFileDlg();
    IupSetAttribute(dlg, "DIALOGTYPE", "DIR");
    if (IupPopup(dlg, IUP_CENTER, IUP_CENTER) == IUP_NOERROR) {
        char *dir = IupGetAttribute(dlg, "VALUE");
        if (dir) IupSetAttribute(IupGetHandle("recv_dir"), "VALUE", dir);
    }
    IupDestroy(dlg);
    detectAndEnable();
    return IUP_DEFAULT;
}

static int start_cb(Ihandle* ih) {
    int location = 0, midiIndex;
    char ret, properties, type, filepath[256];
    IupSetAttribute(ih, "ACTIVE", "NO");
    IupSetAttribute(progressbar, "VALUE", "0");
    midiIndex = atoi(IupGetAttribute(IupGetHandle("midi_dev"), "VALUE")) - 1;
    type = atoi(IupGetAttribute(IupGetHandle("axe_type"), "VALUE")) - 1;
    ret = setupRawMIDIHandles(devs[midiIndex]->hw_string);

    IupSetAttribute(messagelabel, "TITLE", "Starting Transfer...");
    if (mode == SEND_MODE) {
        strcpy(filepath, IupGetAttribute(IupGetHandle("send_file"), "VALUE"));
        properties = detectFileProperties(filepath);
        location = atoi(IupGetAttribute(IupGetHandle("send_loc"), "VALUE"));
        ret = sendFile(filepath, properties, location);
    } else if (mode == RECEIVE_MODE) {
        strcpy(filepath, IupGetAttribute(IupGetHandle("recv_dir"), "VALUE"));
        location = atoi(IupGetAttribute(IupGetHandle("recv_loc"), "VALUE"));
        if (strstr(IupGetAttribute(IupGetHandle("type_opt"), "VALUE"), "preset")) {
            properties = !type ? OG_PRESET :
                         (type == 1) ? XL_PRESET : XLP_PRESET;
            strcat(filepath, "/received_preset.syx");
        } else {
            properties = !type ? OG_IR :
                         (type == 1) ? XL_IR : XLP_IR;
            strcat(filepath, "/received_ir.syx");
        }
        ret = getFile(filepath, properties, location);
    }
    closeRawMIDIHandles();

    if (ret == FILE_ERROR) {
        IupSetAttribute(messagelabel, "TITLE", "Couldn't open file!");
    } else if (ret == DESTINATION_UNIT_INVALID) {
        IupSetAttribute(messagelabel, "TITLE", "Can't send XL/XL+ file to OG/MKII!");
    } else if (ret == HEADER_LOCK_ISSUE) {
        IupSetAttribute(messagelabel, "TITLE", "Couldn't lock onto header!");
    } else if (ret == PROPERTIES_INVALID) {
        IupSetAttribute(messagelabel, "TITLE", "File and/or values are not valid!");
    }

    IupSetAttribute(ih, "ACTIVE", "YES");
    return IUP_DEFAULT;
}

static int show_notes_cb(Ihandle* ih) {
    (void)ih;
    Ihandle *note1 = IupLabel("Note 1: Preset location is ignored when sending, as it's loaded to the edit buffer.");
    Ihandle *note2 = IupLabel("Note 2: Scratchpad presets start at 101+ on MkII units.");
    Ihandle *note3 = IupLabel("Note 3: XL/XL+ usage is untested, please see README and send feedback!");
    Ihandle *box   = IupVbox(
        IupSetAttributes(note1, "EXPAND=HORIZONTAL"),
        IupSetAttributes(note2, "EXPAND=HORIZONTAL"),
        IupSetAttributes(note3, "EXPAND=HORIZONTAL"),
        NULL);
    Ihandle *dlg = IupDialog(box);
    IupSetAttributes(dlg, "TITLE=\"Notes\", SIZE=350x50, RESIZE=NO, MINBOX=NO");
    IupPopup(dlg, IUP_CENTER, IUP_CENTER);
    IupDestroy(dlg);
    return IUP_DEFAULT;
}

/* MAIN */
int main(int argc, char **argv) {
    Ihandle *dlg, *frame_1, *frame_2, *box_1, *box_2,
            *label_1, *label_2, *label_3,
            *entry_1, *entry_2, *entry_3,
            *select_1, *select_2, *button_1,
            *sendtab, *receivetab, *tabs,
            *topmenu_1, *menu_1;

    IupOpen(&argc, &argv);

    /* DEVICE SETUP */
    label_1 = IupLabel("MIDI Device:");
    label_2 = IupLabel("AXE-FX II Type:");
    entry_1 = IupList(NULL);
    entry_2 = IupList(NULL);

    IupSetAttributes(entry_1, "DROPDOWN=YES, EXPAND=HORIZONTAL");
    IupSetAttributes(entry_2, "1=OG/MKII, 2=XL, 3=XL+, VALUE=1, DROPDOWN=YES, EXPAND=HORIZONTAL");
    IupSetHandle("midi_dev", entry_1);
    IupSetHandle("axe_type", entry_2);
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
    label_3  = IupLabel("Detected:");
    entry_1  = IupText(NULL);
    button_1 = IupButton("Browse...", NULL);
    entry_2  = IupText(NULL);
    entry_3  = IupLabel("Type could not be detected");

    IupSetAttributes(entry_1, "EXPAND=HORIZONTAL");
    IupSetAttributes(entry_2, "EXPAND=HORIZONTAL, SPIN=YES, SPINMIN=0, SPINMAX=384, ACTIVE=NO");
    IupSetHandle("send_file", entry_1);
    IupSetHandle("send_loc", entry_2);
    IupSetHandle("send_type", entry_3);
    IupSetCallback(button_1, "ACTION", (Icallback)openFile_cb);

    sendtab = IupGridBox(label_1, entry_1, button_1,
                         label_2, entry_2, IupSpace(),
                         label_3, entry_3, IupSpace(), NULL);
    IupSetAttributes(sendtab, "ALIGNMENTLIN=ACENTER, ALIGNMENTCOL=ARIGHT,"
                              "GAPLIN=20, GAPCOL=5, SIZELIN=-1, NUMDIV=3, CMARGIN=5x5");
    IupSetAttribute(sendtab, "TABTITLE", "SEND");

    /* RECEIVE TAB */
    label_1  = IupLabel("Directory:");
    label_2  = IupLabel("Location:");
    label_3  = IupLabel("Type:");
    entry_1  = IupText(NULL);
    button_1 = IupButton("Browse...", NULL);
    entry_2  = IupText(NULL);
    IupSetAttributes(entry_1, "EXPAND=HORIZONTAL");
    IupSetAttributes(entry_2, "EXPAND=HORIZONTAL, SPIN=YES, SPINMIN=0, SPINMAX=384");
    IupSetHandle("recv_dir", entry_1);
    IupSetHandle("recv_loc", entry_2);
    IupSetCallback(button_1, "ACTION", (Icallback)openDir_cb);

    select_1 = IupSetAttributes(IupToggle("PRESET", NULL), "EXPAND=HORIZONTAL");
    select_2 = IupSetAttributes(IupToggle("IR", NULL), "EXPAND=HORIZONTAL");
    entry_3  = IupRadio(IupHbox(select_1, select_2, NULL));
    IupSetHandle("preset", select_1);
    IupSetHandle("ir", select_2);
    IupSetHandle("type_opt", entry_3);

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
    menu_1 = IupItem("Notes", NULL);
    IupSetCallback(menu_1, "ACTION", (Icallback)show_notes_cb);
    topmenu_1 = IupMenu(menu_1, NULL);
    menu_1 = IupMenu(IupSubmenu("Help", topmenu_1), NULL);

    /* DIALOG */
    dlg = IupDialog(box_1);
    IupSetAttributes(dlg, "TITLE=\"AXE-FX II LOADER\", MAXSIZE=350x500, RESIZE=NO");
    IupSetAttributeHandle(dlg, "MENU", menu_1);

    IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();
    IupClose();
    if (devs != NULL) free_axe_midi_devs(devs);
    return EXIT_SUCCESS;
}
