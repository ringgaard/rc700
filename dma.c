//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// Intel 8237 DMA - Programmable DMA Controller
// 
// Channel 0: Winchester disk controller
// Channel 1: Floppy disk controller
// Channel 2: Visual display controller
// Channel 3: Visual display controller
//

#include <stdio.h>
#include <string.h>

#include "rc700.h"

#define L(x)
#define LL(x)
#define W(x) x

#define NUM_DMA_CHANNELS 4

#define DMA_MODE_VERIFY    0x00
#define DMA_MODE_WRITE     0x04
#define DMA_MODE_READ      0x08

#define DMA_MODE_AUTOINIT  0x10
#define DMA_MODE_DECREMENT 0x20

#define DMA_MODE_DEMAND    0x00
#define DMA_MODE_SINGLE    0x40
#define DMA_MODE_BLOCK     0x80
#define DMA_MODE_CADCADE   0xC0

struct dma_controller {
  BYTE status;
  BYTE command;
  BYTE temp;
  BYTE mask;
  BYTE request;
  WORD temp_adr;
  WORD temp_cnt;

  WORD base_adr[NUM_DMA_CHANNELS];
  WORD curr_adr[NUM_DMA_CHANNELS];
  WORD base_cnt[NUM_DMA_CHANNELS];
  WORD curr_cnt[NUM_DMA_CHANNELS];
  BYTE mode[NUM_DMA_CHANNELS];

  int flipflop;    // Byte pointer flip/flop.
};

struct dma_controller dma;

BYTE dma_status(int reg) {
  BYTE status;

  switch (reg) {
    case 0: 
      // Read status register.
      L(printf("dma: read status %02X PC=%04X\n", dma.status, (WORD) (PC - ram)));
      status = dma.status;
      dma.status &= 0xf0;
      return status;
    case 5:
      // Read temporary register.
      L(printf("dma: read temp %02X\n", dma.temp));
      return dma.temp;
    case 1: 
    case 2: 
    case 3: 
    case 4: 
    case 6: 
    case 7: 
      W(printf("dma: illegal register %d\n", reg));
  }
  return 0; 
}

void dma_command(BYTE data, int reg) {
  int channel;
  int value;

  LL(printf("dma: write %02X to reg %02X\n", data, reg));
  switch (reg) {
    case 0:
      // Write command register.
      L(printf("dma: write command reg %02X\n", data));
      dma.command = data;
      break;

    case 1:
      // Write request register.
      channel = data & 0x03;
      value = (data >> 3) & 0x01;
      L(printf("dma%d: %s request\n", channel, value ? "set" : "clear"));
      if (value) {
        dma.request |= 1 << channel;
      } else {
        dma.request &= ~(1 << channel);
      }
      break;

    case 2:
      // Write single mask register bit.
      channel = data & 0x03;
      value = (data >> 3) & 0x01;
      LL(printf("dma%d: %s mask\n", channel, value ? "set" : "clear"));
      if (value) {
        dma.mask |= 1 << channel;
      } else {
        dma.mask &= ~(1 << channel);
      }
      break;

    case 3:
      // Write mode register.
      channel = data & 0x03;
      value = data & 0xfc;
      L(printf("dma%d: write mode reg %02X\n", channel, value));
      dma.mode[channel] = value;
      break;

    case 4:
      // Clear byte pointer flip/flop.
      LL(printf("dma: clear pointer flip/flop %02X\n", data));
      dma.flipflop = 0;
      break;

    case 5:
      // Master clear.
      L(printf("dma: master clear\n"));
      memset(&dma, 0, sizeof(struct dma_controller));
      break;

    case 6:
      // Illegal command.
      W(printf("dma: illegal command %02X\n", data));
      break;

    case 7: break;
      // Write all mask register bits.
      L(printf("dma: write all mask reg %02X\n", data));
      dma.mask = data & 0x0f;
      break;
  }
}

BYTE dma_adr_in(int channel) {
  L(printf("dma%d: read addr %02X (%s)\n", channel, dma.curr_adr[channel], dma.flipflop ? "hi" : "lo"));
  return dma.flipflop ? dma.curr_adr[channel] >> 8 : dma.curr_adr[channel] & 0xff;
}

void dma_adr_out(BYTE data, int channel) {
  L(if (channel < 2) printf("dma%d: write addr %02X (%s)\n", channel, data, dma.flipflop ? "hi" : "lo"));
  if (dma.flipflop) {
    dma.base_adr[channel] = (dma.base_adr[channel] & 0x00ff) | (data << 8);
  } else {
    dma.base_adr[channel] = (dma.base_adr[channel] & 0xff00) | (data & 0xff);
  }

  dma.curr_adr[channel] = dma.base_adr[channel];
  dma.flipflop = !dma.flipflop;
}

