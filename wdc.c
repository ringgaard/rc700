//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// WDC1002 - Winchester Disk Controller 
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "rc700.h"

#define L(x)
#define LL(x)
#define W(x) x

#define MAX_DRIVES 4

#define WDC_REG_ERROR   0
#define WDC_REG_SECTCNT 1
#define WDC_REG_SECTNO  2
#define WDC_REG_CYLLO   3
#define WDC_REG_CYLHI   4
#define WDC_REG_SDH     5

#define WDC_NUM_REGS    6

#define WDC_CMD_RESTORE 0x10
#define WDC_CMD_TEST    0x90
#define WDC_CMD_SEEK    0x70
#define WDC_CMD_READ    0x20
#define WDC_CMD_WRITE   0x30
#define WDC_CMD_FORMAT  0x50

#define WDC_STAT_ERROR            0x01
#define WDC_STAT_CORRECTED_DATA   0x04
#define WDC_STAT_DATA_REQUEST     0x08
#define WDC_STAT_SEEK_COMPLETE    0x10
#define WDC_STAT_WRITE_FAULT      0x20
#define WDC_STAT_DRIVE_READY      0x40
#define WDC_STAT_BUSY             0x80

// Geometry of 10 MB Winchester disk.
#define SECTSIZE 512
#define CYLS     192
#define HEADS    6
#define SECTORS  16

struct winchester_disk_controller {
  BYTE status;
  BYTE reg[WDC_NUM_REGS];
  FILE *drive[MAX_DRIVES];
  char buffer[SECTSIZE];
};

int sector_size[4] = {256, 512, 1024, 128};

struct winchester_disk_controller wdc;

BYTE wdc_data_in(int dev) {
  W(printf("wdc: data in not implemented\n"));
  return 0;
}

void wdc_data_out(BYTE data, int dev) {
  W(printf("wdc: data out %02X, not implemented\n", data));
}

BYTE wdc_reg_in(int reg) {
  LL(printf("wdc: read reg%d, %02X \n", reg, wdc.reg[reg]));
  return wdc.reg[reg];
}

void wdc_reg_out(BYTE data, int reg) {
  LL(printf("wdc: write %02X to reg%d\n", data, reg));
  wdc.reg[reg] = data;
}

BYTE wdc_status(int reg) {
  LL(printf("wdc: status %02X\n", wdc.status));
  return wdc.status;
}

void wdc_read() {
  FILE *drive;
  WORD adr = 0;
  WORD cnt = 0;
  
  // Get parameters.
  int d = (wdc.reg[WDC_REG_SDH] >> 3) & 0x03;
  int c = wdc.reg[WDC_REG_CYLLO]  + (wdc.reg[WDC_REG_CYLHI] << 8);
  int h = wdc.reg[WDC_REG_SDH] & 0x07;
  int s = wdc.reg[WDC_REG_SECTNO];
  int n = wdc.reg[WDC_REG_SECTCNT];
  int size = sector_size[(wdc.reg[WDC_REG_SDH] >> 5) & 0x03];
  L(printf("wdc: read: drive=%d c=%d h=%d, s=%d, n=%d, sector size %d, addr=%02X cnt=%d\n", d, c, h, s, n, size, dma_address(0), dma_count(0)));

  // Get mounted disk image for drive.
  drive = wdc.drive[d];
  if (!drive) {
    wdc.status |= WDC_STAT_ERROR;
    wdc.reg[WDC_STAT_ERROR] = 1 << 6;
    wdc.status &= ~WDC_STAT_DRIVE_READY;
    return;
  }

  // Read sectors until we reach terminal count for DMA.
  if (!n) n = 32;
  dma_transfer_start(0);
  while (n > 0 && !dma_completed(0)) {
    if (c < CYLS && h < HEADS && s < SECTORS) {
      // Read sector from disk.
      L(printf("fdc: read sector C=%d,H=%d,S=%d: read %d bytes to %04Xs\n", c, s, s, size, dma_address(0)));
      if (fseek(drive, ((((c * HEADS) + h) * SECTORS + s) * SECTSIZE), SEEK_SET) == -1) {
        W(printf("wdc%d: read seek error %d sector C=%d,H=%d,S=%d\n", d, errno, c, h, s));
        wdc.status |= WDC_STAT_ERROR;
        break;
      }
      if (fread(wdc.buffer, 1, SECTSIZE, drive) != SECTSIZE) {
        W(printf("wdc%d: read error %d sector C=%d,H=%d,S=%d\n", d, errno, c, h, s));
        wdc.status |= WDC_STAT_ERROR;
        break;
      }
      dma_transfer(0, wdc.buffer, size);
    } else {
      // Sector not found.
      wdc.status |= WDC_STAT_ERROR;
      break;
    }

    // Move to next sector.
    s += 1;
    n--;
  }
  dma_transfer_done(0);
}

