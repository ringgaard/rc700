/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * Disk image interface
 */

#define MAX_SIDES     2
#define MAX_CYLINDERS 80
#define MAX_SECTORS   30

struct sector {
  BYTE *data;
  BYTE *original;
  BYTE fill;
  int deleted;
  int bad;
};

struct track {
  int num_sectors;
  int sector_size;
  int transfer_rate;
  int mfm;
  struct sector sectors[MAX_SECTORS];
};

struct disk {
  BYTE *data;
  char *label;
  struct track tracks[MAX_CYLINDERS][MAX_SIDES];
};

int load_disk_image(char *imagefile, struct disk *disk);

