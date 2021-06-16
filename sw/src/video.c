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

#include "video.h"
#include "sys.h"

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

static inline uint16_t READ_BE_UINT16(const void *ptr) {
	const uint8_t *b = (const uint8_t *)ptr;
	return (b[0] << 8) | b[1];
}


static struct Polygon polygon;
static uint8_t *_dataBuf;
static uint8_t *_pDatapc;

//Precomputer division lookup table
static uint16_t _interpTable[0x400];

static uint8_t *_pages[4];

static uint8_t *_curPagePtr1, *_curPagePtr2, *_curPagePtr3;
static int16_t _hliney;
static const uint8_t _font[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00,
	0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x7E, 0x24, 0x24, 0x7E, 0x24, 0x00,
	0x08, 0x3E, 0x48, 0x3C, 0x12, 0x7C, 0x10, 0x00, 0x42, 0xA4, 0x48, 0x10, 0x24, 0x4A, 0x84, 0x00,
	0x60, 0x90, 0x90, 0x70, 0x8A, 0x84, 0x7A, 0x00, 0x08, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x06, 0x08, 0x10, 0x10, 0x10, 0x08, 0x06, 0x00, 0xC0, 0x20, 0x10, 0x10, 0x10, 0x20, 0xC0, 0x00,
	0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x10, 0x10, 0x7C, 0x10, 0x10, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x10, 0x28, 0x10, 0x00, 0x00, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00,
	0x78, 0x84, 0x8C, 0x94, 0xA4, 0xC4, 0x78, 0x00, 0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x7C, 0x00,
	0x78, 0x84, 0x04, 0x08, 0x30, 0x40, 0xFC, 0x00, 0x78, 0x84, 0x04, 0x38, 0x04, 0x84, 0x78, 0x00,
	0x08, 0x18, 0x28, 0x48, 0xFC, 0x08, 0x08, 0x00, 0xFC, 0x80, 0xF8, 0x04, 0x04, 0x84, 0x78, 0x00,
	0x38, 0x40, 0x80, 0xF8, 0x84, 0x84, 0x78, 0x00, 0xFC, 0x04, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00,
	0x78, 0x84, 0x84, 0x78, 0x84, 0x84, 0x78, 0x00, 0x78, 0x84, 0x84, 0x7C, 0x04, 0x08, 0x70, 0x00,
	0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x10, 0x10, 0x60,
	0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0xFE, 0x00, 0x00,
	0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20, 0x00, 0x7C, 0x82, 0x02, 0x0C, 0x10, 0x00, 0x10, 0x00,
	0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00, 0x78, 0x84, 0x84, 0xFC, 0x84, 0x84, 0x84, 0x00,
	0xF8, 0x84, 0x84, 0xF8, 0x84, 0x84, 0xF8, 0x00, 0x78, 0x84, 0x80, 0x80, 0x80, 0x84, 0x78, 0x00,
	0xF8, 0x84, 0x84, 0x84, 0x84, 0x84, 0xF8, 0x00, 0x7C, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7C, 0x00,
	0xFC, 0x80, 0x80, 0xF0, 0x80, 0x80, 0x80, 0x00, 0x7C, 0x80, 0x80, 0x8C, 0x84, 0x84, 0x7C, 0x00,
	0x84, 0x84, 0x84, 0xFC, 0x84, 0x84, 0x84, 0x00, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7C, 0x00,
	0x04, 0x04, 0x04, 0x04, 0x84, 0x84, 0x78, 0x00, 0x8C, 0x90, 0xA0, 0xE0, 0x90, 0x88, 0x84, 0x00,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFC, 0x00, 0x82, 0xC6, 0xAA, 0x92, 0x82, 0x82, 0x82, 0x00,
	0x84, 0xC4, 0xA4, 0x94, 0x8C, 0x84, 0x84, 0x00, 0x78, 0x84, 0x84, 0x84, 0x84, 0x84, 0x78, 0x00,
	0xF8, 0x84, 0x84, 0xF8, 0x80, 0x80, 0x80, 0x00, 0x78, 0x84, 0x84, 0x84, 0x84, 0x8C, 0x7C, 0x03,
	0xF8, 0x84, 0x84, 0xF8, 0x90, 0x88, 0x84, 0x00, 0x78, 0x84, 0x80, 0x78, 0x04, 0x84, 0x78, 0x00,
	0x7C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x84, 0x84, 0x84, 0x84, 0x84, 0x84, 0x78, 0x00,
	0x84, 0x84, 0x84, 0x84, 0x84, 0x48, 0x30, 0x00, 0x82, 0x82, 0x82, 0x82, 0x92, 0xAA, 0xC6, 0x00,
	0x82, 0x44, 0x28, 0x10, 0x28, 0x44, 0x82, 0x00, 0x82, 0x44, 0x28, 0x10, 0x10, 0x10, 0x10, 0x00,
	0xFC, 0x04, 0x08, 0x10, 0x20, 0x40, 0xFC, 0x00, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00,
	0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00,
	0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
	0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, 0x00, 0x00, 0x38, 0x04, 0x3C, 0x44, 0x3C, 0x00,
	0x40, 0x40, 0x78, 0x44, 0x44, 0x44, 0x78, 0x00, 0x00, 0x00, 0x3C, 0x40, 0x40, 0x40, 0x3C, 0x00,
	0x04, 0x04, 0x3C, 0x44, 0x44, 0x44, 0x3C, 0x00, 0x00, 0x00, 0x38, 0x44, 0x7C, 0x40, 0x3C, 0x00,
	0x38, 0x44, 0x40, 0x60, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x3C, 0x44, 0x44, 0x3C, 0x04, 0x78,
	0x40, 0x40, 0x58, 0x64, 0x44, 0x44, 0x44, 0x00, 0x10, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00,
	0x02, 0x00, 0x02, 0x02, 0x02, 0x02, 0x42, 0x3C, 0x40, 0x40, 0x46, 0x48, 0x70, 0x48, 0x46, 0x00,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0xEC, 0x92, 0x92, 0x92, 0x92, 0x00,
	0x00, 0x00, 0x78, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00,
	0x00, 0x00, 0x78, 0x44, 0x44, 0x78, 0x40, 0x40, 0x00, 0x00, 0x3C, 0x44, 0x44, 0x3C, 0x04, 0x04,
	0x00, 0x00, 0x4C, 0x70, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x3C, 0x40, 0x38, 0x04, 0x78, 0x00,
	0x10, 0x10, 0x3C, 0x10, 0x10, 0x10, 0x0C, 0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x78, 0x00,
	0x00, 0x00, 0x44, 0x44, 0x44, 0x28, 0x10, 0x00, 0x00, 0x00, 0x82, 0x82, 0x92, 0xAA, 0xC6, 0x00,
	0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x42, 0x22, 0x24, 0x18, 0x08, 0x30,
	0x00, 0x00, 0x7C, 0x08, 0x10, 0x20, 0x7C, 0x00, 0x60, 0x90, 0x20, 0x40, 0xF0, 0x00, 0x00, 0x00,
	0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x00, 0x38, 0x44, 0xBA, 0xA2, 0xBA, 0x44, 0x38, 0x00,
	0x38, 0x44, 0x82, 0x82, 0x44, 0x28, 0xEE, 0x00, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA
};

