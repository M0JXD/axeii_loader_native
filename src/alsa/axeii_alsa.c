/*    axeii_alsa.c - ALSA backend for AxeII-Loader
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

#include <alsa/asoundlib.h>
#include "../midi_interface.h"

/* GLOBALS */
static snd_rawmidi_t *handleIn, *handleOut;

/* FUNCTIONS */

char initMIDI(char *devString) {
    /* TODO: This call leaks? */
    char err = snd_rawmidi_open(&handleIn, &handleOut, devString, 0);
    if (err != 0) {
        return 1;
    }
    /* Blocking mode */
    snd_rawmidi_nonblock(handleIn, 0);
    /*snd_rawmidi_nonblock(handleOut, 0);*/
    return 0;
}

char closeMIDI(void) {
    snd_rawmidi_close(handleIn);
    snd_rawmidi_close(handleOut);
    return 0;
}

char sendMidi(unsigned char *data, unsigned int len) {
    snd_rawmidi_write(handleOut, data, len);
    return 0;
}

char getMidi(unsigned char *data, unsigned int len) {
    int read = 0;
	do {
		read += snd_rawmidi_read(handleIn, &data[read], len - read);
		if (read < 0) {
			return -1;
		}
	} while (read != (int)len);
    return 0;
}

char clearMidiInBuffer(void) {
    snd_rawmidi_drop(handleIn);
    return 0;
}

/* This rest of this file is modified and reduced from the amidi.c source code
 * to implement device checking. Note it only checks the first five devices.
 * Original license is below.
 */

/*
 *  amidi.c - read from/write to RawMIDI ports
 *
 *  Copyright (c) Clemens Ladisch <clemens@ladisch.de>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

static void error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	putc('\n', stderr);
}

static int list_device(snd_ctl_t *ctl, int card, int device, dev_info_t *dev)
{
	snd_rawmidi_info_t *info;
	const char *name;
	const char *sub_name;
	int subs, subs_in, subs_out;
	int sub;
	int err;
	int amount = 0;

	snd_rawmidi_info_alloca(&info);
	snd_rawmidi_info_set_device(info, device);

	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
	err = snd_ctl_rawmidi_info(ctl, info);
	if (err >= 0)
		subs_in = snd_rawmidi_info_get_subdevices_count(info);
	else
		subs_in = 0;

	snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
	err = snd_ctl_rawmidi_info(ctl, info);
	if (err >= 0)
		subs_out = snd_rawmidi_info_get_subdevices_count(info);
	else
		subs_out = 0;

	subs = subs_in > subs_out ? subs_in : subs_out;
	if (!subs)
		return 0;

	for (sub = 0; sub < subs; ++sub) {
		snd_rawmidi_info_set_stream(info, sub < subs_in ?
					    SND_RAWMIDI_STREAM_INPUT :
					    SND_RAWMIDI_STREAM_OUTPUT);
		snd_rawmidi_info_set_subdevice(info, sub);
		err = snd_ctl_rawmidi_info(ctl, info);
		if (err < 0) {
			error("cannot get rawmidi information %d:%d:%d: %s\n",
			      card, device, sub, snd_strerror(err));
			return 0;
		}

        /* At time of development, this is a bleeding edge (commit 2 days old!) change to filter inactive ports */
        /* It requires Kernel 6.14 or later so it is omitted. */
		/*if (!list_all &&*/
		/*    (snd_rawmidi_info_get_flags(info) & SNDRV_RAWMIDI_INFO_STREAM_INACTIVE))*/
		/*	continue;*/
		name = snd_rawmidi_info_get_name(info);
		sub_name = snd_rawmidi_info_get_subdevice_name(info);
		if (sub == 0 && sub_name[0] == '\0') {
			/*printf("%c%c  hw:%d,%d    %s",*/
			/*       sub < subs_in ? 'I' : ' ',*/
			/*       sub < subs_out ? 'O' : ' ',*/
			/*       card, device, name);*/
			sprintf(dev->hw_string, "hw:%d,%d", card, device);
			sprintf(dev->hw_name, "%s", name);
			if (subs > 1)
				/*printf(" (%d subdevices)", subs);*/
			/*putchar('\n');*/
			break;
		} else {
			/*printf("%c%c  hw:%d,%d,%d  %s\n",*/
			/*       sub < subs_in ? 'I' : ' ',*/
			/*       sub < subs_out ? 'O' : ' ',*/
			/*       card, device, sub, sub_name);*/
			sprintf(dev->hw_string, "hw:%d,%d,%d", card, device, sub);
			sprintf(dev->hw_name, "%s", sub_name);
		}
		amount++;
	}
	return amount;
}

static int list_card_devices(int card, dev_info_t **devs)
{
	snd_ctl_t *ctl;
	char name[32];
	int device;
	int err;
	int amount = 0;

	sprintf(name, "hw:%d", card);
	if ((err = snd_ctl_open(&ctl, name, 0)) < 0) {
		error("cannot open control for card %d: %s", card, snd_strerror(err));
		return 0;
	}
	device = -1;
	for (;;) {
		if ((err = snd_ctl_rawmidi_next_device(ctl, &device)) < 0) {
			error("cannot determine device number: %s", snd_strerror(err));
			break;
		}
		if (device < 0)
			break;
		amount += list_device(ctl, card, device, devs[device]);
	}
	snd_ctl_close(ctl);
	return amount;
}


/* Essentially amidi -l start point */
static int device_list(dev_info_t** devs)
{
	int card, err, amount = 0;

	card = -1;
	if ((err = snd_card_next(&card)) < 0) {
		error("cannot determine card number: %s", snd_strerror(err));
		return -1;
	}
	if (card < 0) {
		error("no sound card found");
		return -1;
	}
	/*puts("Dir Device    Name");*/
	do {
		amount += list_card_devices(card, devs);
		if ((err = snd_card_next(&card)) < 0) {
			error("cannot determine card number: %s", snd_strerror(err));
			break;
		}
        if (amount == 5) break;
	} while (card >= 0);
    return amount;
}

dev_info_t** getAxeMidiDevs(int *amount, int *axe_index)
{
    dev_info_t **devs;
    *axe_index = -1;

    devs = (dev_info_t**)malloc(sizeof(dev_info_t*) * 5);
    devs[0] = (dev_info_t*)malloc(sizeof(dev_info_t) * 5);

    *amount = device_list(devs);

    for (int i = 0; i < *amount; i++) {
        if (strstr(devs[0]->hw_name, "AXE") != NULL) {
            *axe_index = i;
            break;
        }
    }
    return devs;
}

void freeAxeMidiDevs(dev_info_t **devs)
{
    free(devs[0]);
    free(devs);
    devs = NULL;
}
