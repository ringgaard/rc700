//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// File transfer device
//

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "rc700.h"

#define L(x) x
#define LL(x)

#define FTP_CMD_RESET       0x00
#define FTP_CMD_FILENAME    0x01
#define FTP_CMD_OPEN        0x02
#define FTP_CMD_CREATE      0x03
#define FTP_CMD_CLOSE       0x04

#define FTP_STAT_OK         0x00
#define FTP_STAT_OVERFLOW   0xfe
#define FTP_STAT_INVALID    0xfe
#define FTP_STAT_EOF        0xff

#define FTP_FNAME_SIZE   256

struct ftp {
  BYTE status;
  char filename[FTP_FNAME_SIZE];
  int fnidx;
  int fnmode;
  FILE *file;
};

struct ftp ftp;

void ftp_reset() {
  L(printf("ftp: reset\n"));
  if (ftp.file) fclose(ftp.file);
  memset(&ftp, 0, sizeof(struct ftp));
}

void ftp_filename() {
  L(printf("ftp: filename\n"));
  ftp.fnmode = 1;
  ftp.fnidx = 0;
  memset(ftp.filename, 0, FTP_FNAME_SIZE);
  ftp.status = FTP_STAT_OK;
}

void ftp_open() {
  L(printf("ftp: open %s\n", ftp.filename));
  if (ftp.file) {
    ftp.status = FTP_STAT_INVALID;
    return;
  }
  ftp.fnmode = 0;
  ftp.file = fopen(ftp.filename, "rb");
  if (!ftp.file) {
    perror(ftp.filename);
    ftp.status = errno;
    return;
  }

  ftp.status = 0;
}

void ftp_create() {
  L(printf("ftp: create %s\n", ftp.filename));
  if (ftp.file) {
    ftp.status = FTP_STAT_INVALID;
    return;
  }
  ftp.fnmode = 0;
  ftp.file = fopen(ftp.filename, "wb");
  if (!ftp.file) {
    perror(ftp.filename);
    ftp.status = errno;
    return;
  }

  ftp.status = 0;
}

void ftp_close() {
  L(printf("ftp: close %s\n", ftp.filename));
  if (!ftp.file) {
    ftp.status = FTP_STAT_INVALID;
    return;
  }

  fclose(ftp.file);
  ftp.file = NULL;
  ftp.status = 0;
}

BYTE ftp_status(int dev) {
  BYTE status = ftp.status;
  ftp.status = 0;
  LL(printf("ftp: status %02X\n", status));
  return status;
}

void ftp_command(BYTE data, int dev) {
  LL(printf("ftp: command %x\n", data));
  switch (data) {
    case FTP_CMD_RESET: ftp_reset(); break;
    case FTP_CMD_FILENAME: ftp_filename(); break;
    case FTP_CMD_OPEN: ftp_open(); break;
    case FTP_CMD_CREATE: ftp_create(); break;
    case FTP_CMD_CLOSE: ftp_close(); break;
    default: ftp.status = FTP_STAT_INVALID;
  }
}

BYTE ftp_data_in(int dev) {
  int data;

  LL(printf("ftp: data in\n"));
  if (!ftp.file) {
    ftp.status = FTP_STAT_INVALID;
    return 0xFF;
  }
  data = getc(ftp.file);
  if (data == EOF) {
    ftp.status = FTP_STAT_EOF;
    return 0xFF;
  }

  ftp.status = FTP_STAT_OK;
  return data;
}

void ftp_data_out(BYTE data, int dev) {
  LL(printf("ftp: data out %x\n", data));
  if (ftp.fnmode) {
    if (ftp.fnidx < FTP_FNAME_SIZE - 1) {
      ftp.filename[ftp.fnidx++] = data;
      ftp.status = 0;
    } else {
      ftp.status = FTP_STAT_OVERFLOW;
    } 
  } else if (ftp.file) {
    if (fputc(data, ftp.file) == EOF) {
      ftp.status = errno;
    } else {
      ftp.status = FTP_STAT_OK;
    }
  } else {
    ftp.status = FTP_STAT_INVALID;
  }
}

void init_ftp() {
  memset(&ftp, 0, sizeof(struct ftp));
  register_port(0xe0, ftp_status, ftp_command, 0);
  register_port(0xe1, ftp_data_in, ftp_data_out, 0);
}

