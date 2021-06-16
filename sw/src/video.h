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

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct StrEntry {
	uint16_t id;
	const char *str;
};

#define MAX_POINTS 50

struct Polygon {


};

// This is used to detect the end of  _stringsTableEng and _stringsTableDemo
#define END_OF_STRING_DICTIONARY 0xFFFF 

// Special value when no palette change is necessary
#define NO_PALETTE_CHANGE_REQUESTED 0xFF 

#define	VID_PAGE_SIZE  320 * 200 / 2


void videoinit();
void videosetDataBuffer(uint8_t *dataBuf, uint16_t offset);
void videoreadAndDrawPolygon(uint8_t color, uint16_t zoom, int16_t x, int16_t y);
void videodrawString(uint8_t color, uint16_t x, uint16_t y, uint16_t strId);
void videochangePagePtr1(uint8_t page);
uint8_t *videogetPage(uint8_t page);
void videocopyPage(uint8_t src, uint8_t dst, int16_t vscroll);
void videosetBitmap(const uint8_t *src);
void videoupdateDisplay(uint8_t page);

#ifdef __cplusplus
}
#endif
#endif
