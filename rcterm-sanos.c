//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// RC700 terminal for Sanos
//

#include <stdio.h>
#include <string.h>
#include <os/vga.h>
#include <os.h>

#include "rc700.h"

// RC752 amber colors.
#define HI_COLOR 0xFFCC66
#define FG_COLOR 0xCC9933
#define BG_COLOR 0x552200
#define MI_COLOR 0x996611
#define BORDER   0x000000

pixel32_t palette32[16] = {
  BG_COLOR, BG_COLOR, BG_COLOR, 0,
  FG_COLOR, MI_COLOR, BG_COLOR, 0,
  BG_COLOR, MI_COLOR, FG_COLOR, 0,
  FG_COLOR, FG_COLOR, FG_COLOR, 0,
};

#define MARGINX  92
#define MARGINY  109

static struct vesa_mode_info mode;
static pixel16_t *surface;
static int pitch;
static pixel16_t palette[16];
sigset_t blocked_sigmask, orig_sigmask;

static pixel16_t screen_color(pixel32_t rgb) {
  BYTE r = (rgb >> 16) & 0xFF;
  BYTE g = (rgb >> 8) & 0xFF;
  BYTE b = rgb & 0xFF;
  return ((r >> (8 - mode.red_mask_size)) << mode.red_mask_pos) |
         ((g >> (8 - mode.green_mask_size)) << mode.green_mask_pos) |
         ((b >> (8 - mode.blue_mask_size)) << mode.blue_mask_pos);
}

static void fill_rect(int x, int y, int width, int height, pixel16_t color) {
  int w, h, skip;
  pixel16_t *pixels = surface + y * pitch + x;
  skip = pitch - width;
  for (h = 0; h < height; ++h) {
    for (w = 0; w < width; ++w) *pixels++ = color;
    pixels += skip;
  }
}

void rcterm_init() {
  int i;

  // Get frame buffer and video mode info.
  int fb = open("/dev/fb0", 0);
  if (fb < 0) {
    printf("rcterm: no frame buffer\n");
    abort();
  }
  ioctl(fb, IOCTL_VGA_GET_VIDEO_MODE, &mode, sizeof(struct vesa_mode_info));
  ioctl(fb, IOCTL_VGA_GET_FRAME_BUFFER, &surface, sizeof(pixel16_t *));
  close(fb);

  // Initialize video mode.
  for (i = 0; i < 16; ++i) palette[i] = screen_color(palette32[i]);
  pitch = mode.bytes_per_scanline / sizeof(pixel16_t);
  fill_rect(0, 0, mode.x_resolution, mode.y_resolution, screen_color(BORDER));

  sigemptyset(&blocked_sigmask);
  sigaddset(&blocked_sigmask, SIGINT);
  sigaddset(&blocked_sigmask, SIGTSTP);
  sigaddset(&blocked_sigmask, SIGABRT);
  sigprocmask(SIG_BLOCK, &blocked_sigmask, &orig_sigmask);
}

void rcterm_exit() {
  sigprocmask(SIG_SETMASK, &orig_sigmask, NULL);
}

void rcterm_clear_screen(int cols, int rows) {
  fill_rect(MARGINX / 2, MARGINY / 2, mode.x_resolution - MARGINX, mode.y_resolution - MARGINY, screen_color(BG_COLOR));
}

void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows) {
  draw_screen16(surface, palette, pitch, MARGINX, MARGINY, screen);
}

void rcterm_set_cursor(int type, int underline) {
  cursor_type = type;
  under_line = underline;
}

int rcterm_gotoxy(int col, int row) {
  if (col == cur_x && row == cur_y) return 0;
  cur_x = col;
  cur_y = row;
  return 1;
}

int rcterm_keypressed() {
  unsigned char c;
  int rc;

  rc = ioctl(0, IOCTL_KBHIT, NULL, 0);
  if (rc <= 0) return -1;

  rc = read(0, &c, 1);
  if (rc != 1) return -1;

  if (c == 0) {
    rc = read(0, &c, 1);
    if (rc != 1) return -1;
    switch (c) {
      case 0x0F: return 0x05; // shift-tab
      case 0x47: return 0x81; // home
      case 0x48: return 0x1A; // up
      case 0x4B: return 0x08; // left
      case 0x4D: return 0x18; // right
      case 0x50: return 0x0A; // down
      case 0x53: return 0x0C; // delete
    }
    return -1;
  } else {
    return c;
  }
}

void rcterm_print(BYTE ch) {
}

int rcterm_read_clipboard() {
  return -1;
}

