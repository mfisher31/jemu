/*
    This file is part of Jemu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JEMU_CONTROL_UP     = 0,
    JEMU_CONTROL_DOWN   = 1,
    JEMU_CONTROL_LEFT   = 2,
    JEMU_CONTROL_RIGHT  = 3,
    JEMU_CONTROL_A      = 4,
    JEMU_CONTROL_B      = 5,
    JEMU_CONTROL_X      = 6,
    JEMU_CONTROL_Y      = 7,
    JEMU_CONTROL_L1     = 8,
    JEMU_CONTROL_R1     = 9,
    JEMU_CONTROL_L2     = 10,
    JEMU_CONTROL_R2     = 11,
    JEMU_CONTROL_SELECT = 12,
    JEMU_CONTROL_START  = 13
} JemuControl;

#ifdef __cplusplus
}
#endif
