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

//Stripped down sysImplementation for another_serv

#include <zephyr.h>
#include "sys.h"

#define NUM_COLORS 16

void sysinit() {}

void sysdestroy() {}

void syssetPalette(const uint8_t *p) {
  // The incoming palette is in 565 format.
  for (int i = 0; i < NUM_COLORS; ++i)
  {
    uint8_t c1 = *(p + 0);
    uint8_t c2 = *(p + 1);
    sys_write16(c1 | (c2<<8), 0x20000000+i*4);
    p += 2;
  }
}

void sysupdateDisplay(const uint8_t *src) {
  //Verilator testbench updates the screen contents from the src pointer
  sys_write32(src, 0x10000000);
}

void sysprocessEvents(struct PlayerInput *input) {}

void sysSleep(uint32_t duration) {
  /*SDL_Delay(duration);*/
}

uint32_t sysGetTimeStamp() {
  return 0;
  //return SDL_GetTicks();	
}
