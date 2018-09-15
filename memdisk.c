//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// MEMDISK - Memory Disk (custom RC700 hardware) 
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "rc700.h"

#define L(x) x
#define LL(x) x
#define W(x) x

// Geometry of memory disk.
#define SECTSIZE 1024
#define TRACKS   64
#define SECTORS  16

struct memdisk_controller {
  BYTE status;
  BYTE track;
  BYTE sect;
  int blank;
  FILE *disk;
  char buffer[SECTSIZE];
};

struct memdisk_controller mdc;

BYTE mdc_status_in(int dev) {
  W(printf("mdc: status %02x\n", mdc.status));
  return mdc.status;
}

void mdc_track_out(BYTE data, int dev) {
  L(printf("mdc: set track number %d\n", data));
  mdc.track = data & (TRACKS - 1);
  mdc.status &= ~3;
}

BYTE mdc_sect_in(int reg) {
  L(printf("mdc: read sector number %d\n", mdc.sect));
  return mdc.sect;
}

void mdc_read() {
  int n;

  if (mdc.track >= TRACKS || mdc.sect >= SECTORS) {
    W(printf("mdc: illegal read, track %d, sector %d\n", mdc.track, mdc.sect));
    return;
  }

  dma_transfer_start(0);
  while (!dma_completed(0)) {
    L(printf("mdc: read track %d sector %d\n", mdc.track, mdc.sect));
    fseek(mdc.disk, ((mdc.track * SECTORS) + mdc.sect) * SECTSIZE, SEEK_SET);
    n = fread(mdc.buffer, 1, SECTSIZE, mdc.disk);
    if (n < SECTSIZE) mdc.status |= 1;
    dma_transfer(0, mdc.buffer, SECTSIZE);
    mdc.sect++;
    if (mdc.sect >= SECTORS) {
      mdc.sect = 0;
      mdc.track++;
    }
  }
  dma_transfer_done(0);
}

void mdc_write() {
  int n;

  if (mdc.track >= TRACKS || mdc.sect >= SECTORS) {
    W(printf("mdc: illegal read, track %d, sector %d\n", mdc.track, mdc.sect));
    return;
  }

  while (!dma_completed(0)) {
    int size = SECTSIZE;
    BYTE *addr = dma_fetch(0, &size);

    L(printf("mdc: write track %d sector %d, %d bytes\n", mdc.track, mdc.sect, size));
    fseek(mdc.disk, ((mdc.track * SECTORS) + mdc.sect) * SECTSIZE, SEEK_SET);
    n = fwrite(addr, 1, size, mdc.disk);
    if (n < size) mdc.status |= 1;
    while (size > 0) {
      mdc.sect++;
      if (mdc.sect >= SECTORS) {
        mdc.sect = 0;
        mdc.track++;
      }
      size -= SECTSIZE;
    }
  }
  fflush(mdc.disk);
  dma_transfer_done(0);
}

void mdc_sect_out(BYTE data, int reg) {
  L(printf("mdc: set sector number %d\n", data));
  mdc.sect = data & (SECTORS - 1);
  if (dma_write_ready(0)) {
    mdc_read();
  } else if (dma_read_ready(0)) {
    mdc_write();
  } else {
    W(printf("mdc: DMA not ready\n"));
  }
}

void init_mdc() {
  memset(&mdc, 0, sizeof(struct memdisk_controller));
  mdc.status = TRACKS;
  mdc.disk = fopen("memdisk.img", "r+b"); 
  register_port(0xee, mdc_status_in, mdc_track_out, 0);
  register_port(0xef, mdc_sect_in, mdc_sect_out, 0);
}

