/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * uPD765A - Floppy-Disk Controller 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"
#include "simglb.h"
#include "disk.h"

#define L(x)
#define LL(x)
#define W(x) x

/* FDC status register bits */
#define FDC_STAT_FDD0_BUSY 0x01 /* FDD 0 is in seek mode */
#define FDC_STAT_FDD1_BUSY 0x02 /* FDD 1 is in seek mode */
#define FDC_STAT_FDD2_BUSY 0x04 /* FDD 2 is in seek mode */
#define FDC_STAT_FDD3_BUSY 0x08 /* FDD 3 is in seek mode */
#define FDC_STAT_CB        0x10 /* FDC busy, read or write command in progress */
#define FDC_STAT_EXM       0x20 /* Execution mode */
#define FDC_STAT_DIO       0x40 /* Data input/output */
#define FDC_STAT_DFM       0x80 /* Request for master */

/* FDC commands */
#define FDC_CMD_READ_TRACK              2
#define FDC_CMD_SPECIFY                 3
#define FDC_CMD_SENSE_DRIVE_STATUS      4
#define FDC_CMD_WRITE_DATA              5
#define FDC_CMD_READ_DATA               6
#define FDC_CMD_RECALIBRATE             7
#define FDC_CMD_SENSE_INTERRUPT_STATUS  8
#define FDC_CMD_WRITE_DELETED_DATA      9
#define FDC_CMD_READ_ID                 10
#define FDC_CMD_READ_DELETED_DATA       12
#define FDC_CMD_FORMAT_TRACK            13
#define FDC_CMD_SEEK                    15
#define FDC_CMD_SCAN_EQUAL              17
#define FDC_CMD_SCAN_LOW_OR_EQUAL       25
#define FDC_CMD_SCAN_HIGH_OR_EQUAL      29

/* FDC command registers */

/* FDC result registers */
#define FDC_RES_ST0 0
#define FDC_RES_ST1 1
#define FDC_RES_ST2 2

/* FDC status register 0 */

#define FDC_ST0_NR    0x08       /* Not ready */
#define FDC_ST0_EC    0x10       /* Equipment check */
#define FDC_ST0_SE    0x20       /* Seek end */

#define FDC_ST0_NT    0x00       /* Interrupt code, normal termination */
#define FDC_ST0_AT    0x40       /* Interrupt code, abnormal termination */
#define FDC_ST0_IC    0x80       /* Interrupt code, invalid command */
#define FDC_ST0_MASK  0xE0       /* Interrupt code mask */

/* FDC status register 1 */

#define FDC_ST1_MA    0x01       /* Missing address mark */
#define FDC_ST1_NW    0x02       /* Not writable */
#define FDC_ST1_ND    0x04       /* No data */
#define FDC_ST1_OR    0x10       /* Overrun */
#define FDC_ST1_DE    0x20       /* Data error */
#define FDC_ST1_EN    0x80       /* End of cylinder */

/* FDC status register 3 */

#define FDC_ST3_TS    0x08       /* Two-sided */
#define FDC_ST3_T0    0x10       /* Track 0 */
#define FDC_ST3_RY    0x20       /* Ready */
#define FDC_ST3_WP    0x40       /* Write protected */
#define FDC_ST3_FT    0x80       /* Fault */

static int fdc_parameter_count[] = {
  1, 1,  /*  0 */
  1, 1,  /*  1 */
  9, 7,  /*  2 */
  3, 0,  /*  3 */
  2, 1,  /*  4 */
  9, 7,  /*  5 */
  9, 7,  /*  6 */
  2, 0,  /*  7 */
  1, 2,  /*  8 */
  9, 7,  /*  9 */
  2, 7,  /* 10 */
  1, 1,  /* 11 */
  9, 7,  /* 12 */
  6, 7,  /* 13 */
  1, 1,  /* 14 */
  3, 0,  /* 15 */
  1, 1,  /* 16 */
  9, 7,  /* 17 */
  1, 1,  /* 18 */
  1, 1,  /* 19 */
  1, 1,  /* 20 */
  1, 1,  /* 21 */
  1, 1,  /* 22 */
  1, 1,  /* 23 */
  1, 1,  /* 24 */
  9, 7,  /* 25 */
  1, 1,  /* 26 */
  1, 1,  /* 27 */
  1, 1,  /* 28 */
  9, 7,  /* 29 */
  1, 1,  /* 30 */
  1, 1,  /* 31 */
};

#define MAX_FDC_DRIVES         4
#define MAX_FDC_COMMAND_BYTES  9
#define MAX_FDC_RESULT_BYTES   7

