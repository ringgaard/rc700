#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sim.h"
#include "disk.h"

#define DEBUG_CPMDISK
//#define COMAL

struct disk disk;

int read_sector(int c, int h, int s, BYTE *buffer) {
  struct track *track;
  struct sector *sector;
  
  if (c < 0 || c >= MAX_CYLINDERS || h < 0 || h >= MAX_SIDES) {
    fprintf(stderr, "Illegal disk track (c,h,s)=(%d,%d,%d)\n", c, h, s);
    return -1;
  }
  
  track = &disk.tracks[c][h];
  if (s < 0 || s >= track->num_sectors) {
    fprintf(stderr, "Illegal disk sector (c,h,s)=(%d,%d,%d)\n", c, h, s);
    return -1;
  }
  sector = &track->sectors[s];
  if (sector->bad) {
    fprintf(stderr, "Bad sector (c,h,s)=(%d,%d,%d)\n", c, h, s);
    return -1;
  }
  if (sector->deleted) {
    fprintf(stderr, "Deleted sector (c,h,s)=(%d,%d,%d)\n", c, h, s);
    return -1;
  }
  
  if (sector->data) {
    memcpy(buffer, sector->data, track->sector_size);
  } else {
    memset(buffer, sector->fill, track->sector_size);
  }
  return track->sector_size;
}

struct extent {
  BYTE user;
  BYTE name[8];
  BYTE type[3];
  BYTE xl;
  BYTE bc;
  BYTE xh;
  BYTE rc;
  BYTE al[16];
};

#define BLOCKSIZE         2048
#define SECTORSIZE        512
#define SECTS_PER_BLOCK   (BLOCKSIZE / SECTORSIZE)
#ifdef COMAL
#define RESERVED_TRACKS   6
#else
#define RESERVED_TRACKS   4
#endif
#define SECTS_PER_TRACK   9
#define DISK_SIDES        2

#define DIR_BLOCKS        2
#define DIR_ENTRIES       ((BLOCKSIZE * DIR_BLOCKS) / sizeof(struct extent))

BYTE directory[DIR_BLOCKS * BLOCKSIZE];

static int interleave[SECTS_PER_TRACK] = {0,2,4,6,8,1,3,5,7};

int read_block(int blkno, BYTE *buffer) {
  int sectno = blkno * SECTS_PER_BLOCK;
  int i, l;

  for (i = 0; i < SECTS_PER_BLOCK; ++i) {
    int t = sectno / SECTS_PER_TRACK + RESERVED_TRACKS;
    int c = t / DISK_SIDES;
    int h = t % DISK_SIDES;
    int s = interleave[sectno % SECTS_PER_TRACK];
#ifdef DEBUG_CPMDISK
    printf("(sn=%d,t=%d,c=%d,h=%d,s=%d)\n", sectno, t, c, h, s + 1);
#endif
    l = read_sector(c, h, s, buffer);
    if (l != SECTORSIZE) {
      fprintf(stderr, "Wrong sector size %d, block %d, (c,h,s)=(%d,%d,%d)\n", l, blkno, c, h, s);
      return -1;
    }
    sectno++;
    buffer += SECTORSIZE;
  }
  return BLOCKSIZE;
}

void extract_extent(struct extent *d, int size, FILE *output) {
  int i, n;
  char block[BLOCKSIZE];
  
  for (i = 0; i < 16; ++i) {
    if (d->al[i] == 0) continue;
#ifdef DEBUG_CPMDISK
    printf(" read block %d, %d bytes left\n", d->al[i], size);
#endif
    n = read_block(d->al[i], block);
    if (n > size) n = size;
    fwrite(block, 1, n, output);
    size -= n;
  }
}

void extract_filename(struct extent *d, char *fn) {
  int i;

  for (i = 0; i < 8; ++i) {
    int ch = d->name[i];
    if (ch != ' ') *fn++ = ch;
  }
  if (memcmp(d->type, "   ", 3) != 0) {
#ifndef COMAL
    *fn++ = '.';
#endif
    for (i = 0; i < 3; ++i) {
      int ch = d->type[i];
      if (ch != ' ') *fn++ = ch & 0x7F;
    }
  }
  *fn = 0;
}

void extract_file(char *fn, FILE *output) {
  struct extent *dir = (struct extent *) directory;
  int i;

  for (i = 0; i < DIR_ENTRIES; ++i) {
    struct extent *d = &dir[i];
    char name[13];
    int size;

    if (d->user == 0xE5) continue;
    extract_filename(d, name);
    if (strcmp(fn, name) != 0) continue;

    size = (d->rc * 128) + d->bc + (d->xl & 1) * BLOCKSIZE * 8;
    extract_extent(d, size, output);
  }
}

int main(int argc, char *argv[]) {
  struct extent *dir = (struct extent *) directory;
  char *imdfile = NULL;
  char *dest = NULL;
  BYTE *p;
  int i, j;

  // Parse command line arguments.
  if (argc != 2 && argc != 3) {
    fprintf(stderr, "usage: %s IMGDISK [DETINATION]\n", argv[0]);
    return 1;
  }
  imdfile = argv[1];
  if (argc >= 3) dest = argv[2];

  // Read image into memory.
  load_disk_image(imdfile, &disk);
  
  // Read directory.
  p = directory;
  for (i = 0; i < DIR_BLOCKS; ++i) {
    read_block(i, p);
    p += BLOCKSIZE;
  }
  
  printf("Files:\n");
  for (i = 0; i < DIR_ENTRIES; ++i) {
    struct extent *d = &dir[i];
    char name[13];
    char *s = name;
    int size, als, extno;

    if (d->user == 0xE5) continue;
    extract_filename(d, name);

    size = (d->rc * 128) + d->bc + (d->xl & 1) * BLOCKSIZE * 8;
    extno = (d->xl >> 1) + d->xh * 32;

    printf(" %5d bytes %-15s ", size, name);
    if (d->type[0] & 0x80) printf(" (RO)");
    if (d->type[1] & 0x80) printf(" (SY)");
    if (d->type[2] & 0x80) printf(" (AR)");
    if (extno > 0) printf(" extent %d", extno);

#ifdef DEBUG_CPMDISK
    printf(" XL=%02X BC=%02X XH=%02X RC=%02X", d->xl, d->bc, d->xh, d->rc);
    printf(" extent %3d", extno);
    printf(" AL:");
    als = 0;
    for (j = 0; j < 16; ++j) {
      if (d->al[j] != 0) {
        printf(" %02X", d->al[j]);
        als++;
      }
    }
    printf(" (%d blks, %d bytes)", als, als * BLOCKSIZE);
#endif

    printf("\n");

    if (dest != NULL && extno == 0) {
      char destfn[256];
      FILE *f;
      strcpy(destfn, dest);
      strcat(destfn, "/");
      strcat(destfn, name);
      f = fopen(destfn, "wb");
      if (!f) {
        perror(destfn);
        return 0;
      }
      extract_file(name, f);
      fclose(f);
    }
  }

  return 0;
}