void wdc_write() {
  FILE *drive;
  WORD adr;
  WORD cnt;

  // Get parameters.  
  int d = (wdc.reg[WDC_REG_SDH] >> 3) & 0x03;
  int c = wdc.reg[WDC_REG_CYLLO]  + (wdc.reg[WDC_REG_CYLHI] << 8);
  int h = wdc.reg[WDC_REG_SDH] & 0x07;
  int s = wdc.reg[WDC_REG_SECTNO];
  int n = wdc.reg[WDC_REG_SECTCNT];
  int size = sector_size[(wdc.reg[WDC_REG_SDH] >> 5) & 0x03];
  L(printf("wdc: write drive=%d c=%d h=%d, s=%d, n=%d, sector size %d\n", d, c, h, s, n, size));

  // Get mounted disk image for drive.
  drive = wdc.drive[d];
  if (!drive) {
    wdc.status |= WDC_STAT_ERROR;
    wdc.reg[WDC_STAT_ERROR] = 1 << 6;
    wdc.status &= ~WDC_STAT_DRIVE_READY;
    return;
  }

  // Write sectors.
  if (!n) n = 32;
  while (n > 0 && !dma_completed(0)) {
    if (c < CYLS && h < HEADS && s < SECTORS) {
      // Fetch data from DMA channel.
      cnt = size;
      adr = dma_fetch(0, &cnt);

      LL(printf("wdc%d: write sector C=%d,H=%d,S=%d: write %d bytes from %04X\n", d, c, h, s, cnt, adr));
    
      // Write data to disk.
      memcpy(wdc.buffer, ram + adr, cnt);
      if (fseek(drive, ((((c * HEADS) + h) * SECTORS + s) * SECTSIZE), SEEK_SET) == -1) {
        W(printf("wdc%d: write seek error %d sector C=%d,H=%d,S=%d\n", d, errno, c, h, s));
        wdc.status |= WDC_STAT_ERROR;
        break;
      }
      if (fwrite(wdc.buffer, 1, SECTSIZE, drive) != SECTSIZE) {
        W(printf("wdc%d: write error %d sector C=%d,H=%d,S=%d\n", d, errno, c, h, s));
        wdc.status |= WDC_STAT_ERROR;
        break;
      }
    } else {
      // Sector not found.
      wdc.status |= WDC_STAT_ERROR;
      break;
    }

    // Move to next sector.
    s += 1;
    n--;
  }
  dma_transfer_done(0);
}

void wdc_command(BYTE data, int dev) {
  int cyl;

  wdc.status &= ~WDC_STAT_ERROR;
  switch (data & 0xF0) {
    case WDC_CMD_RESTORE:
      L(printf("wdc: restore\n"));
      wdc.reg[WDC_REG_ERROR] = 0;
      wdc.reg[WDC_REG_SECTCNT] = 0;
      wdc.reg[WDC_REG_SECTNO] = 0;
      wdc.reg[WDC_REG_CYLLO] = 0;
      wdc.reg[WDC_REG_CYLHI] = 0;
      wdc.reg[WDC_REG_SDH] = 0;
      wdc.status |= WDC_STAT_SEEK_COMPLETE;
      break;

    case WDC_CMD_TEST:
      W(printf("wdc: test command, not implemented\n"));
      break;

    case WDC_CMD_SEEK:
      cyl = wdc.reg[WDC_REG_CYLLO] + (wdc.reg[WDC_REG_CYLHI] << 8);
      L(printf("wdc: seek to cylinder %d, stepping rate %d\n", cyl, data & 0x0F));
      wdc.status |= WDC_STAT_SEEK_COMPLETE;
      interrupt(0x08, 3);
      break;

    case WDC_CMD_READ:
      wdc_read();
      interrupt(0x08, 3);
      break;

    case WDC_CMD_WRITE:
      wdc_write();
      interrupt(0x08, 3);
      break;

    case WDC_CMD_FORMAT:
      W(printf("wdc: format command, not implemented\n"));
      break;

    default:
      W(printf("wdc: unknown command (%02X)\n", data));
  }
}

void init_wdc() {
  int i;

  memset(&wdc, 0, sizeof(struct winchester_disk_controller));

  wdc.drive[0] = fopen("hd.img", "r+b");
  wdc.status |= WDC_STAT_DRIVE_READY | WDC_STAT_SEEK_COMPLETE;

  register_port(0x60, wdc_data_in, wdc_data_out, 0);
  for (i = 0; i < WDC_NUM_REGS; ++i) {
    register_port(0x61 + i, wdc_reg_in, wdc_reg_out, i);
  }
  register_port(0x67, wdc_status, wdc_command, 0);
}

