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

#include "resource.h"
#include "bank.h"
#include "video.h"
#include "parts.h"

#define RT_POLY_ANIM  2 // full screen video buffer, size=0x7D00 

static uint8_t *_memPtrStart, *_scriptBakPtr, *_scriptCurPtr, *_vidBakPtr, *_vidCurPtr;
static struct MemEntry _memList[150];
uint16_t _numMemList;

extern uint16_t currentPartId;
extern uint8_t *_segVideo2;
extern uint16_t requestedNextPart;

const uint16_t memListParts[GAME_NUM_PARTS][4] = {

//MEMLIST_PART_PALETTE   MEMLIST_PART_CODE   MEMLIST_PART_VIDEO1   MEMLIST_PART_VIDEO2
	{ 0x14,                    0x15,                0x16,                0x00 }, // protection screens
	{ 0x17,                    0x18,                0x19,                0x00 }, // introduction cinematic
	{ 0x1A,                    0x1B,                0x1C,                0x11 },
	{ 0x1D,                    0x1E,                0x1F,                0x11 },
	{ 0x20,                    0x21,                0x22,                0x11 },
	{ 0x23,                    0x24,                0x25,                0x00 }, // battlechar cinematic
	{ 0x26,                    0x27,                0x28,                0x11 },
	{ 0x29,                    0x2A,                0x2B,                0x11 },
	{ 0x7D,                    0x7E,                0x7F,                0x00 },
	{ 0x7D,                    0x7E,                0x7F,                0x00 }  // password screen
};


static const uint8_t _binary_bank01_start[] = {
#include "bank01.inc"
};
static const uint8_t _binary_bank02_start[] = {
#include "bank02.inc"
};
static const uint8_t _binary_bank03_start[] = {
#include "bank03.inc"
};
static const uint8_t _binary_bank04_start[] = {
#include "bank04.inc"
};
static const uint8_t _binary_bank05_start[] = {
#include "bank05.inc"
};
static const uint8_t _binary_bank06_start[] = {
#include "bank06.inc"
};
static const uint8_t _binary_bank07_start[] = {
#include "bank07.inc"
};
static const uint8_t _binary_bank08_start[] = {
#include "bank08.inc"
};
static const uint8_t _binary_bank09_start[] = {
#include "bank09.inc"
};
static const uint8_t _binary_bank0a_start[] = {
#include "bank0a.inc"
};
static const uint8_t _binary_bank0b_start[] = {
#include "bank0b.inc"
};
static const uint8_t _binary_bank0c_start[] = {
#include "bank0c.inc"
};
static const uint8_t _binary_bank0d_start[] = {
#include "bank0d.inc"
};

static void invalidateAll();
static void loadMarkedAsNeeded();
void readBank(const struct MemEntry *me, uint8_t *dstBuf) {

  const uint8_t *bank;
  switch (me->bankId) {
  case 0x1 : bank = _binary_bank01_start; break;
  case 0x2 : bank = _binary_bank02_start; break;
  case 0x3 : bank = _binary_bank03_start; break;
  case 0x4 : bank = _binary_bank04_start; break;
  case 0x5 : bank = _binary_bank05_start; break;
  case 0x6 : bank = _binary_bank06_start; break;
  case 0x7 : bank = _binary_bank07_start; break;
  case 0x8 : bank = _binary_bank08_start; break;
  case 0x9 : bank = _binary_bank09_start; break;
  case 0xa : bank = _binary_bank0a_start; break;
  case 0xb : bank = _binary_bank0b_start; break;
  case 0xc : bank = _binary_bank0c_start; break;
  case 0xd : bank = _binary_bank0d_start; break;
  default  : return;
    //  default: printf("ERROR");
  }

  __builtin_memcpy(dstBuf, bank+me->bankOffset, me->packedSize);
  if (me->packedSize != me->size)
    bankread(me->packedSize, dstBuf);
}

#define RES_SIZE 0
#define RES_COMPRESSED 1
int resourceSizeStats[7][2];
#define STATS_TOTAL_SIZE 6
int resourceUnitStats[7][2];

static const uint8_t _binary_memlist_bin_start[] = {
#include "memlist.bin.inc"
};
uint8_t *mptr;
/*
	Read all entries from memlist.bin. Do not load anything in memory,
	this is just a fast way to access the data later based on their id.
*/
#define FILE uint8_t

uint8_t readByte(FILE *f) {
  return *(mptr++);
}

uint16_t readUint16BE(FILE *f) {
  uint8_t hi = readByte(f);
  uint8_t lo = readByte(f);
  return (hi << 8) | lo;
}

uint32_t readUint32BE(FILE *f) {
  uint16_t hi = readUint16BE(f);
  uint16_t lo = readUint16BE(f);
  return (hi << 16) | lo;
}

void resreadEntries() {	
	int resourceCounter = 0;

	FILE *f;

	mptr = _binary_memlist_bin_start;

	_numMemList = 0;
	struct MemEntry *memEntry = _memList;
	while (1) {
		memEntry->state = readByte(f);
		memEntry->type = readByte(f);
		memEntry->bufPtr = 0; readUint16BE(f);
		memEntry->unk4 = readUint16BE(f);
		memEntry->rankNum = readByte(f);
		memEntry->bankId = readByte(f);
		memEntry->bankOffset = readUint32BE(f);
		memEntry->unkC = readUint16BE(f);
		memEntry->packedSize = readUint16BE(f);
		memEntry->unk10 = readUint16BE(f);
		memEntry->size = readUint16BE(f);

    if (memEntry->state == MEMENTRY_STATE_END_OF_MEMLIST) {
      break;
    }

		resourceCounter++;

		_numMemList++;
		memEntry++;
	}

}