static const char *strst[] = {
	"P E A N U T  3000",
	"Copyright  } 1990 Peanut Computer, Inc.\nAll rights reserved.\n\nCDOS Version 5.01",
	"2",
	"3",
	".",
	"A",
	"@",
	"PEANUT 3000",
	"R",
	"U",
	"N",
	"P",
	"R",
	"O",
	"J",
	"E",
	"C",
	"T",
	"Shield 9A.5f Ok",
	"Flux % 5.0177 Ok",
	"CDI Vector ok",
	" %%%ddd ok",
	"Race-Track ok",
	"SYNCHROTRON",
	"E: 23%\ng: .005\n\nRK: 77.2L\n\nopt: g+\n\n Shield:\n1: OFF\n2: ON\n3: ON\n\nP~: 1\n",
	"ON",
	"-",
	"|",
	"--- Theoretical study ---",
	" THE EXPERIMENT WILL BEGIN IN    SECONDS",
	"  20",
	"  19",
	"  18",
	"  4",
	"  3",
	"  2",
	"  1",
	"  0",
	"L E T ' S   G O",
	"- Phase 0:\nINJECTION of particles\ninto synchrotron",
	"- Phase 1:\nParticle ACCELERATION.",
	"- Phase 2:\nEJECTION of particles\non the shield.",
	"A  N  A  L  Y  S  I  S",
	"- RESULT:\nProbability of creating:\n ANTIMATTER: 91.V %\n NEUTRINO 27:  0.04 %\n NEUTRINO 424: 18 %\n",
	"   Practical verification Y/N ?",
	"SURE ?",
	"MODIFICATION OF PARAMETERS\nRELATING TO PARTICLE\nACCELERATOR (SYNCHROTRON).",
	"       RUN EXPERIMENT ?",
	"t---t",
	"000 ~",
	".20x14dd",
	"gj5r5r",
	"tilgor 25%",
	"12% 33% checked",
	"D=4.2158005584",
	"d=10.00001",
	"+",
	"*",
	"% 304",
	"gurgle 21",
	"{{{{",
	"Delphine Software",
	"By Eric Chahi",
	"  5",
	"  17",	
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"        ACCESS CODE:",
	"PRESS BUTTON OR RETURN TO CONTINUE",
	"   ENTER ACCESS CODE",
	"   INVALID PASSWORD !",
	"ANNULER",
	"      INSERT DISK ?\n\n\n\n\n\n\n\n\nPRESS ANY KEY TO CONTINUE",
	" SELECT SYMBOLS CORRESPONDING TO\n THE POSITION\n ON THE CODE WHEEL",
	"    LOADING...",
	"              ERROR",
	"LDKD",
	"HTDC",
	"CLLD",
	"FXLC",
	"KRFK",
	"XDDJ",
	"LBKG",
	"KLFB",
	"TTCT",
	"DDRX",
	"TBHK",
	"BRTD",
	"CKJL",
	"LFCK",
	"BFLX",
	"XJRT",
	"HRTB",
	"HBHK",
	"JCGB",
	"HHFL",
	"TFBB",
	"TXHF",
	"JHJL",
	" BY",
	"ERIC CHAHI",
	"         MUSIC AND SOUND EFFECTS",
	" ",
	"JEAN-FRANCOIS FREITAS",
	"IBM PC VERSION",
	"      BY",
	" DANIEL MORAIS",
	"       THEN PRESS FIRE",
	" PUT THE PADDLE ON THE UPPER LEFT CORNER",
	"PUT THE PADDLE IN CENTRAL POSITION",
	"PUT THE PADDLE ON THE LOWER RIGHT CORNER",
	"      Designed by ..... Eric Chahi",
	"    Programmed by...... Eric Chahi",
	"      Artwork ......... Eric Chahi",
	"Music by ........ Jean-francois Freitas",
	"            Sound effects",
	"        Jean-Francois Freitas\n             Eric Chahi",
	"              Thanks To",
	"           Jesus Martinez\n\n          Daniel Morais\n\n        Frederic Savoir\n\n      Cecile Chahi\n\n    Philippe Delamarre\n\n  Philippe Ulrich\n\nSebastien Berthet\n\nPierre Gousseau",
	"Now Go Out Of This World",
	"Good evening professor.",
	"I see you have driven here in your\nFerrari.",
	"IDENTIFICATION",
	"Monsieur est en parfaite sante.",
	"Y\n",
	"AU BOULOT !!!\n",
	""
};
const uint16_t strid[] =
  {
	0x001,
	0x002,
	0x003,
	0x004,
	0x005,
	0x006,
	0x007,
	0x008,
	0x00A,
	0x00B,
	0x00C,
	0x00D,
	0x00E,
	0x00F,
	0x010,
	0x011,
	0x012,
	0x013,
	0x014,
	0x015,
	0x016,
	0x017,
	0x018,
	0x019,
	0x01A,
	0x01B,
	0x01C,
	0x021,
	0x022,
	0x023,
	0x024,
	0x025,
	0x026,
	0x027,
	0x028,
	0x029,
	0x02A,
	0x02B,
	0x02C,
	0x031,
	0x032,
	0x033,
	0x034,
	0x035,
	0x036,
	0x037,
	0x038,
	0x039,
	0x03C,
	0x03D,
	0x03E,
	0x03F,
	0x040,
	0x041,
	0x042,
	0x043,
	0x044,
	0x045,
	0x046,
	0x047,
	0x048,
	0x049,
	0x04A,
	0x04B,
	0x04C,
	0x12C,
	0x12D,
	0x12E,
	0x12F,
	0x130,
	0x131,
	0x132,
	0x133,
	0x134,
	0x135,
	0x136,
	0x137,
	0x138,
	0x139,
	0x13A,
	0x13B,
	0x13C,
	0x13D,
	0x13E,
	0x13F,
	0x140,
	0x141,
	0x142,
	0x143,
	0x144,
	0x15E,
	0x15F,
	0x160,
	0x161,
	0x162,
	0x163,
	0x164,
	0x165,
	0x166,
	0x167,
	0x168,
	0x169,
	0x16A,
	0x16B,
	0x16C,
	0x16D,
	0x16E,
	0x16F,
	0x170,
	0x171,
	0x172,
	0x173,
	0x174,
	0x181,
	0x182,
	0x183,
	0x184,
	0x185,
	0x186,
	0x187,
	0x188,
	0x18B,
	0x18C,
	0x18D,
	0x18E,
	0x258,
	0x259,
	0x25A,
	0x25B,
	0x25C,
	0x25D,
	0x263,
	0x264,
	0x265,
	0x190,
	0x191,
	0x192,
	0x193,
	0x194,
	0x193,
	END_OF_STRING_DICTIONARY,
  };

