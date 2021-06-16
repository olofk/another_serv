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

#include <zephyr.h>
#include <string.h>
#include "vm.h"
#include "resource.h"
#include "video.h"
#include "sys.h"
#include "parts.h"

extern uint16_t currentPartId;
extern uint8_t *segCinematic;
extern uint8_t *_segVideo2;
extern uint16_t requestedNextPart;
extern bool _useSegVideo2;

static inline uint16_t READ_BE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 8) | b[1];
}

static int16_t vmVariables[VM_NUM_VARIABLES];
static uint16_t threadsData[NUM_DATA_FIELDS][VM_NUM_THREADS];
static uint8_t _stackPtr;
static uint16_t _scriptStackCalls[VM_NUM_THREADS];
static bool gotoNextThread;
static void executeThread();

	// This array is used: 
	//     0 to save the channel's instruction pointer 
	//     when the channel release control (this happens on a break).
	//     1 When a setVec is requested for the next vm frame.
static uint8_t vmIsChannelActive[NUM_THREAD_FIELDS][VM_NUM_THREADS];
void vminit() {

	__builtin_memset(vmVariables, 0, sizeof(vmVariables));
	vmVariables[0x54] = 0x81;
	vmVariables[VM_VARIABLE_RANDOM_SEED] = 0 /*time(0)*/;
#define BYPASS_PROTECTION 1
#ifdef BYPASS_PROTECTION
   // these 3 variables are set by the game code
   vmVariables[0xBC] = 0x10;
   vmVariables[0xC6] = 0x80;
   vmVariables[0xF2] = 4000;
   // these 2 variables are set by the engine executable
   vmVariables[0xDC] = 33;
#endif

}
static uint8_t *_scriptPtr;

void op_movConst() {//3
	uint8_t variableId = _scriptPtr[0];
	int16_t value = READ_BE_UINT16(_scriptPtr+1);
	vmVariables[variableId] = value;

	_scriptPtr += 3;
}

void op_mov() {//2
	uint8_t dstVariableId = _scriptPtr[0];
	uint8_t srcVariableId = _scriptPtr[1];	
	vmVariables[dstVariableId] = vmVariables[srcVariableId];

	_scriptPtr += 2;
}

void op_add() {//2
	uint8_t dstVariableId = *_scriptPtr++;
	uint8_t srcVariableId = *_scriptPtr++;
	vmVariables[dstVariableId] += vmVariables[srcVariableId];
}

void op_addConst() {//3
	uint8_t variableId = *_scriptPtr++;
	int16_t value = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	vmVariables[variableId] += value;
}
extern uint8_t *segBytecode;

void op_call() {//2

	uint16_t offset = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	uint8_t sp = _stackPtr;

	_scriptStackCalls[sp] = _scriptPtr - segBytecode;
	++_stackPtr;
	_scriptPtr = segBytecode + offset ;
}

void op_ret() {//0
	--_stackPtr;
	uint8_t sp = _stackPtr;
	_scriptPtr = segBytecode + _scriptStackCalls[sp];
}

void op_pauseThread() {//0
	gotoNextThread = true;
}

void op_jmp() {//2
	uint16_t pcOffset = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	_scriptPtr = segBytecode + pcOffset;	
}

void op_setSetVect() {//3
	uint8_t threadId = *_scriptPtr++;
	uint16_t pcOffsetRequested = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	threadsData[REQUESTED_PC_OFFSET][threadId] = pcOffsetRequested;
}