struct floppy_disk_controller {
  BYTE status;
  BYTE st0;
  BYTE st1;
  BYTE st2;
  BYTE st3;
  BYTE cylinder[MAX_FDC_DRIVES];
  struct disk *disk[MAX_FDC_DRIVES];

  BYTE command[MAX_FDC_COMMAND_BYTES];
  BYTE result[MAX_FDC_RESULT_BYTES];
  int command_index;
  int command_size;
  int result_index;
  int result_size;
};

struct floppy_disk_controller fdc;

void fdc_update_status(int drive, int head, int intr) {
  if (intr) fdc.st0 = (fdc.st0 & ~0x07) | (head << 2) | drive;
  fdc.st3 = (fdc.st3 & ~0x07) | (head << 2) | drive;
  if (fdc.cylinder[drive] == 0) {
    fdc.st3 |= FDC_ST3_T0;
  } else {
    fdc.st3 &= ~FDC_ST3_T0;
  }
  if (fdc.disk[drive]) {
    fdc.st0 &= ~FDC_ST0_NR;
    fdc.st3 |= FDC_ST3_RY;
  } else {
    fdc.st0 |= FDC_ST0_NR;
    fdc.st3 &= ~FDC_ST3_RY;
  }
}

void fdc_update_transfer_result(int drive, int cylinder, int head, int sector, int count) {
  fdc_update_status(drive, head, 1);
  fdc.result[0] = fdc.st0;
  fdc.result[1] = fdc.st1;
  fdc.result[2] = fdc.st2;
  fdc.result[3] = cylinder;
  fdc.result[4] = head;
  fdc.result[5] = sector;
  fdc.result[6] = count;
}

void fdc_check_encoding(int drive, int head, int mfm) {
  struct disk *disk;
  struct track *track;
  
  disk = fdc.disk[drive];
  if (disk == NULL) {
    printf("drive %d is not mounted\n", drive);
    return;
  }
  
  track = &disk->tracks[fdc.cylinder[drive]][head];
  if (mfm != track->mfm) {
    fdc.st0 |= FDC_ST0_AT;
    fdc.st1 |= FDC_ST1_MA | FDC_ST1_ND;
  }
}

void fdc_read_sectors(int drive) {
  int sector_size;

  // Get command parameters.
  int MT = fdc.command[0] & 0x80;
  int MF = fdc.command[0] & 0x40;
  int SK = fdc.command[0] & 0x20;
  int C = fdc.command[2];
  int H = fdc.command[3];
  int R = fdc.command[4];
  int N = fdc.command[5];
  int EOT = fdc.command[6];
  int GPL = fdc.command[7];
  int DTL = fdc.command[8];

  // Get mounted disk image for drive.
  struct disk *disk = fdc.disk[drive];
  if (!disk) {
    W(printf("fdc: drive %d is not mounted\n", drive));
    fdc_update_transfer_result(drive, C, H, R, N);
    return;
  }

  L(printf("read: MT=%d MF=%d SK=%d C=%d H=%d R=%d N=%d EOT=%d GPL=%d DTL=%d\n", MT, MF, SK, C, H, R, N, EOT, GPL, DTL));

  // Compute sector size.
  if (N > 0) {
    sector_size = 128 << N;
  } else {
    sector_size = DTL;
    if (MF) sector_size *= 2;
  }

  // Read sectors until we reach terminal count for DMA.
  dma_transfer_start(1);
  while (!dma_completed(1)) {
    struct track *track = &disk->tracks[C][H];
    if (R <= track->num_sectors) {
      struct sector *sector = &track->sectors[R - 1];

      // Transfer sector using DMA.
      int size = track->sector_size;
      if (size > sector_size) size = sector_size;

      L(printf("fdc: read sector C=%d,H=%d,S=%d: read %d bytes to %04X, %d kbps, %s\n", C, H, R - 1, size, dma_address(1), track->transfer_rate, track->mfm ? "MFM" : "FM"));
      if (sector->bad) W(printf("fdc: read bad sector C=%d,H=%d,S=%d on drive %d\n", C, H, R - 1, drive));

      if (sector->data) {
        dma_transfer(1, sector->data, size);
      } else {
        dma_fill(1, sector->fill, size);
      }
    } else {
      // Sector not found.
      fdc.st1 |= FDC_ST1_ND;
      break;
    }

    // Move to next sector.
    R += 1;
    if (R > EOT) {
      R = 1;
      if (!MT) break;
      H = 1 - H;
    }
  }
  dma_transfer_done(1);
  fdc_update_transfer_result(drive, C, H, R, N);
  L(printf("read done\n"));
}

