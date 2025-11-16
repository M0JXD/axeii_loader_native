/*    midi_devs.h - Structures and declarations for midi_devs.c
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

#ifndef MIDI_DEVS_H
#define MIDI_DEVS_H

typedef struct DEV_INFO_S {
    char hw_string[32];
    char hw_name[120];
} dev_info_t;

dev_info_t** get_axe_midi_devs(int *amount, int *axe_index);
void free_axe_midi_devs(dev_info_t **devs);

#endif  /* MIDI_DEVS_H */
