/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * Curses-based RC700 terminal
 */

#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "sim.h"
#include "simglb.h"

#define L(x)

static WINDOW *term;
static int xofs = 0;
static int yofs = 0;

void rcterm_init(void) {
  L(printf("rcterm: init\n"));
  term = initscr();
  start_color();
  nonl();
  cbreak();
  noecho();
  nodelay(term, TRUE);
  use_default_colors();
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(1, COLOR_YELLOW, COLOR_BLACK);
}

void rcterm_exit(void) {
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
  int y, curx, cury;
  BYTE *p;

  getyx(term, cury, curx);
  p = screen;
  attron(COLOR_PAIR(1));
  for (y = 0; y < rows; ++y) {
    mvaddnstr(y + yofs, xofs, p, cols);
    p += cols;
  }
  attron(COLOR_PAIR(0));
  move(cury, curx);
  refresh();
}

void rcterm_gotoxy(int col, int row) {
  L(printf("rcterm: gotoxy col=%d, row=%d\n", col, row));
  move(row + yofs, col + xofs);
  refresh();
}

int rcterm_keypressed(void) {
  int ch = getch();
  if (ch == ERR) return -1;
  return ch;
}