void fdc_write_sectors(int drive) {
  WORD adr;
  WORD cnt;
  int sector_size;

  // Get command parameters.
  int MT = fdc.command[0] & 0x80;
  int MF = fdc.command[0] & 0x40;
  int SK = fdc.command[0] & 0x20;
  int C = fdc.command[2];
  int H = fdc.command[3];
  int R = fdc.command[4];
  int N = fdc.command[5];
  int EOT = fdc.command[6];
  int GPL = fdc.command[7];
  int DTL = fdc.command[8];

  // Get mounted disk image for drive.
  struct disk *disk = fdc.disk[drive];
  if (!disk) {
    W(printf("fdc: drive %d is not mounted\n", drive));
    fdc_update_transfer_result(drive, C, H, R, N);
    return;
  }

  L(printf("write: MT=%d MF=%d SK=%d C=%d H=%d R=%d N=%d EOT=%d GPL=%d DTL=%d\n", MT, MF, SK, C, H, R, N, EOT, GPL, DTL));

  // Compute sector size.
  if (N > 0) {
    sector_size = 128 << N;
  } else {
    sector_size = DTL;
    if (MF) sector_size *= 2;
  }

  // Write sectors.
  while (!dma_completed(1)) {
    struct track *track = &disk->tracks[C][H];

    if (R <= track->num_sectors) {
      struct sector *sector = &track->sectors[R - 1];

      int size = track->sector_size;
      if (size > sector_size) size = sector_size;
      
      // Fetch data from DMA channel.
      cnt = size;
      adr = dma_fetch(1, &cnt);

      L(printf("fdc: write sector C=%d,H=%d,S=%d: write %d bytes to %04X, %d kbps, %s\n", C, H, R - 1, cnt, adr, track->transfer_rate, track->mfm ? "MFM" : "FM"));
      
      if (cnt != track->sector_size) {
        W(printf("fdc: partial sector C=%d,H=%d,S=%d on drive %d, %d bytes, %d expected\n", C, H, R - 1, drive, cnt, track->sector_size));
      }

      // Write data to disk.
      write_disk_sector(disk, C, H, R - 1, ram + adr, cnt);      
    } else {
      // Sector not found.
      fdc.st1 |= FDC_ST1_ND;
      break;
    }

    // Move to next sector.
    R += 1;
    if (R > EOT) {
      R = 1;
      if (!MT) break;
      H = 1 - H;
    }
  }
  dma_transfer_done(1);
  fdc_update_transfer_result(drive, C, H, R, N);
  L(printf("write done\n"));
}

void fdc_execute_command(void) {
  int cmd, head, drive, srt, hut, intr, prev_st0;

  cmd = fdc.command[0] & 0x1f;
  head = (fdc.command[1] >> 2) & 1;
  drive = fdc.command[1] & 0x03;
 
  intr = 0;
  prev_st0 = fdc.st0;
  fdc.st0 &= ~FDC_ST0_MASK;
  fdc.st1 = 0;
  switch (cmd) {
    case FDC_CMD_READ_TRACK:
      printf("fdc: read track\n");
      break;

    case FDC_CMD_SPECIFY:
      L(printf("fdc: specify: srt=%d ms, hut=%d ms, hlt=%d ms, non_dma=%d\n", 
             16 - (fdc.command[1] >> 4), 
             (fdc.command[1] & 0x0f) * 16, 
             (fdc.command[2] >> 1) * 2,
             fdc.command[2] & 1));
      break;

    case FDC_CMD_SENSE_DRIVE_STATUS:
      L(printf("fdc: sense drive status, drive=%d, head=%d\n", drive, head));
      fdc_update_status(drive, head, 0);
      fdc.result[0] = fdc.st3;
      break;

    case FDC_CMD_WRITE_DATA:
      fdc_write_sectors(drive);
      intr = 1;
      break;

    case FDC_CMD_READ_DATA:
      fdc_read_sectors(drive);
      intr = 1;
      break;

    case FDC_CMD_RECALIBRATE:
      L(printf("fdc: recalibrate, drive=%d\n", drive));
      fdc.cylinder[drive] = 0;
      fdc.st0 |= FDC_ST0_SE;
      fdc_update_status(drive, head, 1);
      intr = 1;
      break;

    case FDC_CMD_SENSE_INTERRUPT_STATUS:
      L(printf("fdc: sense interrupt status, st0=%02x cyl=%d\n", prev_st0, fdc.cylinder[prev_st0 & 0x03]));
      fdc.result[0] = prev_st0;
      fdc.result[1] = fdc.cylinder[prev_st0 & 0x03];
      break;

    case FDC_CMD_WRITE_DELETED_DATA:
      W(printf("fdc: write deleted data, not implemented\n"));
      break;

    case FDC_CMD_READ_ID:
      L(printf("fdc: read id, drive=%d, head=%d, mf=%02x\n", drive, head, fdc.command[0] & 0x40));
      fdc_check_encoding(drive, head, (fdc.command[0] & 0x40) != 0);
      fdc_update_transfer_result(drive, fdc.cylinder[drive], head, 0, 0);
      intr = 1;
      break;

    case FDC_CMD_READ_DELETED_DATA:
      W(printf("fdc: read deleted data, not implemented\n"));
      break;

    case FDC_CMD_FORMAT_TRACK:
      W(printf("fdc: format track, not implemented\n"));
      break;

    case FDC_CMD_SEEK:
      L(printf("fdc: seek, drive=%d cyl=%d\n", drive, fdc.command[2]));
      fdc.cylinder[drive] = fdc.command[2];
      fdc.st0 |= FDC_ST0_SE;
      fdc_update_status(drive, 0 /* head */, 1);
      intr = 1;
      break;

    case FDC_CMD_SCAN_EQUAL:
      W(printf("fdc: scan equal, not implemented\n"));
      break;

    case FDC_CMD_SCAN_LOW_OR_EQUAL:
      W(printf("fdc: scan low or equal, not implemented\n"));
      break;

    case FDC_CMD_SCAN_HIGH_OR_EQUAL:
      W(printf("fdc: scan high or equal, not implemented\n"));
      break;

    default:
      W(printf("fdc: invalid command %02X\n", cmd));
      fdc.result[FDC_RES_ST0] = 0x80;
  }

  if (fdc.result_index < fdc.result_size) {
    fdc.status |= FDC_STAT_DIO;
  } else {
    fdc.status &= ~(FDC_STAT_DIO | FDC_STAT_CB);
  }

  if (intr) {
    // In RC702, the FDC INT pin is connected to the CLK/TRG3 on the CTC
    ctc_trigger(CTC_CHANNEL_FDC);
  }
}