static void readVertices(const uint8_t *p, uint16_t zoom);
static void fillPolygon(uint16_t color, uint16_t zoom, int16_t x, int16_t y);
static void readAndDrawPolygonHierarchy(uint16_t zoom, int16_t x, int16_t y);
static int32_t calcStep(int16_t p1x, int16_t p1y, int16_t p2x, int16_t p2y, uint16_t *dy);
static void videochangePal(uint8_t pal);
static void drawChar(uint8_t c, uint16_t x, uint16_t y, uint8_t color, uint8_t *buf);
static void drawPoint(uint8_t color, int16_t x, int16_t y);
static void drawLineP(int16_t x1, int16_t x2, uint8_t color);
static void drawLineN(int16_t x1, int16_t x2, uint8_t color);
static void drawLineBlend(int16_t x1, int16_t x2, uint8_t color);

static uint16_t bbw, bbh;
static uint8_t numPoints;
static int16_t xpoints[MAX_POINTS];
static int16_t ypoints[MAX_POINTS];

void readVertices(const uint8_t *p, uint16_t zoom) {
	bbw = (*p++) * zoom / 64;
	bbh = (*p++) * zoom / 64;
	numPoints = *p++;

	//Read all points, directly from bytecode segment
	for (int i = 0; i < numPoints; ++i) {
	  /*		Point *pt = &points[i];
		pt->x = (*p++) * zoom / 64;
		pt->y = (*p++) * zoom / 64;*/
	  xpoints[i] = (*p++)*zoom/64;
	  ypoints[i] = (*p++)*zoom/64;
	}
}

