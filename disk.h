//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// Disk image interface
//

#define MAX_SIDES     2
#define MAX_CYLINDERS 80
#define MAX_SECTORS   30

struct sector {
  BYTE *data;
  BYTE fill;
  int deleted;
  int bad;
  int dirty;
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
  int num_tracks;
  int dirty;
  int writeprotect;
  int readonly;
  char filename[256];
  struct track tracks[MAX_CYLINDERS][MAX_SIDES];
};

struct disk *load_disk_image(char *imagefile);
int save_disk_image(struct disk *disk);
void free_disk_image(struct disk *disk);

int write_disk_sector(struct disk *disk, int c, int h, int s, char *data, int size);
int fill_disk_sector(struct disk *disk, int c, int h, int s, BYTE fill);

