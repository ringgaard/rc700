//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// VT100-based RC700 terminal
//

#include <stdio.h>
#include <string.h>
#include <sys/poll.h>

#include "rc700.h"

int curx, cury;

void rcterm_init() {
  fputs("\033[2J\033[7l", stdout);
  curx = cury = 0;
  setvbuf(stdin, NULL, _IONBF, 0);
}

void rcterm_exit() {
  fputs("\f\033[7h", stdout);
}

void rcterm_clear_screen(int cols, int rows) {
  fputs("\033[2J", stdout);
  curx = cury = 0;
}

void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows) {
  int l;
  BYTE *line;

  fputs("\033[H", stdout);
  line = screen;
  for (l = 0; l < rows; ++l) {
    fwrite(line, 1, cols, stdout);
    fputs("\033[K\n", stdout);
    line += cols;
  }
  printf("\033[J\033[%d;%dH", cury + 1, curx + 1);
  fflush(stdout);
}

void rcterm_set_cursor(int type, int underline) {
}

int rcterm_gotoxy(int col, int row) {
  curx = col;
  cury = row;
  printf("\033[%d;%dH", cury + 1, curx + 1);
  fflush(stdout);
}

int rcterm_keypressed() {
  struct pollfd fds;
  int rc;
  int ch;
  
  fds.fd = 0;
  fds.events = POLLIN;
  rc = poll(&fds, 1, 0);
  if (rc != 1) return -1;
  return getchar();
}

void rcterm_print(BYTE ch) {
}

int rcterm_read_clipboard() {
  return -1;
}