extern uint8_t paletteIdRequested/*, currentPaletteId*/;

uint8_t videomem[4*VID_PAGE_SIZE];
void videoinit() {

	paletteIdRequested = NO_PALETTE_CHANGE_REQUESTED;

	//uint8_t* tmp = (uint8_t *)malloc(4 * VID_PAGE_SIZE);
	//__builtin_memset(tmp,0,4 * VID_PAGE_SIZE);
	
	for (int i = 0; i < 4; ++i) {
    _pages[i] = videomem + i * VID_PAGE_SIZE;
	}

	_curPagePtr3 = videogetPage(1);
	_curPagePtr2 = videogetPage(2);
	_curPagePtr1 = videogetPage(0xFE);

	_interpTable[0] = 0x4000;

	for (int i = 1; i < 0x400; ++i) {
		_interpTable[i] = 0x4000 / i;
	}
}

/*
	This
*/
void videosetDataBuffer(uint8_t *dataBuf, uint16_t offset) {

	_dataBuf = dataBuf;
	_pDatapc = dataBuf + offset;
}


/*  A shape can be given in two different ways:

	 - A list of screenspace vertices.
	 - A list of objectspace vertices, based on a delta from the first vertex.

	 This is a recursive function. */
//void videoreadAndDrawPolygon(uint8_t color, uint16_t zoom, const Point &pt) {
void videoreadAndDrawPolygon(uint8_t color, uint16_t zoom, const int16_t ptx, int16_t pty) {

	uint8_t i = *_pDatapc++;

	//This is 
	if (i >= 0xC0) {	// 0xc0 = 192

		// WTF ?
		if (color & 0x80) {   //0x80 = 128 (1000 0000)
			color = i & 0x3F; //0x3F =  63 (0011 1111)   
		}

		// pc is misleading here since we are not reading bytecode but only
		// vertices informations.
		readVertices(_pDatapc, zoom);

		fillPolygon(color, zoom, ptx ,pty);



	} else {
		i &= 0x3F;  //0x3F = 63
		if (i == 1) {
		} else if (i == 2) {
		  readAndDrawPolygonHierarchy(zoom, ptx, pty);

		} else {
		}
	}



}