void op_jnz() {//3
	uint8_t i = *_scriptPtr++;
	--vmVariables[i];
	if (vmVariables[i] != 0) {
		op_jmp();
	} else {
	  //	uint16_t pcOffsetRequested = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	}
}

void op_condJmp() {
	uint8_t opcode = *_scriptPtr++;
  const uint8_t var = *_scriptPtr++;
  int16_t b = vmVariables[var];
	int16_t a;
	//2
	if (opcode & 0x80) {
	  a = vmVariables[*_scriptPtr++];
	} else if (opcode & 0x40) {
	  a = READ_BE_UINT16(_scriptPtr);
	  _scriptPtr += 2;
	} else {
	  a = *_scriptPtr++;
	}
	//3/4
	// Check if the conditional value is met.
	bool expr = false;
	switch (opcode & 7) {
	case 0:	// jz
		expr = (b == a);
#define BYPASS_PROTECTION 1
#ifdef BYPASS_PROTECTION
      if (currentPartId == 16000) {
        //
        // 0CB8: jmpIf(VAR(0x29) == VAR(0x1E), @0CD3)
        // ...
        //
        if (b == 0x29 && (opcode & 0x80) != 0) {
          // 4 symbols
          vmVariables[0x29] = vmVariables[0x1E];
          vmVariables[0x2A] = vmVariables[0x1F];
          vmVariables[0x2B] = vmVariables[0x20];
          vmVariables[0x2C] = vmVariables[0x21];
          // counters
          vmVariables[0x32] = 6;
          vmVariables[0x64] = 20;
          expr = true;
        }
      }
#endif
		break;
	case 1: // jnz
		expr = (b != a);
		break;
	case 2: // jg
		expr = (b > a);
		break;
	case 3: // jge
		expr = (b >= a);
		break;
	case 4: // jl
		expr = (b < a);
		break;
	case 5: // jle
		expr = (b <= a);
		break;
	default:
		break;
	}

	if (expr) {
		op_jmp();
	} else {
	_scriptPtr += 2;
	}
	//3/4/6
}

extern uint8_t paletteIdRequested;
void op_setPalette() {//2
	uint16_t paletteId = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	paletteIdRequested = paletteId >> 8;
}

void op_resetThread() {//2/3 ??

	uint8_t threadId = *_scriptPtr++;
	uint8_t i =        *_scriptPtr++;

	// FCS: WTF, this is cryptic as hell !!
	//int8_t n = (i & 0x3F) - threadId;  //0x3F = 0011 1111
	// The following is so much clearer

	//Make sure i within [0-VM_NUM_THREADS-1]
	i = i & (VM_NUM_THREADS-1) ;
	int8_t n = i - threadId;

	if (n < 0) {
		return;
	}
	++n;
	uint8_t a = *_scriptPtr++;


	if (a == 2) {
		uint16_t *p = &threadsData[REQUESTED_PC_OFFSET][threadId];
		while (n--) {
			*p++ = 0xFFFE;
		}
	} else if (a < 2) {
		uint8_t *p = &vmIsChannelActive[REQUESTED_STATE][threadId];
		while (n--) {
			*p++ = a;
		}
	}
}

void op_selectVideoPage() {//1
	uint8_t frameBufferId = *_scriptPtr++;
	videochangePagePtr1(frameBufferId);
}

void op_fillVideoPage() {//2
	uint8_t pageId = *_scriptPtr++;
	uint8_t color = *_scriptPtr++;
	uint8_t *p = videogetPage(pageId);

	// Since a palette indice is coded on 4 bits, we need to duplicate the
	// clearing color to the upper part of the byte.
	uint8_t c = (color << 4) | color;

	__builtin_memset(p, c, VID_PAGE_SIZE);
}

void op_copyVideoPage() {//2
	uint8_t srcPageId = *_scriptPtr++;
	uint8_t dstPageId = *_scriptPtr++;
	videocopyPage(srcPageId, dstPageId, vmVariables[VM_VARIABLE_SCROLL_Y]);
}


uint32_t lastTimeStamp = 0;
void op_blitFramebuffer() {//1

	uint8_t pageId = *_scriptPtr++;
	//	inp_handleSpecialKeys();

  int32_t delay = sysGetTimeStamp() - lastTimeStamp;
  int32_t timeToSleep = vmVariables[VM_VARIABLE_PAUSE_SLICES] * 20 - delay;

  // The bytecode will set vmVariables[VM_VARIABLE_PAUSE_SLICES] from 1 to 5
  // The virtual machine hence indicate how long the image should be displayed.

  if (timeToSleep > 0) {
    sysSleep(timeToSleep);
  }

  lastTimeStamp = sysGetTimeStamp();

	//WTF ?
	vmVariables[0xF7] = 0;

	videoupdateDisplay(pageId);
}

void op_killThread() {//0
	_scriptPtr = segBytecode + 0xFFFF;
	gotoNextThread = true;
}

void op_drawString() {//5
	uint16_t stringId = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	uint16_t x = *_scriptPtr++;
	uint16_t y = *_scriptPtr++;
	uint16_t color = *_scriptPtr++;


	videodrawString(color, x, y, stringId);
}

void op_sub() {//2
	uint8_t i = *_scriptPtr++;
	uint8_t j = *_scriptPtr++;
	vmVariables[i] -= vmVariables[j];
}

void op_and() {//3
	uint8_t variableId = *_scriptPtr++;
	uint16_t n = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] & n;
}

void op_or() {//3
	uint8_t variableId = *_scriptPtr++;
	uint16_t value = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] | value;
}

void op_shl() {//3
	uint8_t variableId = *_scriptPtr++;
	uint16_t leftShiftValue = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] << leftShiftValue;
}

void op_shr() {//3
	uint8_t variableId = *_scriptPtr++;
	uint16_t rightShiftValue = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;
	vmVariables[variableId] = (uint16_t)vmVariables[variableId] >> rightShiftValue;
}

