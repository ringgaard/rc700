//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// SDL-based RC700 terminal for SDL version 2.0
//

#include <stdio.h>
#include <string.h>
#include <locale.h>
#ifdef WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "rc700.h"

#define L(x)

#define NUL_FILLERS   5

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static SDL_Surface *term = NULL;

static struct fifo kbdbuf;
static BYTE *clipboard = NULL;
static BYTE *clipboard_current = NULL;
static BYTE *clipboard_end = NULL;

// RC752 amber colors.
#define HI_COLOR 0xFFCC66
#define FG_COLOR 0xCC9933
#define BG_COLOR 0x552200
#define MI_COLOR 0x996611

pixel32_t palette[16] = {
  BG_COLOR, BG_COLOR, BG_COLOR, 0,
  FG_COLOR, MI_COLOR, BG_COLOR, 0,
  BG_COLOR, MI_COLOR, FG_COLOR, 0,
  FG_COLOR, FG_COLOR, FG_COLOR, 0,
};

pixel32_t screen_color(pixel32_t rgb) {
  BYTE r = (rgb >> 16) & 0xFF;
  BYTE g = (rgb >> 8) & 0xFF;
  BYTE b = rgb & 0xFF;
  return SDL_MapRGB(term->format, r, g, b);
}

void rcterm_init() {
  int i;

  L(printf("rcterm: init\n"));
  memset(&kbdbuf, 0, sizeof(kbdbuf));

  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("RC700",
                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  renderer =  SDL_CreateRenderer(window, -1, 0);
  texture = SDL_CreateTexture(renderer,
                              SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING,
                              SCREEN_WIDTH, SCREEN_HEIGHT);
  term = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP,
                              0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

  for (i = 0; i < 16; ++i) palette[i] = screen_color(palette[i]);
}

void rcterm_exit() {
  L(printf("rcterm: exit\n"));
  SDL_FreeSurface(term);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  L(printf("rcterm: quit\n"));
  SDL_Quit();
}

void rcterm_clear_screen(int cols, int rows) {
  L(printf("rcterm: clear screen\n"));
  SDL_FillRect(term, NULL, screen_color(BG_COLOR));
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows) {
  L(printf("rcterm: screen at %04x cols=%d, rows=%d\n", screen - ram, cols, rows));
#if 0
  pixel32_t *pixels;
  int pitch;

  SDL_LockTexture(texture, NULL, (void *) &pixels, &pitch);
  draw_screen32(pixels, palette, screen);
  SDL_UnlockTexture(texture);

  //SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);

#else
  draw_screen32(term->pixels, palette, screen);
  SDL_UpdateTexture(texture, NULL, term->pixels, term->pitch);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
#endif
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

int unicode_to_key(int unicode) {
  switch (unicode) {
    case 0xC6: return 0x5B; // AE
    case 0xD8: return 0x5C; // OE
    case 0xC5: return 0x5D; // AA
    case 0xE6: return 0x7B; // ae
    case 0xF8: return 0x7C; // oe
    case 0xE5: return 0x7D; // aa
    case 0xE4: return 0x60; // a umlaut
    case 0xF6: return 0x73; // o umlaut
    case 0xFC: return 0x40; // u umlaut
    default: if (unicode > 0 && unicode <= 0x7F) return unicode;
  }
  return -1;
}

void paste_clipboard() {
  char *text;
  BYTE *p;
  BYTE *q;
  //int newlines;
  int i;

  if (clipboard) return;

  text  = SDL_GetClipboardText();
  if (!text) return;
  //printf("sdl: paste clipboard [%s]\n", text);

  //newlines = 0;
  //for (p = text; *p; p++) if (*p == '\n') newlines++;

  //clipboard = malloc(strlen(text) + newlines * NUL_FILLERS);
  clipboard = (char *) malloc(strlen(text) * (1 + NUL_FILLERS));
  p = text;
  q = clipboard;
  while (*p) {
    int unicode = -1;
    if ((*p & 0x80) == 0) {
      unicode = *p++;
    } else if ((*p & 0xe0) == 0xe0) {
      unicode = (*p++ & 0x1F) << 12;
      unicode |= (*p++ & 0x3F) << 6;
      unicode |= (*p++ & 0x3F);
    } else {
      unicode = (*p++ & 0x3F) << 6;
      unicode |= (*p++ & 0x3F);
    }
    if (unicode != -1) {
      int key = unicode_to_key(unicode);
      if (key != -1) {
        *q++ = key & 0xFF;
        for (i = 0; i < NUL_FILLERS; ++i) *q++ = 0;
      }
    }
  }
  clipboard_current = clipboard;
  clipboard_end = q;
  SDL_free(text);
}

int rcterm_keypressed() {
  SDL_Event event;
  int sym;
  int shift;
  int ctrl;
  int code;

  if (!fifo_empty(&kbdbuf)) return fifo_get(&kbdbuf);
  if (!SDL_PollEvent(&event)) return -1;
  switch (event.type) {
    case SDL_KEYDOWN:
      sym = event.key.keysym.sym;
      shift = event.key.keysym.mod & KMOD_SHIFT;
      ctrl = event.key.keysym.mod & KMOD_CTRL;
      switch (sym) {
        case SDLK_UP: 
          return shift ? 0x9A : 0x1A;

        case SDLK_DOWN: 
          return shift ? 0x8A : 0x0A;

        case SDLK_LEFT:
          return shift ? 0x88 : 0x08;

        case SDLK_RIGHT: 
          return shift ? 0x98 : 0x18;

        case SDLK_BACKSPACE: // Mapped to left/rubout
          return shift ? 0x7F : 0x08;

        case SDLK_INSERT:
          if (shift) paste_clipboard();
          break;

        case SDLK_DELETE: // Mapped to clear
          return shift ? 0x8C : 0x0C;

        case SDLK_HOME: // Mapped to reset
          return 0x81;

        case SDLK_ESCAPE:
          return shift ? 0x9B : 0x1B;

        case SDLK_TAB:
          return shift ? 0x05 : 0x09;

        case SDLK_RETURN:
          return shift ? 0x8D : 0x0D;

        case SDLK_SPACE: 
          return shift ? 0x0A : 0x20;
   
        case SDLK_F1: // Mapped to PA1
          return shift ? 0x94 : 0x83;

        case SDLK_F2: // Mapped to PA2
          return shift ? 0x95 : 0x84;

        case SDLK_F3: // Mapped to PA3
          return shift ? 0x99 : 0x8B;

        case SDLK_F4: // Mapped to PA4
          return shift ? 0x9C : 0x8E;

        case SDLK_F5: // Mapped to PA5
          return shift ? 0x9E : 0x90;

        case SDLK_F10:
          cpu_error = USERINT;
          cpu_state = STOPPED;
          return -1;

        default:
          if (ctrl) return sym - 'a' + 1;
      }
      break;

    case SDL_TEXTINPUT: {
      BYTE *p = event.text.text;
      while (*p) {
        int unicode = -1;
        if ((*p & 0x80) == 0) {
          unicode = *p++;
        } else if ((*p & 0xe0) == 0xe0) {
          unicode = (*p++ & 0x1F) << 12;
          unicode |= (*p++ & 0x3F) << 6;
          unicode |= (*p++ & 0x3F);
        } else {
          unicode = (*p++ & 0x3F) << 6;
          unicode |= (*p++ & 0x3F);
        }
        if (unicode != -1 && unicode != ' ') {
          int key = unicode_to_key(unicode);
          if (key != -1) fifo_put(&kbdbuf, key);
        }
      }
      break;
    }

    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
        SDL_RenderPresent(renderer);
      }
      break;

    case SDL_QUIT:
      cpu_error = USERINT;
      cpu_state = STOPPED;
      return -1;
  }
  return -1;
}

void rcterm_print(BYTE ch) {
  putchar(ch);
}

int rcterm_read_clipboard() {
  if (clipboard_current == clipboard_end) {
    if (clipboard) {
      free(clipboard);
      clipboard = clipboard_current = clipboard_end = NULL;
    }
    return -1;
  }
  return *clipboard_current++;
}