void fillPolygon(uint16_t color, uint16_t zoom, int16_t ptx, int16_t pty) {

	if (bbw == 0 && bbh == 1 && numPoints == 4) {
		drawPoint(color, ptx, pty);

		return;
	}
	
	int16_t x1 = ptx - bbw / 2;
	int16_t x2 = ptx + bbw / 2;
	int16_t y1 = pty - bbh / 2;
	int16_t y2 = pty + bbh / 2;

	if (x1 > 319 || x2 < 0 || y1 > 199 || y2 < 0)
		return;

	_hliney = y1;
	
	uint16_t i, j;
	i = 0;
	j = numPoints - 1;
	
	x2 = xpoints[i] + x1;
	x1 = xpoints[j] + x1;

	++i;
	--j;

	uint32_t cpt1 = x1 << 16;
	uint32_t cpt2 = x2 << 16;

	while (1) {
		numPoints -= 2;
		if (numPoints == 0) {
			break;
		}
		uint16_t h;
		int32_t step1 = calcStep(xpoints[j + 1], ypoints[j + 1], xpoints[j], ypoints[j], &h);
		int32_t step2 = calcStep(xpoints[i - 1], ypoints[i - 1], xpoints[i], ypoints[i], &h);

		++i;
		--j;

		cpt1 = (cpt1 & 0xFFFF0000) | 0x7FFF;
		cpt2 = (cpt2 & 0xFFFF0000) | 0x8000;

		if (h == 0) {	
			cpt1 += step1;
			cpt2 += step2;
		} else {
			for (; h != 0; --h) {
				if (_hliney >= 0) {
					x1 = cpt1 >> 16;
					x2 = cpt2 >> 16;
					if (x1 <= 319 && x2 >= 0) {
						if (x1 < 0) x1 = 0;
						if (x2 > 319) x2 = 319;
						if (color < 0x10)
						  drawLineN(x1, x2, color);
						else if (color > 0x10)
						  drawLineP(x1, x2, color);
						else
						  drawLineBlend(x1, x2, color);
					}
				}
				cpt1 += step1;
				cpt2 += step2;
				++_hliney;					
				if (_hliney > 199) return;
			}
		}
	}





}

/*
    What is read from the bytecode is not a pure screnspace polygon but a polygonspace polygon.

*/
void readAndDrawPolygonHierarchy(uint16_t zoom, int16_t ptx, int16_t pty) {

	ptx -= *_pDatapc++ * zoom / 64;
	pty -= *_pDatapc++ * zoom / 64;

	int16_t childs = *_pDatapc++;

	for ( ; childs >= 0; --childs) {

		uint16_t off = READ_BE_UINT16(_pDatapc);
		_pDatapc += 2;

		int16_t pox = ptx;
		int16_t poy = pty;

		pox += *_pDatapc++ * zoom / 64;
		poy += *_pDatapc++ * zoom / 64;

		uint16_t color = 0xFF;
		uint16_t _bp = off;
		off &= 0x7FFF;

		if (_bp & 0x8000) {
			color = *_pDatapc & 0x7F;
			_pDatapc += 2;
		}

		uint8_t *bak = _pDatapc;
		_pDatapc = _dataBuf + off * 2;


		videoreadAndDrawPolygon(color, zoom, pox, poy);


		_pDatapc = bak;
	}

	
}

int32_t calcStep(int16_t p1x, int16_t p1y, int16_t p2x, int16_t p2y, uint16_t *dy) {
	*dy = p2y - p1y;
	return (p2x - p1x) * _interpTable[*dy] * 4;
}

