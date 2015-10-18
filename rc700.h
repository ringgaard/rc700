//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//

#include "cpu.h"

// fifo.c

#define FIFOSIZE        128

struct fifo {
  BYTE queue[FIFOSIZE];
  int count;
  int head;
  int tail;
};

int fifo_put(struct fifo *f, BYTE data);
BYTE fifo_get(struct fifo *f);
int fifo_empty(struct fifo *f);
int fifo_full(struct fifo *f);

// rc700.c

struct port {
  BYTE (*in)(int dev);
  void (*out)(BYTE data, int dev);
  int dev;
};

void register_port(int adr, BYTE (*in)(int dev), void (*out)(BYTE data, int dev), int dev);

// ctc.c

#define CTC_CHANNEL_SIOA  0
#define CTC_CHANNEL_SIOB  1
#define CTC_CHANNEL_CRT   2
#define CTC_CHANNEL_FDC   3
#define CTC_CHANNEL_WDC   4

void init_ctc();
void ctc_trigger(int channel);

// rom.c

void init_rom();

// pio.c

void init_pio();
int pio_poll();

// sio.c

void init_sio();

// dma.c

void init_dma();
WORD dma_address(int channel);
WORD dma_count(int channel);
int dma_completed(int channel);
void dma_transfer_start(int channel);
void dma_transfer(int channel, BYTE *data, int  bytes);
void dma_fill(int channel, BYTE value, int bytes);
BYTE *dma_fetch(int channel, int *size);
void dma_transfer_done(int channel);
int dma_read_ready(int channel);
int dma_write_ready(int channel);

// crt.c

void init_crt();
int crt_poll();
void dump_screen();

// fdc.c

#define FDC_READONLY     1
#define FDC_WRITEPROTECT 2

void init_fdc();
void fdc_floppy_motor(BYTE data, int dev);
int fdc_mount_disk(int drive, char *imagefile, int flags);
void fdc_flush_disk(int drive);

// wdc.c

void init_wdc();
int wdc_mount_harddisk(int drive, char *imagefile);

// ftp.c

void init_ftp();

// memdisk.c
void init_mdc();

// monitor.c

void mon();

// disasm.c

void disasm(unsigned char **p, int adr);

// screen.c

typedef unsigned int pixel32_t;
typedef unsigned short pixel16_t;

#define XSCALE 3/2
#define YSCALE 2

#define SCREEN_WIDTH  (80 * 7 * XSCALE)
#define SCREEN_HEIGHT (25 * 11 * YSCALE)
#define SCREEN_BPP    32

extern int cursor_type;
extern int under_line;
extern int cur_x;
extern int cur_y;

void draw_screen32(pixel32_t *bitmap, pixel32_t *palette, int pitch, int xmargin, int ymargin, unsigned char *text);
void draw_screen16(pixel16_t *bitmap, pixel16_t *palette, int pitch, int xmargin, int ymargin, unsigned char *text);

// rcterm-*.c

void rcterm_init();
void rcterm_exit();
void rcterm_clear_screen(int cols, int rows);
void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows);
void rcterm_set_cursor(int type, int underline);
int rcterm_gotoxy(int col, int row);
int rcterm_keypressed();
void rcterm_print(BYTE ch);
int rcterm_read_clipboard();

