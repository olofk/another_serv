/* Raw - Another World Interpreter
 * Copyright (C) 2004 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define BYTE_PER_PIXEL 3

struct PlayerInput {
	uint8_t dirMask;
	bool button;
	bool code;
	bool quit;
	char lastChar;
};

void sysprocessEvents(struct PlayerInput *input);
void sysinit();
void syssetPalette(const uint8_t *buf);
void sysdestroy();
void sysupdateDisplay(const uint8_t *buf);
void sysSleep(uint32_t duration);
uint32_t sysGetTimeStamp();
#ifdef __cplusplus
}
#endif
#endif
