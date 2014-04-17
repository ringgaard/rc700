/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * SDL-based RC700 terminal
 */

#include <stdio.h>
#include <string.h>
#include <locale.h>
#ifdef WIN32
#include <SDL.h>
#else
#include <stdint.h>
#include <SDL/SDL.h>
#endif
#include "sim.h"
#include "simglb.h"

#define L(x)

#define XSCALE 3/2
#define YSCALE 2

#define SCREEN_WIDTH  (80 * 7 * XSCALE)
#define SCREEN_HEIGHT (25 * 11 * YSCALE)
#define SCREEN_BPP    32

/*
 * Character attributes:
 *   0x80 Change character mode
 *   0x01 Highlight (not used in RC702)
 *   0x02 Blink
 *   0x04 Semigraphic character set
 *   0x10 Reverse
 *   0x20 Underline
 *
 * Special codes:
 *   0xF0 End of row
 *   0xF1 End of row, stop DMA
 *   0xF2 End of screen
 *   0xF3 End of screen, stop DMA
 *
 *   Character matrix 7x11 bits
 */

#define ATTR_MODE      0x80
#define ATTR_HIGHLIGHT 0x01
#define ATTR_BLINK     0x02
#define ATTR_SEMI      0x04
#define ATTR_REVERSE   0x10
#define ATTR_UNDERLINE 0x20

#define ATTR_SPECIAL   0xF0
#define ATTR_CLREOL    0xF0
#define ATTR_CLREOS    0xF2

#define HI_COLOR 0xFFCC66
#define FG_COLOR 0xCC9933
#define BG_COLOR 0x552200
#define MI_COLOR 0x996611

#ifdef WIN32
typedef unsigned __int32 pixel_t;
#else
typedef uint32_t pixel_t;
#endif

static SDL_Surface *term = NULL;
extern unsigned char charrom[];
extern unsigned char charram[];

static int cursor_type = 0;
static int under_line = 9;
static int cur_x = -1;
static int cur_y = -1;

unsigned char blank[] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

pixel_t palette[16] = {
  BG_COLOR, BG_COLOR, BG_COLOR, 0,
  FG_COLOR, MI_COLOR, BG_COLOR, 0,
  BG_COLOR, MI_COLOR, FG_COLOR, 0,
  FG_COLOR, FG_COLOR, FG_COLOR, 0,
};

void draw_screen(pixel_t *bitmap, unsigned char *text) {
  int row, col, line, blk, grp;
  int clreos, clreol, uline;
  unsigned char **c;
  unsigned char *a;
  unsigned char mode;
  unsigned char ch;
  unsigned char m;
  unsigned char pixels;
  pixel_t mask;
  pixel_t *begin;
  unsigned char *charmem;
  unsigned char *cell[80];
  unsigned char attr[80];

  // Generate 25 lines of text.
  mode = 0;
  clreos = 0;
  uline = 10 - under_line;
  for (col = 0; col < 25; ++col) {
    // Set up pointers into the character cell ROM.
    c = cell;
    a = attr;
    clreol = clreos;
    charmem = mode & ATTR_SEMI ? charram : charrom;
    for (row = 0; row < 80; ++row) {
      ch = *text++;
      if (ch & ATTR_MODE) {
        if ((ch & ATTR_SPECIAL) == ATTR_SPECIAL) {
          switch (ch) {
            case  ATTR_CLREOL: clreol = 1; break;
            case ATTR_CLREOS: clreos = 1; break;
          }
        } else {
          mode = ch & (ATTR_HIGHLIGHT | ATTR_BLINK | ATTR_SEMI | ATTR_REVERSE | ATTR_UNDERLINE);
          charmem = mode & ATTR_SEMI ? charram : charrom;
        }
        *a++ = 0;
        *c++ = blank;
      } else if (clreol) {
        *a++ = 0;
        *c++ = blank;
      } else {
        m = mode;
        if (row == cur_x && col == cur_y) {
          m ^= (cursor_type & 0x01) ? ATTR_UNDERLINE : ATTR_REVERSE;
        }
        *a++ = m;
        *c++ = charmem + (ch << 4);
      }
    }
    
    // Each character has 11 lines.
    for (line = 11; line > 0; --line) {
      begin = bitmap;
      c = cell;
      
      // Generate 20 * 4 = 80 characters per line.
      a = attr;
      for (blk = 20; blk > 0; --blk) {
        // Fetch next 4 characters, each 7 pixels wide.
        mask = 0;
        for (grp = 4; grp > 0; --grp) {
          pixels = *(*c++)++;
          mode = *a++;
          if (mode) {
            if (mode & ATTR_UNDERLINE && line == uline) pixels = 0x7F;
            if (mode & ATTR_REVERSE) pixels ^= 0x7F;
          }
          mask = (mask >> 7) | (pixels << 21);
        }

        // Generate two pixels at a time.
        for (grp = 14; grp > 0; --grp) {
          // Generate three screen pixels per character pixel pair.
          int color = mask & 0x03;
          pixel_t *p = palette + (color << 2);
          *bitmap++ = *p++;
          *bitmap++ = *p++;
          *bitmap++ = *p++;
          mask >>= 2;
        }
      }
      
      // Duplicate previous pixel line.
      memcpy(bitmap, begin, 40 * 3 * 7 * 4);
      bitmap += 40 * 3 * 7;
    }
  }
}