BYTE dma_cnt_in(int channel) {
  L(printf("dma%d: read count %02X (%s)\n", channel, dma.curr_cnt[channel], dma.flipflop ? "hi" : "lo"));
  return dma.flipflop ? dma.curr_cnt[channel] >> 8 : dma.curr_cnt[channel] & 0xff;
}

void dma_cnt_out(BYTE data, int channel) {
  L(if (channel < 2) printf("dma%d: write count %02X (%s)\n", channel, data, dma.flipflop ? "hi" : "lo"));
  if (dma.flipflop) {
    dma.base_cnt[channel] = (dma.base_cnt[channel] & 0x00ff) | (data << 8);
  } else {
    dma.base_cnt[channel] = (dma.base_cnt[channel] & 0xff00) | (data & 0xff);
  }

  dma.curr_cnt[channel] = dma.base_cnt[channel];
  if (dma.curr_cnt[channel] != 0) dma.status &= ~(1 << channel);
  dma.flipflop = !dma.flipflop;
}

void dma_transfer_start(int channel) {
  dma.status |= (1 << (channel + 4));
}

void dma_transfer(int channel, BYTE *data, int  bytes) {
  WORD adr = dma.curr_adr[channel];
  WORD cnt = dma.curr_cnt[channel];
  int dir = dma.mode[channel] & DMA_MODE_DECREMENT ? -1 : 1;
  BYTE *end = data + bytes;
  while (data < end) {
    *(ram + adr) = *data++;
    adr += dir;
    if (cnt == 0) break;
    cnt--;
  }
  dma.curr_adr[channel] = adr;
  dma.curr_cnt[channel] = cnt;
  if (cnt == 0) {
    L(printf("dma: tc on channel %d\n", channel));
    dma.status |= (1 << channel);
  }
}

void dma_fill(int channel, BYTE value, int  bytes) {
  WORD adr = dma.curr_adr[channel];
  WORD cnt = dma.curr_cnt[channel];
  int dir = dma.mode[channel] & DMA_MODE_DECREMENT ? -1 : 1;
  while (bytes > 0) {
    *(ram + adr) = value;
    adr += dir;
    if (cnt == 0) break;
    cnt--;
    bytes--;
  }
  dma.curr_adr[channel] = adr;
  dma.curr_cnt[channel] = cnt;
  if (cnt == 0) {
    L(printf("dma: tc on channel %d\n", channel));
    dma.status |= (1 << channel);
  }
}

WORD dma_fetch(int channel, WORD *size) {
  WORD len = *size;
  WORD adr = dma.curr_adr[channel];
  WORD cnt = dma.curr_cnt[channel];
  WORD left = cnt + 1;

  if (cnt == 0) {
    dma.status |= (1 << channel);
    *size = 0;
    return adr;
  }

  if (dma.mode[channel] & DMA_MODE_DECREMENT) {
    L(printf("dma%d: cannot fetch in decrement mode\n", channel));
  }

  if (len > left) len = left;

  dma.curr_adr[channel] += len;
  dma.curr_cnt[channel] -= len;
  if (dma.curr_cnt[channel] == 0xFFFF) {
    dma.curr_cnt[channel] = 0;
    dma.status |= (1 << channel);
  }

  *size = len;
  return adr;
}

void dma_transferred(int channel) {
  dma.curr_adr[channel] += dma.curr_cnt[channel];
  dma.curr_cnt[channel] = 0;
  dma.status |= (1 << channel);
}

void dma_transfer_done(int channel) {
  dma.status &= ~(1 << (channel + 4));
}

int dma_completed(int channel) {
  return dma.status & (1 << channel);
}

WORD dma_address(int channel) {
  return dma.curr_adr[channel];
}

WORD dma_count(int channel) {
  return dma.curr_cnt[channel];
}

void init_dma() {
  int i;

  memset(&dma, 0, sizeof(struct dma_controller));
  for (i = 0; i < NUM_DMA_CHANNELS; ++i) {
    register_port(0xf0 + i * 2 + 0, dma_adr_in, dma_adr_out, i);
    register_port(0xf0 + i * 2 + 1, dma_cnt_in, dma_cnt_out, i);
  }
  for (i = 0; i < 8; ++i) {
    register_port(0xf8 + i, dma_status, dma_command, i);
  }
}

