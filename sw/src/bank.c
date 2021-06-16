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

#include <stdbool.h>
#include "bank.h"
static uint8_t *_iBuf, *_oBuf;

struct UnpackContext {
	uint16_t size;
	uint32_t crc;
	uint32_t chk;
	int32_t datasize;
};

static struct UnpackContext _unpCtx;

static void decUnk1(uint8_t numChunks, uint8_t addCount);
static void decUnk2(uint8_t numChunks);
static bool unpack(uint8_t *_startBuf);
static uint16_t getCode(uint8_t numChunks);
static bool nextChunk();
static bool rcr(bool CF);

static inline uint32_t READ_BE_UINT32(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

bool bankread(uint16_t packedSize, uint8_t *buf) {

  bool ret = false;
  _iBuf = buf + packedSize - 4;
  ret = unpack(buf);
	
  return ret;
}

void decUnk1(uint8_t numChunks, uint8_t addCount) {
	uint16_t count = getCode(numChunks) + addCount + 1;
	_unpCtx.datasize -= count;
	while (count--) {
		*_oBuf = (uint8_t)getCode(8);
		--_oBuf;
	}
}

/*
   Note from fab: This look like run-length encoding.
*/
void decUnk2(uint8_t numChunks) {
	uint16_t i = getCode(numChunks);
	uint16_t count = _unpCtx.size + 1;
	_unpCtx.datasize -= count;
	while (count--) {
		*_oBuf = *(_oBuf + i);
		--_oBuf;
	}
}

/*
	Most resource in the banks are compacted.
*/
bool unpack(uint8_t *_startBuf) {
	_unpCtx.size = 0;
	_unpCtx.datasize = READ_BE_UINT32(_iBuf); _iBuf -= 4;
	_oBuf = _startBuf + _unpCtx.datasize - 1;
	_unpCtx.crc = READ_BE_UINT32(_iBuf); _iBuf -= 4;
	_unpCtx.chk = READ_BE_UINT32(_iBuf); _iBuf -= 4;
	_unpCtx.crc ^= _unpCtx.chk;
	do {
		if (!nextChunk()) {
			_unpCtx.size = 1;
			if (!nextChunk()) {
				decUnk1(3, 0);
			} else {
				decUnk2(8);
			}
		} else {
			uint16_t c = getCode(2);
			if (c == 3) {
				decUnk1(8, 8);
			} else {
				if (c < 2) {
					_unpCtx.size = c + 2;
					decUnk2(c + 9);
				} else {
					_unpCtx.size = getCode(8);
					decUnk2(12);
				}
			}
		}
	} while (_unpCtx.datasize > 0);
	return (_unpCtx.crc == 0);
}

uint16_t getCode(uint8_t numChunks) {
	uint16_t c = 0;
	while (numChunks--) {
		c <<= 1;
		if (nextChunk()) {
			c |= 1;
		}			
	}
	return c;
}

bool nextChunk() {
	bool CF = rcr(false);
	if (_unpCtx.chk == 0) {
		_unpCtx.chk = READ_BE_UINT32(_iBuf); _iBuf -= 4;
		_unpCtx.crc ^= _unpCtx.chk;
		CF = rcr(true);
	}
	return CF;
}

bool rcr(bool CF) {
	bool rCF = (_unpCtx.chk & 1);
	_unpCtx.chk >>= 1;
	if (CF) _unpCtx.chk |= 0x80000000;
	return rCF;
}