void rcterm_init(void) {
  L(printf("rcterm: init\n"));
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
  term = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE | SDL_DOUBLEBUF);
  SDL_WM_SetCaption("RC700", NULL);
  SDL_EnableUNICODE(SDL_ENABLE);
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

void rcterm_exit(void) {
  L(printf("rcterm: exit\n"));
  SDL_FreeSurface(term);
  L(printf("rcterm: quit\n"));
  SDL_Quit();
}

void rcterm_clear_screen(int cols, int rows) {
  L(printf("rcterm: clear screen\n"));
  SDL_FillRect(term, NULL, BG_COLOR);
  SDL_Flip(term);
}

void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows) {
  L(printf("rcterm: screen at %04x cols=%d, rows=%d\n", screen - ram, cols, rows));
  SDL_LockSurface(term);
  draw_screen(term->pixels, screen);
  SDL_UnlockSurface(term);
  SDL_Flip(term);
}

void rcterm_set_cursor(int type, int underline) {
  L(printf("rcterm: set cursor type %d underline %d\n", type, underline));
  cursor_type = type;
  under_line = underline;
}

int rcterm_gotoxy(int col, int row) {
  L(printf("rcterm: gotoxy col=%d, row=%d\n", col, row));
  if (col == cur_x && row == cur_y) return 0;
  cur_x = col;
  cur_y = row;
  return 1;
}

int rcterm_keypressed(void) {
  SDL_Event event;
  int sym;
  int code;

  if (!SDL_PollEvent(&event)) return -1;
  switch (event.type) {
    case SDL_KEYDOWN:
      sym = event.key.keysym.sym;
      switch (sym) {
        case SDLK_UP: return 0x1A;
        case SDLK_DOWN: return 0x0A;
        case SDLK_LEFT: return 0x08;
        case SDLK_RIGHT: return 0x18;
        case SDLK_BACKSPACE: return 0x08;
        case SDLK_ESCAPE: return 0x1B;
        case SDLK_TAB: return '\t';
        case SDLK_RETURN: return '\r';
        default:
          code = event.key.keysym.unicode;
          switch (code) {
            case 0xC6: return 0x5B; // AE
            case 0xD8: return 0x5C; // OE
            case 0xC5: return 0x5D; // AA
            case 0xE6: return 0x7B; // ae
            case 0xF8: return 0x7C; // oe
            case 0xE5: return 0x7D; // aa
            default: if (code > 0 && code <= 0x7F) return code;
          }
      }
      break;
    
    case SDL_QUIT:
      cpu_error = USERINT;
      cpu_state = STOPPED;
      return -1;
  }
  return -1;
}

