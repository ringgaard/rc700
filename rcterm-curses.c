//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// Curses-based RC700 terminal
//

#define _XOPEN_SOURCE_EXTENDED
// apt-get install libncursesw5-dev

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <curses.h>

#include "rc700.h"

#define L(x)

#define NOCHAR   0x2423
#define MAXUTF8  5

// Unicode equvalents for RC700 character set.
int rcchars[256] = {
  /* 00 */ 0x00FC, 0x00E0, 0x00D1, 0x00A3, 0x2613, 0x0040, 0x00C4, 0x00CB, 
  /* 08 */ 0x00C7, 0x00F5, 0x00C9, 0x005B, 0x005C, 0x005D, 0x00F6, 0x007E,
  /* 10 */ 0x00A8, 0x03B2, 0x00F1, 0x00A7, 0x00F9, 0x00E8, 0x0060, 0x00EB,
  /* 18 */ 0x00E7, 0x00EC, 0x00E9, 0x007B, 0x007C, 0x007D, 0x03B1, 0x00F2,
  /* 20 */ 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
  /* 28 */ 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
  /* 30 */ 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  /* 38 */ 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
  /* 40 */ 0x00FC, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  /* 48 */ 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  /* 50 */ 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  /* 58 */ 0x0058, 0x0059, 0x005A, 0x00C6, 0x00D8, 0x00C5, 0x2191, 0x005F,
  /* 60 */ 0x00E4, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
  /* 68 */ 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
  /* 70 */ 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  /* 78 */ 0x0078, 0x0079, 0x007A, 0x00E6, 0x00F8, 0x00E5, 0x00F6, 0x2637,
  /* 80 */ 0x250C, 0x2510, 0x2514, 0x2518, 0x252C, 0x2524, 0x251C, 0x2534,
  /* 88 */ 0x2500, 0x2502, 0x253C, 0x2572, 0x2571, NOCHAR, NOCHAR, NOCHAR,
  /* 90 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, 0x2573,
  /* 98 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, 0x2666, 0x2663, 0x2665, 0x2663,
  /* A0 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
  /* A8 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
  /* B0 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
  /* B8 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
  /* C0 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
  /* C8 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
  /* D0 */ 0x00FC, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
  /* D8 */ 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
  /* E0 */ 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
  /* E8 */ 0x0058, 0x0059, 0x005A, 0x00C6, 0x00D8, 0x00C5, 0x2191, 0x005F,
  /* F0 */ 0x0020, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
  /* F8 */ NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR, NOCHAR,
};

static WINDOW *term;
static int xofs = 0;
static int yofs = 0;

static int toutf8(unsigned long ch, char *dest) {
  if (ch < 0x80) {
    dest[0] = (char) ch;
    dest[1] = 0;
    return 1;
  }
  if (ch < 0x800) {
    dest[0] = (ch >> 6) | 0xC0;
    dest[1] = (ch & 0x3F) | 0x80;
    dest[2] = 0;
    return 2;
  }
  if (ch < 0x10000) {
    dest[0] = (ch >> 12) | 0xE0;
    dest[1] = ((ch >> 6) & 0x3F) | 0x80;
    dest[2] = (ch & 0x3F) | 0x80;
    dest[3] = 0;
    return 3;
  }
  if (ch < 0x110000) {
    dest[0] = (ch >> 18) | 0xF0;
    dest[1] = ((ch >> 12) & 0x3F) | 0x80;
    dest[2] = ((ch >> 6) & 0x3F) | 0x80;
    dest[3] = (ch & 0x3F) | 0x80;
    dest[4] = 0;
    return 4;
  }
  
  dest[0] = 0;
  return 0;
}

void rcterm_init() {
  L(printf("rcterm: init\n"));
  setlocale(LC_ALL,"");
  setenv("ESCDELAY","100");
  term = initscr();
  start_color();
  nonl();
  cbreak();
  noecho();
  keypad(term, TRUE);
  nodelay(term, TRUE);
  use_default_colors();
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);
}

void rcterm_exit() {
  L(printf("rcterm: exit\n"));
  delwin(term);
  endwin();
  refresh();
}

void rcterm_clear_screen(int cols, int rows) {
  int width, height;
  int x, y;

  L(printf("rcterm: clear screen\n"));
  getmaxyx(term, height, width);
  xofs = (width - cols) / 2;
  if (xofs < 0) xofs = 0;
  yofs = (height - rows) / 2;
  if (yofs < 0) yofs = 0;
  attron(COLOR_PAIR(0));
  erase();
  attron(COLOR_PAIR(1));
  for (y = 0; y < rows; ++y) {
    move(y + yofs, xofs);
    for (x = 0; x < cols; ++x) addch(' ');
  }
  attron(COLOR_PAIR(0));
  refresh();
}

void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows) {
  int y, x, curx, cury;
  BYTE *p;
  char buffer[MAXUTF8];

  getyx(term, cury, curx);
  p = screen;
  attron(COLOR_PAIR(1));
  for (y = 0; y < rows; ++y) {
    move(y + yofs, xofs);
    for (x = 0; x < cols; ++x) {
      //if (rcchars[*p] == NOCHAR) printw("<%02X>", *p);
      toutf8(rcchars[*p++], buffer);
      addstr(buffer);
    }
  }
  attron(COLOR_PAIR(0));
  move(cury, curx);
  refresh();
}

void rcterm_set_cursor(int type, int underline) {
  L(printf("rcterm: set cursor type %d underline %d\n", type, underline));
}

int rcterm_gotoxy(int col, int row) {
  L(printf("rcterm: gotoxy col=%d, row=%d\n", col, row));
  move(row + yofs, col + xofs);
  refresh();
  return 0;
}

int rcterm_keypressed() {
  int ch = getch();
  if (ch == ERR) return -1;
  switch (ch) {
    case KEY_UP: return 0x1A;
    case KEY_DOWN: return 0x0A;
    case KEY_LEFT: return 0x08;
    case KEY_RIGHT: return 0x18;
    case KEY_BACKSPACE: return 0x08;
    case 0x1B: return getch() == ERR ? 0x1B : -1;
    default: if (ch >= 0 && ch <= 0xFF) return ch;
  }
  return -1;
}