void op_playSound() {//5
  /*uint16_t resourceId =*/ _scriptPtr += 2;
	/*uint8_t freq =    */ *_scriptPtr++;
	/*uint8_t vol =     */ *_scriptPtr++;
	/*uint8_t channel = */ *_scriptPtr++;
}

void op_updateMemList() {//2

	uint16_t resourceId = READ_BE_UINT16(_scriptPtr);
	_scriptPtr += 2;

	if (resourceId == 0) {
		resinvalidateRes();
	} else {
		resloadPartsOrMemoryEntry(resourceId);
	}
}

void op_playMusic() {
  _scriptPtr += 5;
}

void vminitForPart(uint16_t partId) {


	//WTF is that ?
	vmVariables[0xE4] = 0x14;

	ressetupPart(partId);

	//Set all thread to inactive (pc at 0xFFFF or 0xFFFE )
	memset((uint8_t *)threadsData, 0xFF, sizeof(threadsData));


	memset((uint8_t *)vmIsChannelActive, 0, sizeof(vmIsChannelActive));
	
	int firstThreadId = 0;
	threadsData[PC_OFFSET][firstThreadId] = 0;	
}

/* 
     This is called every frames in the infinite loop.
*/
void vmcheckThreadRequests() {

	//Check if a part switch has been requested.
	if (requestedNextPart != 0) {
		vminitForPart(requestedNextPart);
		requestedNextPart = 0;
	}

	
	// Check if a state update has been requested for any thread during the previous VM execution:
	//      - Pause
	//      - Jump

	// JUMP:
	// Note: If a jump has been requested, the jump destination is stored
	// in threadsData[REQUESTED_PC_OFFSET]. Otherwise threadsData[REQUESTED_PC_OFFSET] == 0xFFFF

	// PAUSE:
	// Note: If a pause has been requested it is stored in  vmIsChannelActive[REQUESTED_STATE][i]

	for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

		vmIsChannelActive[CURR_STATE][threadId] = vmIsChannelActive[REQUESTED_STATE][threadId];

		uint16_t n = threadsData[REQUESTED_PC_OFFSET][threadId];

		if (n != VM_NO_SETVEC_REQUESTED) {

			threadsData[PC_OFFSET][threadId] = (n == 0xFFFE) ? VM_INACTIVE_THREAD : n;
			threadsData[REQUESTED_PC_OFFSET][threadId] = VM_NO_SETVEC_REQUESTED;
		}
	}
}

void vmhostFrame() {

	// Run the Virtual Machine for every active threads (one vm frame).
	// Inactive threads are marked with a thread instruction pointer set to 0xFFFF (VM_INACTIVE_THREAD).
	// A thread must feature a break opcode so the interpreter can move to the next thread.

	for (int threadId = 0; threadId < VM_NUM_THREADS; threadId++) {

		if (vmIsChannelActive[CURR_STATE][threadId])
			continue;
		
		uint16_t n = threadsData[PC_OFFSET][threadId];

		if (n != VM_INACTIVE_THREAD) {

			// Set the script pointer to the right location.
			// script pc is used in executeThread in order
			// to get the next opcode.
			_scriptPtr = segBytecode + n;
			_stackPtr = 0;

			gotoNextThread = false;
			executeThread();

			//Since  is going to be modified by this next loop iteration, we need to save it.
			threadsData[PC_OFFSET][threadId] = _scriptPtr - segBytecode;


			/*if (sys->input.quit) {
				break;
				}*/
		}
		
	}
}

#define COLOR_BLACK 0xFF
#define DEFAULT_ZOOM 0x40


