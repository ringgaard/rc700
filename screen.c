//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// Screen rendering
//

#include <stdio.h>
#include <string.h>

#include "rc700.h"

//
// Character attributes:
//   0x80 Change character mode
//   0x01 Highlight (not used in RC702)
//   0x02 Blink
//   0x04 Semigraphic character set
//   0x10 Reverse
//   0x20 Underline
//
// Special codes:
//   0xF0 End of row
//   0xF1 End of row, stop DMA
//   0xF2 End of screen
//   0xF3 End of screen, stop DMA
//
//   Character matrix 7x11 bits
//

#define ATTR_MODE      0x80
#define ATTR_HIGHLIGHT 0x01
#define ATTR_BLINK     0x02
#define ATTR_SEMI      0x04
#define ATTR_REVERSE   0x10
#define ATTR_UNDERLINE 0x20

#define ATTR_SPECIAL   0xF0
#define ATTR_CLREOL    0xF0
#define ATTR_CLREOS    0xF2

extern unsigned char charrom[];
extern unsigned char charram[];

int cursor_type = 0;
int under_line = 9;
int cur_x = -1;
int cur_y = -1;

unsigned char blank[] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

void draw_screen32(pixel32_t *bitmap, pixel32_t *palette, int pitch, int xmargin, int ymargin, int stretch, unsigned char *text) {
  int row, col, line, blk, grp;
  int clreos, clreol, uline;
  unsigned char **c;
  unsigned char *a;
  unsigned char mode;
  unsigned char ch;
  unsigned char m;
  unsigned char pixels;
  pixel32_t mask;
  pixel32_t *begin;
  pixel32_t *pix;
  unsigned char *charmem;
  unsigned char *cell[80];
  unsigned char attr[80];

  // Skip margins.
  bitmap += pitch * ymargin + xmargin;

  // Generate 25 lines of text.
  mode = 0;
  clreos = 0;
  uline = 10 - under_line;
  for (row = 0; row < 25; ++row) {
    // Set up pointers into the character cell ROM.
    c = cell;
    a = attr;
    clreol = clreos;
    charmem = mode & ATTR_SEMI ? charram : charrom;
    for (col = 0; col < 80; ++col) {
      ch = *text++;
      if (ch & ATTR_MODE) {
        if ((ch & ATTR_SPECIAL) == ATTR_SPECIAL) {
          switch (ch) {
            case ATTR_CLREOL: clreol = 1; break;
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
        if (col == cur_x && row == cur_y) {
          m ^= (cursor_type & 0x01) ? ATTR_UNDERLINE : ATTR_REVERSE;
        }
        *a++ = m;
        *c++ = charmem + (ch << 4);
      }
    }

    // Each character has 11 lines.
    for (line = 11; line > 0; --line) {
      pix = bitmap;
      c = cell;
      if (stretch) {
        // Generate 20 * 4 = 80 characters per line.
        a = attr;
        begin = pix;
        for (blk = 20; blk > 0; --blk) {
          // Fetch next 4 characters, each 7 pixels wide.
          mask = 0;
          for (grp = 4; grp > 0; --grp) {
            pixels = *(*c++)++;
            m = *a++;
            if (m) {
              if (m & ATTR_UNDERLINE && line == uline) pixels = 0x7F;
              if (m & ATTR_REVERSE) pixels ^= 0x7F;
            }
            mask = (mask >> 7) | (pixels << 21);
          }

          // Generate two pixels at a time.
          for (grp = 14; grp > 0; --grp) {
            // Generate three screen pixels per character pixel pair.
            int color = mask & 0x03;
            pixel32_t *p = palette + (color << 2);
            *pix++ = *p++;
            *pix++ = *p++;
            *pix++ = *p++;
            mask >>= 2;
          }
        }
        bitmap += pitch;

        // Duplicate previous pixel line.
        memcpy(bitmap, begin, 40 * 3 * 7 * sizeof(pixel32_t));
        bitmap += pitch;
      } else {
        // Generate 80 characters * 7 pixels per line.
        a = attr;
        for (blk = 80; blk > 0; --blk) {
          pixels = *(*c++)++;
          m = *a++;
          if (m) {
            if (m & ATTR_UNDERLINE && line == uline) pixels = 0x7F;
            if (m & ATTR_REVERSE) pixels ^= 0x7F;
          }

          for (grp = 7; grp > 0; --grp) {
            int color = pixels & 0x01;
            *pix++ = palette[color << 2];
            pixels >>= 1;
          }
        }
        bitmap += pitch;
      }
    }
  }
}

void draw_screen16(pixel16_t *bitmap, pixel16_t *palette, int pitch, int xmargin, int ymargin, unsigned char *text) {
  int row, col, line, blk, grp;
  int clreos, clreol, uline;
  unsigned char **c;
  unsigned char *a;
  unsigned char mode;
  unsigned char ch;
  unsigned char m;
  unsigned char pixels;
  pixel32_t mask;
  pixel16_t *begin;
  pixel16_t *pix;
  unsigned char *charmem;
  unsigned char *cell[80];
  unsigned char attr[80];

  // Skip margins.
  bitmap += pitch * ymargin + xmargin;

  // Generate 25 lines of text.
  mode = 0;
  clreos = 0;
  uline = 10 - under_line;
  for (row = 0; row < 25; ++row) {
    // Set up pointers into the character cell ROM.
    c = cell;
    a = attr;
    clreol = clreos;
    charmem = mode & ATTR_SEMI ? charram : charrom;
    for (col = 0; col < 80; ++col) {
      ch = *text++;
      if (ch & ATTR_MODE) {
        if ((ch & ATTR_SPECIAL) == ATTR_SPECIAL) {
          switch (ch) {
            case ATTR_CLREOL: clreol = 1; break;
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
        if (col == cur_x && row == cur_y) {
          m ^= (cursor_type & 0x01) ? ATTR_UNDERLINE : ATTR_REVERSE;
        }
        *a++ = m;
        *c++ = charmem + (ch << 4);
      }
    }

    // Each character has 11 lines.
    for (line = 11; line > 0; --line) {
      begin = pix = bitmap;
      c = cell;

      // Generate 20 * 4 = 80 characters per line.
      a = attr;
      for (blk = 20; blk > 0; --blk) {
        // Fetch next 4 characters, each 7 pixels wide.
        mask = 0;
        for (grp = 4; grp > 0; --grp) {
          pixels = *(*c++)++;
          m = *a++;
          if (m) {
            if (m & ATTR_UNDERLINE && line == uline) pixels = 0x7F;
            if (m & ATTR_REVERSE) pixels ^= 0x7F;
          }
          mask = (mask >> 7) | (pixels << 21);
        }

        // Generate two pixels at a time.
        for (grp = 14; grp > 0; --grp) {
          // Generate three screen pixels per character pixel pair.
          int color = mask & 0x03;
          pixel16_t *p = palette + (color << 2);
          *pix++ = *p++;
          *pix++ = *p++;
          *pix++ = *p++;
          mask >>= 2;
        }
      }
      bitmap += pitch;

      // Duplicate previous pixel line.
      memcpy(bitmap, begin, 40 * 3 * 7 * sizeof(pixel16_t));
      bitmap += pitch;
    }
  }
}

