#include <stdint.h>
#include <signal.h>

#include <SDL.h>

#include "verilated_vcd_c.h"
#include "Vanother_serv__Syms.h"

using namespace std;

static bool done;

vluint64_t main_time = 0;       // Current simulation time
// This is a 64-bit integer to reduce wrap over issues and
// allow modulus.  You can also use a double, if you wish.

double sc_time_stamp () {       // Called by $time in Verilog
  return main_time;           // converts to double, to match
  // what SystemC does
}

void INThandler(int signal)
{
	printf("\nCaught ctrl-c\n");
	done = true;
}

typedef struct {
  bool last_value;
} gpio_context_t;

typedef struct {
  uint8_t state;
  char ch;
  uint32_t baud_t;
  vluint64_t last_update;
} uart_context_t;

void uart_init(uart_context_t *context, uint32_t baud_rate) {
  context->baud_t = 1000*1000*1000/baud_rate;
  context->state = 0;
}

void do_uart(uart_context_t *context, bool rx) {
  if (context->state == 0) {
    if (rx)
      context->state++;
  }
  else if (context->state == 1) {
    if (!rx) {
      context->last_update = main_time + context->baud_t/2;
      context->state++;
    }
  }
  else if(context->state == 2) {
    if (main_time > context->last_update) {
      context->last_update += context->baud_t;
      context->ch = 0;
      context->state++;
    }
  }
  else if (context->state < 11) {
    if (main_time > context->last_update) {
      context->last_update += context->baud_t;
      context->ch |= rx << (context->state-3);
      context->state++;
    }
  }
  else {
    if (main_time > context->last_update) {
      context->last_update += context->baud_t;
      putchar(context->ch);
      context->state=1;
    }
  }
}

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

void setpalcol(int idx, uint16_t col) {
  uint8_t c1 = col & 0xff;
  uint8_t c2 = (col >> 8) & 0xff;
  palette[idx].r = (((c1 & 0x0F) << 2) | ((c1 & 0x0F) >> 2)) << 2; // r
  palette[idx].g = (((c2 & 0xF0) >> 2) | ((c2 & 0xF0) >> 6)) << 2; // g
  palette[idx].b = (((c2 & 0x0F) >> 2) | ((c2 & 0x0F) << 2)) << 2; // b
  palette[idx].a = 0xff;
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

int main(int argc, char **argv, char **env)
{
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

  uint8_t vbuf[320*200/2];
  vluint64_t sample_time = 0;
	uint32_t insn = 0;
	uint32_t ex_pc = 0;
	int baud_rate = 0;

	gpio_context_t gpio_context;
	uart_context_t uart_context;
	Verilated::commandArgs(argc, argv);

	Vanother_serv* top = new Vanother_serv;

	const char *arg = Verilated::commandArgsPlusMatch("uart_baudrate=");
	if (arg[0]) {
	  baud_rate = atoi(arg+15);
	  if (baud_rate) {
	    uart_init(&uart_context, baud_rate);
	  }
	}

	VerilatedVcdC * tfp = 0;
	const char *vcd = Verilated::commandArgsPlusMatch("vcd=");
	if (vcd[0]) {
	  Verilated::traceEverOn(true);
	  tfp = new VerilatedVcdC;
	  top->trace (tfp, 99);
	  tfp->open ("trace.vcd");
	}

	signal(SIGINT, INThandler);

	vluint64_t timeout = 0;
	const char *arg_timeout = Verilated::commandArgsPlusMatch("timeout=");
	if (arg_timeout[0])
	  timeout = atoi(arg_timeout+9);

	vluint64_t vcd_start = 0;
	const char *arg_vcd_start = Verilated::commandArgsPlusMatch("vcd_start=");
	if (arg_vcd_start[0])
	  vcd_start = atoi(arg_vcd_start+11);

	bool dump = false;
	top->wb_clk = 1;
	bool q = top->q;
	
	while (!(done || Verilated::gotFinish())) {
	  if (tfp && !dump && (main_time > vcd_start)) {
	    dump = true;
	  }
	  top->wb_rst = main_time < 100;
	  top->eval();
	  if (dump)
	    tfp->dump(main_time);
	  if (baud_rate)
	    do_uart(&uart_context, top->q);

	  if (top->pal_en) {
	    setpalcol(top->pal_idx, top->pal_dat);
	  }
	  if (top->vid_en) {
	    for (int i=0;i<320*200/8;i++) {
	      vbuf[i*4+0] = (top->another_serv->ram->mem[(top->dma_adr>>2)+i] >>  0) & 0xff;
	      vbuf[i*4+1] = (top->another_serv->ram->mem[(top->dma_adr>>2)+i] >>  8) & 0xff;
	      vbuf[i*4+2] = (top->another_serv->ram->mem[(top->dma_adr>>2)+i] >> 16) & 0xff;
	      vbuf[i*4+3] = (top->another_serv->ram->mem[(top->dma_adr>>2)+i] >> 24) & 0xff;
	    }
	    sysupdateDisplay(vbuf);
	  }

	  if (timeout && (main_time >= timeout)) {
	    printf("Timeout: Exiting at time %lu\n", main_time);
	    done = true;
	  }

	  top->wb_clk = !top->wb_clk;
	  main_time+=31.25;

	  top->eval();
	  if (dump)
	    tfp->dump(main_time);
	  top->wb_clk = !top->wb_clk;
	  main_time+=31.25;

	}
	if (tfp)
	  tfp->close();
	exit(0);
}
