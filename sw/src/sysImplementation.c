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

#include <SDL.h>
#include "sys.h"

#define NUM_COLORS 16
#define nullptr NULL

static SDL_Renderer * _renderer = nullptr;
static SDL_Surface *_screen = nullptr;
static SDL_Window * _window = nullptr;
static SDL_Color palette[NUM_COLORS];

#define DEFAULT_SCALE 3
#define SCREEN_W 320
#define SCREEN_H 200
static uint8_t _scale = DEFAULT_SCALE;

void sysinit() {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

  SDL_ShowCursor(SDL_DISABLE);

  SDL_ShowCursor( SDL_ENABLE );
  SDL_CaptureMouse(SDL_TRUE);

  _scale = DEFAULT_SCALE;
  int w = SCREEN_W;
  int h = SCREEN_H;

  _window = SDL_CreateWindow("Another World", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w * _scale, h * _scale, SDL_WINDOW_SHOWN);
  _renderer = SDL_CreateRenderer(_window, -1, 0);
  _screen = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
  SDL_SetPaletteColors(_screen->format->palette, palette, 0, NUM_COLORS);
}

void sysdestroy() {
	SDL_Quit();
}

void syssetPalette(const uint8_t *p) {
  // The incoming palette is in 565 format.
  for (int i = 0; i < NUM_COLORS; ++i)
  {
    uint8_t c1 = *(p + 0);
    uint8_t c2 = *(p + 1);
    palette[i].r = (((c1 & 0x0F) << 2) | ((c1 & 0x0F) >> 2)) << 2; // r
    palette[i].g = (((c2 & 0xF0) >> 2) | ((c2 & 0xF0) >> 6)) << 2; // g
    palette[i].b = (((c2 & 0x0F) >> 2) | ((c2 & 0x0F) << 2)) << 2; // b
    palette[i].a = 0xFF;
    p += 2;
  }
  SDL_SetPaletteColors(_screen->format->palette, palette, 0, NUM_COLORS);
}

void sysupdateDisplay(const uint8_t *src) {
  uint16_t height = SCREEN_H;
  uint8_t* p = (uint8_t*)_screen->pixels;

  //For each line
  while (height--) {
    //One byte gives us two pixels, we only need to iterate w/2 times.
    for (int i = 0; i < SCREEN_W / 2; ++i) {
      //Extract two palette indices from upper byte and lower byte.
      p[i * 2 + 0] = *(src + i) >> 4;
      p[i * 2 + 1] = *(src + i) & 0xF;
    }
    p += _screen->pitch;
    src += SCREEN_W/2;
  }

  SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, _screen);
  SDL_RenderCopy(_renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(_renderer);
  SDL_DestroyTexture(texture);

}

#define DIR_LEFT  (1 << 0)
#define DIR_RIGHT (1 << 1)
#define DIR_UP    (1 << 2)
#define DIR_DOWN  (1 << 3)
void sysprocessEvents(struct PlayerInput *input) {
	SDL_Event ev;
	while(SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			input->quit = true;
			break;
		case SDL_KEYUP:
			switch(ev.key.keysym.sym) {
			case SDLK_LEFT:
				input->dirMask &= ~DIR_LEFT;
				break;
			case SDLK_RIGHT:
				input->dirMask &= ~DIR_RIGHT;
				break;
			case SDLK_UP:
				input->dirMask &= ~DIR_UP;
				break;
			case SDLK_DOWN:
				input->dirMask &= ~DIR_DOWN;
				break;
			case SDLK_SPACE:
			case SDLK_RETURN:
				input->button = false;
				break;
			case SDLK_ESCAPE:
			  input->quit = true;
				break;
			}
			break;
		case SDL_KEYDOWN:
			input->lastChar = ev.key.keysym.sym;
			switch(ev.key.keysym.sym) {
			case SDLK_LEFT:
				input->dirMask |= DIR_LEFT;
				break;
			case SDLK_RIGHT:
				input->dirMask |= DIR_RIGHT;
				break;
			case SDLK_UP:
				input->dirMask |= DIR_UP;
				break;
			case SDLK_DOWN:
				input->dirMask |= DIR_DOWN;
				break;
			case SDLK_SPACE:
			case SDLK_RETURN:
				input->button = true;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
}

void sysSleep(uint32_t duration) {
	SDL_Delay(duration);
}

uint32_t sysGetTimeStamp() {
	return SDL_GetTicks();	
}