void videodrawString(uint8_t color, uint16_t x, uint16_t y, uint16_t stringId) {

	uint16_t se;
	//Search for the location where the string is located.
	while (strid[se] != END_OF_STRING_DICTIONARY && strid[se] != stringId) 
		++se;
	

	//Not found
	if (strid[se] == END_OF_STRING_DICTIONARY)
		return;
	

    //Used if the string contains a return carriage.
	uint16_t xOrigin = x;
	int len = __builtin_strlen(strst[se]);
	for (int i = 0; i < len; ++i) {

		if (strst[se][i] == '\n') {
			y += 8;
			x = xOrigin;
			continue;
		} 
		
		drawChar(strst[se][i], x, y, color, _curPagePtr1);
		x++;
		
	}
}

void drawChar(uint8_t character, uint16_t x, uint16_t y, uint8_t color, uint8_t *buf) {
	if (x <= 39 && y <= 192) {
		
		const uint8_t *ft = _font + (character - ' ') * 8;

		uint8_t *p = buf + x * 4 + y * 160;

		for (int j = 0; j < 8; ++j) {
			uint8_t ch = *(ft + j);
			for (int i = 0; i < 4; ++i) {
				uint8_t b = *(p + i);
				uint8_t cmask = 0xFF;
				uint8_t colb = 0;
				if (ch & 0x80) {
					colb |= color << 4;
					cmask &= 0x0F;
				}
				ch <<= 1;
				if (ch & 0x80) {
					colb |= color;
					cmask &= 0xF0;
				}
				ch <<= 1;
				*(p + i) = (b & cmask) | colb;
			}
			p += 160;
		}
	}
}

void drawPoint(uint8_t color, int16_t x, int16_t y) {
	if (x >= 0 && x <= 319 && y >= 0 && y <= 199) {
		uint16_t off = y * 160 + x / 2;
	
		uint8_t cmasko, cmaskn;
		if (x & 1) {
			cmaskn = 0x0F;
			cmasko = 0xF0;
		} else {
			cmaskn = 0xF0;
			cmasko = 0x0F;
		}

		uint8_t colb = (color << 4) | color;
		if (color == 0x10) {
			cmaskn &= 0x88;
			cmasko = ~cmaskn;
			colb = 0x88;		
		} else if (color == 0x11) {
			colb = *(_pages[0] + off);
		}
		uint8_t b = *(_curPagePtr1 + off);
		*(_curPagePtr1 + off) = (b & cmasko) | (colb & cmaskn);
	}
}

/* Blend a line in the current framebuffer (_curPagePtr1)
*/
void drawLineBlend(int16_t x1, int16_t x2, uint8_t color) {
	int16_t xmax = MAX(x1, x2);
	int16_t xmin = MIN(x1, x2);
	uint8_t *p = _curPagePtr1 + _hliney * 160 + xmin / 2;

	uint16_t w = xmax / 2 - xmin / 2 + 1;
	uint8_t cmaske = 0;
	uint8_t cmasks = 0;	
	if (xmin & 1) {
		--w;
		cmasks = 0xF7;
	}
	if (!(xmax & 1)) {
		--w;
		cmaske = 0x7F;
	}

	if (cmasks != 0) {
		*p = (*p & cmasks) | 0x08;
		++p;
	}
	while (w--) {
		*p = (*p & 0x77) | 0x88;
		++p;
	}
	if (cmaske != 0) {
		*p = (*p & cmaske) | 0x80;
		++p;
	}


}

void drawLineN(int16_t x1, int16_t x2, uint8_t color) {
	int16_t xmax = MAX(x1, x2);
	int16_t xmin = MIN(x1, x2);
	uint8_t *p = _curPagePtr1 + _hliney * 160 + xmin / 2;

	uint16_t w = xmax / 2 - xmin / 2 + 1;
	uint8_t cmaske = 0;
	uint8_t cmasks = 0;	
	if (xmin & 1) {
		--w;
		cmasks = 0xF0;
	}
	if (!(xmax & 1)) {
		--w;
		cmaske = 0x0F;
	}

	uint8_t colb = ((color & 0xF) << 4) | (color & 0xF);	
	if (cmasks != 0) {
		*p = (*p & cmasks) | (colb & 0x0F);
		++p;
	}
	while (w--) {
		*p++ = colb;
	}
	if (cmaske != 0) {
		*p = (*p & cmaske) | (colb & 0xF0);
		++p;		
	}

	
}