BYTE fdc_status(int dev) {
  LL(printf("fdc: read status %02X\n", fdc.status));
  return fdc.status; 
}

BYTE fdc_data_in(int dev) {
  BYTE result;

  if (fdc.result_index < fdc.result_size) {
    result = fdc.result[fdc.result_index++];
  } else {
    W(printf("fdc: status read beyond limit\n"));
    result = 0;
  }
  if (fdc.result_index == fdc.result_size) {
    fdc.status &= ~(FDC_STAT_DIO | FDC_STAT_CB);
  }
  LL(printf("fdc: result read %02X\n", result));
  return result;
}

void fdc_data_out(BYTE data, int dev) {
  int cmd;

  LL(printf("fdc: command %02X\n", data));
  if (fdc.status & FDC_STAT_CB) {
    /* Add command byte */
    if (fdc.command_index < MAX_FDC_COMMAND_BYTES) {
      fdc.command[fdc.command_index++] = data;
    }
  } else {
    /* Start new command */
    fdc.command[0] = data;
    cmd = data & 0x1f;
    fdc.command_index = 1;
    fdc.result_index = 0;
    fdc.command_size = fdc_parameter_count[cmd * 2];
    fdc.result_size = fdc_parameter_count[cmd * 2 + 1];
    if (fdc.command_index < fdc.command_size) {
      fdc.status |= FDC_STAT_CB;
      fdc.status &= ~FDC_STAT_DIO;
    }
  }

  if (fdc.command_index == fdc.command_size) {
    fdc_execute_command();
  }
}

void fdc_floppy_motor(BYTE data, int dev) {
  L(printf("fdc: floppy motor %02X\n", data));
}

int fdc_mount_disk(int drive, char *imagefile) {
  struct disk *disk;

  L(printf("fdc: mount %s on drive %d\n", imagefile, drive));
  disk = load_disk_image(imagefile);
  if (disk) {
    fdc.disk[drive] = disk;
  } else {
    W(printf("unable to load disk image in %s\n", imagefile));
  }
}

void fdc_flush_disk(int drive, char *imagefile) {
  if (fdc.disk[drive] && fdc.disk[drive]->dirty) {
    W(printf("fdc: save drive %d to %s\n", drive, imagefile));
    save_disk_image(fdc.disk[drive], imagefile);
  }
}

void init_fdc(void) {
  memset(&fdc, 0, sizeof(struct floppy_disk_controller));
  fdc.status = FDC_STAT_DFM;
  fdc.st0 = FDC_ST0_SE;
  fdc.st3 = FDC_ST3_T0 | FDC_ST3_TS | FDC_ST3_RY;
  register_port(0x04, fdc_status, NULL, 0);
  register_port(0x05, fdc_data_in, fdc_data_out, 0);
}

