//
// RC700  -  a Regnecentralen RC700 simulator
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

void init_ctc();
void ctc_trigger(int channel);

// rom.c

void init_rom();

// pio.c

void init_pio();
int poll_pio();

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
WORD dma_fetch(int channel, WORD *size);
void dma_transfer_done(int channel);

// crt.c

void init_crt();
int poll_crt();

// fdc.c

void init_fdc();
int fdc_mount_disk(int drive, char *imagefile);
void fdc_flush_disk(int drive);

// ftp.c

void init_ftp();

// disasm.c

void mon();

// rcterm-*.c

void rcterm_init();
void rcterm_exit();
void rcterm_clear_screen(int cols, int rows);
void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows);
void rcterm_set_cursor(int type, int underline);
int rcterm_gotoxy(int col, int row);
int rcterm_keypressed();