void drawLineP(int16_t x1, int16_t x2, uint8_t color) {
	int16_t xmax = MAX(x1, x2);
	int16_t xmin = MIN(x1, x2);
	uint16_t off = _hliney * 160 + xmin / 2;
	uint8_t *p = _curPagePtr1 + off;
	uint8_t *q = _pages[0] + off;

	uint8_t w = xmax / 2 - xmin / 2 + 1;
	uint8_t cmaske = 0;
	uint8_t cmasks = 0;	
	if (xmin & 1) {
		--w;
		cmasks = 0xF0;
	}
	if (!(xmax & 1)) {
		--w;
		cmaske = 0x0F;
	}

	if (cmasks != 0) {
		*p = (*p & cmasks) | (*q & 0x0F);
		++p;
		++q;
	}
	while (w--) {
		*p++ = *q++;			
	}
	if (cmaske != 0) {
		*p = (*p & cmaske) | (*q & 0xF0);
		++p;
		++q;
	}

}

uint8_t *videogetPage(uint8_t page) {
	uint8_t *p;
	if (page <= 3) {
		p = _pages[page];
	} else {
		switch (page) {
		case 0xFF:
			p = _curPagePtr3;
			break;
		case 0xFE:
			p = _curPagePtr2;
			break;
		default:
			p = _pages[0]; // XXX check
			break;
		}
	}
	return p;
}



void videochangePagePtr1(uint8_t pageID) {
	_curPagePtr1 = videogetPage(pageID);
}


/*  This opcode is used once the background of a scene has been drawn in one of the framebuffer:
	   it is copied in the current framebuffer at the start of a new frame in order to improve performances. */
void videocopyPage(uint8_t srcPageId, uint8_t dstPageId, int16_t vscroll) {


	if (srcPageId == dstPageId)
		return;

	uint8_t *p;
	uint8_t *q;

	if (srcPageId >= 0xFE || !((srcPageId &= 0xBF) & 0x80)) {
		p = videogetPage(srcPageId);
		q = videogetPage(dstPageId);
		__builtin_memcpy(q, p, VID_PAGE_SIZE);
			
	} else {
		p = videogetPage(srcPageId & 3);
		q = videogetPage(dstPageId);
		if (vscroll >= -199 && vscroll <= 199) {
			uint16_t h = 200;
			if (vscroll < 0) {
				h += vscroll;
				p += -vscroll * 160;
			} else {
				h -= vscroll;
				q += vscroll * 160;
			}
			__builtin_memcpy(q, p, h * 160);
		}
	}
}

void videosetBitmap(const uint8_t *src) {
	uint8_t *dst = _pages[0];
	int h = 200;
	while (h--) {
		int w = 40;
		while (w--) {
			uint8_t p[] = {
				*(src + 8000 * 3),
				*(src + 8000 * 2),
				*(src + 8000 * 1),
				*(src + 8000 * 0)
			};
			for(int j = 0; j < 4; ++j) {
				uint8_t acc = 0;
				for (int i = 0; i < 8; ++i) {
					acc <<= 1;
					acc |= (p[i & 3] & 0x80) ? 1 : 0;
					p[i & 3] <<= 1;
				}
				*dst++ = acc;
			}
			++src;
		}
	}


}

extern uint8_t *segPalettes;

/*
Note: The palettes set used to be allocated on the stack but I moved it to
      the heap so I could dump the four framebuffer and follow how
	  frames are generated.
*/
static void videochangePal(uint8_t palNum) {

	if (palNum >= 32)
		return;
	
	uint8_t *p = segPalettes + palNum * 32; //colors are coded on 2bytes (565) for 16 colors = 32
	syssetPalette(p);
	//currentPaletteId = palNum;
}

void videoupdateDisplay(uint8_t pageId) {


	if (pageId != 0xFE) {
		if (pageId == 0xFF) {
		        	uint8_t *tmp = _curPagePtr2;
				_curPagePtr2 = _curPagePtr3;
				_curPagePtr3 = tmp;
		} else {
			_curPagePtr2 = videogetPage(pageId);
		}
	}

	//Check if we need to change the palette
	if (paletteIdRequested != NO_PALETTE_CHANGE_REQUESTED) {
		videochangePal(paletteIdRequested);
		paletteIdRequested = NO_PALETTE_CHANGE_REQUESTED;
	}

	//Q: Why 160 ?
	//A: Because one byte gives two palette indices so
	//   we only need to move 320/2 per line.
  sysupdateDisplay(_curPagePtr2);
}