/*
	Go over every resource and check if they are marked at "MEMENTRY_STATE_LOAD_ME".
	Load them in memory and mark them are MEMENTRY_STATE_LOADED
*/
#define NULL 0 //Avoid stddef
void loadMarkedAsNeeded() {

	while (1) {
		
		struct MemEntry *me = NULL;

		// get resource with max rankNum
		uint8_t maxNum = 0;
		uint16_t i = _numMemList;
		struct MemEntry *it = _memList;
		while (i--) {
			if (it->state == MEMENTRY_STATE_LOAD_ME && maxNum <= it->rankNum) {
				maxNum = it->rankNum;
				me = it;
			}
			it++;
		}
		
		if (me == NULL) {
			break; // no entry found
		}

		
		// At this point the resource descriptor should be pointed to "me"
		// "That's what she said"

		uint8_t *loadDestination = NULL;
		if (me->type == RT_POLY_ANIM) {
			loadDestination = _vidCurPtr;
		} else {
			loadDestination = _scriptCurPtr;
			if (me->size > _vidBakPtr - _scriptCurPtr) {
				me->state = MEMENTRY_STATE_NOT_NEEDED;
				continue;
				}
		}


		if (me->bankId == 0) {
			me->state = MEMENTRY_STATE_NOT_NEEDED;
		} else {
			readBank(me, loadDestination);
			if(me->type == RT_POLY_ANIM) {
        videosetBitmap(_vidCurPtr);
				me->state = MEMENTRY_STATE_NOT_NEEDED;
			} else {
				me->bufPtr = loadDestination;
				me->state = MEMENTRY_STATE_LOADED;
				_scriptCurPtr += me->size;
			}
		}

	}


}
//Called from vm.cpp
void resinvalidateRes() {
	struct MemEntry *me = _memList;
	uint16_t i = _numMemList;
	while (i--) {
		if (me->type <= RT_POLY_ANIM || me->type > 6) {  // 6 WTF ?!?! ResType goes up to 5 !!
		  me->state = MEMENTRY_STATE_NOT_NEEDED;
		}
		++me;
	}
	_scriptCurPtr = _scriptBakPtr;
}

void invalidateAll() {
	struct MemEntry *me = _memList;
	uint16_t i = _numMemList;
	while (i--) {
		me->state = MEMENTRY_STATE_NOT_NEEDED;
		++me;
	}
	_scriptCurPtr = _memPtrStart;
}

/* This method serves two purpose: 
    - Load parts in memory segments (palette,code,video1,video2)
	           or
    - Load a resource in memory

	This is decided based on the resourceId. If it does not match a mementry id it is supposed to 
	be a part id. */
void resloadPartsOrMemoryEntry(uint16_t resourceId) {

	if (resourceId > _numMemList) {

		requestedNextPart = resourceId;

	} else {

		struct MemEntry *me = &_memList[resourceId];

		if (me->state == MEMENTRY_STATE_NOT_NEEDED) {
			me->state = MEMENTRY_STATE_LOAD_ME;
			loadMarkedAsNeeded();
		}
	}

}

extern uint8_t *segPalettes;
extern uint8_t *segBytecode;
extern uint8_t *segCinematic;
/* Protection screen and cinematic don't need the player and enemies polygon data
   so _memList[video2Index] is never loaded for those parts of the game. When 
   needed (for action phrases) _memList[video2Index] is always loaded with 0x11 
   (as seen in memListParts). */
void ressetupPart(uint16_t partId) {

	

	if (partId == currentPartId)
		return;

	uint16_t memListPartIndex = partId - GAME_PART_FIRST;

	uint8_t paletteIndex = memListParts[memListPartIndex][MEMLIST_PART_PALETTE];
	uint8_t codeIndex    = memListParts[memListPartIndex][MEMLIST_PART_CODE];
	uint8_t videoCinematicIndex  = memListParts[memListPartIndex][MEMLIST_PART_POLY_CINEMATIC];
	uint8_t video2Index  = memListParts[memListPartIndex][MEMLIST_PART_VIDEO2];

	// Mark all resources as located on harddrive.
	invalidateAll();

	_memList[paletteIndex].state = MEMENTRY_STATE_LOAD_ME;
	_memList[codeIndex].state = MEMENTRY_STATE_LOAD_ME;
	_memList[videoCinematicIndex].state = MEMENTRY_STATE_LOAD_ME;

	// This is probably a cinematic or a non interactive part of the game.
	// Player and enemy polygons are not needed.
	if (video2Index != MEMLIST_PART_NONE) 
		_memList[video2Index].state = MEMENTRY_STATE_LOAD_ME;
	

	loadMarkedAsNeeded();

	segPalettes = _memList[paletteIndex].bufPtr;
	segBytecode     = _memList[codeIndex].bufPtr;
	segCinematic   = _memList[videoCinematicIndex].bufPtr;



	// This is probably a cinematic or a non interactive part of the game.
	// Player and enemy polygons are not needed.
	if (video2Index != MEMLIST_PART_NONE) 
		_segVideo2 = _memList[video2Index].bufPtr;
	

	currentPartId = partId;
	

	// _scriptCurPtr is changed in this->load();
	_scriptBakPtr = _scriptCurPtr;	
}
#define	MEM_BLOCK_SIZE (600 * 1024)   //600kb total memory consumed (not taking into account stack and static heap)

void allocMemBlock(uint8_t *mem) {
  _memPtrStart = mem;
  _scriptBakPtr = _scriptCurPtr = _memPtrStart;
  _vidBakPtr = _vidCurPtr = _memPtrStart + MEM_BLOCK_SIZE - 0x800 * 16; //0x800 = 2048, so we have 32KB free for vidBack and vidCur
}

