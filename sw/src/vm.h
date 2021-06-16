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

#ifndef __LOGIC_H__
#define __LOGIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "sys.h"

#define VM_NUM_THREADS 64
#define VM_NUM_VARIABLES 256
#define VM_NO_SETVEC_REQUESTED 0xFFFF
#define VM_INACTIVE_THREAD    0xFFFF


enum ScriptVars {
		VM_VARIABLE_RANDOM_SEED          = 0x3C,
		
		VM_VARIABLE_LAST_KEYCHAR         = 0xDA,

		VM_VARIABLE_HERO_POS_UP_DOWN     = 0xE5,

		VM_VARIABLE_MUS_MARK             = 0xF4,

		VM_VARIABLE_SCROLL_Y             = 0xF9, // = 239
		VM_VARIABLE_HERO_ACTION          = 0xFA,
		VM_VARIABLE_HERO_POS_JUMP_DOWN   = 0xFB,
		VM_VARIABLE_HERO_POS_LEFT_RIGHT  = 0xFC,
		VM_VARIABLE_HERO_POS_MASK        = 0xFD,
		VM_VARIABLE_HERO_ACTION_POS_MASK = 0xFE,
		VM_VARIABLE_PAUSE_SLICES         = 0xFF
	};

//For threadsData navigation
#define PC_OFFSET 0
#define REQUESTED_PC_OFFSET 1
#define NUM_DATA_FIELDS 2

//For vmIsChannelActive navigation
#define CURR_STATE 0
#define REQUESTED_STATE 1
#define NUM_THREAD_FIELDS 2

void vminit();
void vmhostFrame();
void vmcheckThreadRequests();
void vminitForPart(uint16_t partId);
void vminp_updatePlayer(struct PlayerInput *input);

#ifdef __cplusplus
}
#endif
#endif