void executeThread() {

	while (!gotoNextThread) {
		uint8_t opcode = *_scriptPtr++;

		// 1000 0000 is set
		if (opcode & 0x80) 
		{
			uint16_t off = ((opcode << 8) | *_scriptPtr++) * 2;
			_useSegVideo2 = false;
			int16_t x = *_scriptPtr++;
			int16_t y = *_scriptPtr++;
			int16_t h = y - 199;
			if (h > 0) {
				y = 199;
				x += h;
			}

			// This switch the polygon database to "cinematic" and probably draws a black polygon
			// over all the screen.
			videosetDataBuffer(segCinematic, off);
			videoreadAndDrawPolygon(COLOR_BLACK, DEFAULT_ZOOM, x,y);

			continue;
		} 

		// 0100 0000 is set
		if (opcode & 0x40) 
		{
			int16_t x, y;
			uint16_t off = READ_BE_UINT16(_scriptPtr) * 2;
			_scriptPtr += 2;
			x = *_scriptPtr++;

			_useSegVideo2 = false;

			if (!(opcode & 0x20)) 
			{
				if (!(opcode & 0x10))  // 0001 0000 is set
				{
					x = (x << 8) | *_scriptPtr++;
				} else {
					x = vmVariables[x];
				}
			} 
			else 
			{
				if (opcode & 0x10) { // 0001 0000 is set
					x += 0x100;
				}
			}

			y = *_scriptPtr++;

			if (!(opcode & 8))  // 0000 1000 is set
			{
				if (!(opcode & 4)) { // 0000 0100 is set
					y = (y << 8) | *_scriptPtr++;
				} else {
					y = vmVariables[y];
				}
			}

			uint16_t zoom = *_scriptPtr++;

			if (!(opcode & 2))  // 0000 0010 is set
			{
				if (!(opcode & 1)) // 0000 0001 is set
				{
					--_scriptPtr;
					zoom = 0x40;
				} 
				else 
				{
					zoom = vmVariables[zoom];
				}
			} 
			else 
			{
				
				if (opcode & 1) { // 0000 0001 is set
					_useSegVideo2 = true;
					--_scriptPtr;
					zoom = 0x40;
				}
			}
			videosetDataBuffer(_useSegVideo2 ? _segVideo2 : segCinematic, off);
			videoreadAndDrawPolygon(0xFF, zoom, x, y);

			continue;
		} 

		switch(opcode) {
		case 0x00 : op_movConst(); break;
		case 0x01 : op_mov(); break;
		case 0x02 : op_add(); break;
		case 0x03 : op_addConst(); break;
		    /* 0x04 */
		case 0x04 : op_call(); break;
		case 0x05 : op_ret(); break;
		case 0x06 : op_pauseThread(); break;
		case 0x07 : op_jmp(); break;
		    /* 0x08 */
		case 0x08 : op_setSetVect(); break;
		case 0x09 : op_jnz(); break;
		case 0x0a : op_condJmp(); break;
		case 0x0b : op_setPalette(); break;
		    /* 0x0C */
		case 0x0c : op_resetThread(); break;
		case 0x0d : op_selectVideoPage(); break;
		case 0x0e : op_fillVideoPage(); break;
		case 0x0f : op_copyVideoPage(); break;
		    /* 0x10 */
		case 0x10 : op_blitFramebuffer(); break;
		case 0x11 : op_killThread(); break;
		case 0x12 : op_drawString(); break;
		case 0x13 : op_sub(); break;
		    /* 0x14 */
		case 0x14 : op_and(); break;
		case 0x15 : op_or(); break;
		case 0x16 : op_shl(); break;
		case 0x17 : op_shr(); break;
		    /* 0x18 */
		case 0x18 : op_playSound(); break;
		case 0x19 : op_updateMemList(); break;
		case 0x1a : op_playMusic(); break;
		}
	}
}
#define DIR_LEFT  (1 << 0)
#define DIR_RIGHT (1 << 1)
#define DIR_UP    (1 << 2)
#define DIR_DOWN  (1 << 3)

void vminp_updatePlayer(struct PlayerInput *input) {

	if (currentPartId == 0x3E89) {
		char c = input->lastChar;
		if (c == 8 || /*c == 0xD |*/ c == 0 || (c >= 'a' && c <= 'z')) {
			vmVariables[VM_VARIABLE_LAST_KEYCHAR] = c & ~0x20;
			input->lastChar = 0;
		}
	}

	int16_t lr = 0;
	int16_t m = 0;
	int16_t ud = 0;

	if (input->dirMask & DIR_RIGHT) {
		lr = 1;
		m |= 1;
	}
	if (input->dirMask & DIR_LEFT) {
		lr = -1;
		m |= 2;
	}
	if (input->dirMask & DIR_DOWN) {
		ud = 1;
		m |= 4;
	}

	vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = ud;

	if (input->dirMask & DIR_UP) {
		vmVariables[VM_VARIABLE_HERO_POS_UP_DOWN] = -1;
	}

	if (input->dirMask & DIR_UP) { // inpJump
		ud = -1;
		m |= 8;
	}

	vmVariables[VM_VARIABLE_HERO_POS_JUMP_DOWN] = ud;
	vmVariables[VM_VARIABLE_HERO_POS_LEFT_RIGHT] = lr;
	vmVariables[VM_VARIABLE_HERO_POS_MASK] = m;
	int16_t button = 0;

	if (input->button) { // inpButton
		button = 1;
		m |= 0x80;
	}

	vmVariables[VM_VARIABLE_HERO_ACTION] = button;
	vmVariables[VM_VARIABLE_HERO_ACTION_POS_MASK] = m;
}

