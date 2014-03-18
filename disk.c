/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * Disk image interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"
#include "disk.h"

//#define DEBUG_DISK

struct imd_track_header {
  BYTE mode;
  BYTE cylinder;
  BYTE head;
  BYTE sectors;
  BYTE sector_size;
};

int load_disk_image(char *imagefile, struct disk *disk) {
  FILE *f;
  int size;
  BYTE *p;
  BYTE *end;
  int i;
  int bad_sectors = 0;

  // Initialize disk structure.
  memset(disk, 0, sizeof(struct disk));

  // Read image into memory.
  f = fopen(imagefile, "rb");
  if (!f) {
    perror(imagefile);
    return 0;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
#ifdef DEBUG_DISK
  printf("image in %s is %d bytes\n", imagefile, size);
#endif

  disk->data = malloc(size);
  if (fread(disk->data, 1, size, f) != size) {
    perror(imagefile);
    fclose(f);
    return 0;
  }
  fclose(f);

  // Check that it is in IMD format.
  if (size < 3 || memcmp(disk->data, "IMD", 3) != 0) {
    fprintf(stderr, "unknown image format\n");
    return 0;
  }

  // Find end of header.
  disk->label = NULL;
  p = disk->data;
  end = p + size;
  while (p < end && *p != 0x1A) {
    if (*p == '\n' && !disk->label) disk->label = (char *)(p + 1);
    p++;
  }
  if (p < end && *p == 0x1A) *p++ = 0;
#ifdef DEBUG_DISK
  printf("label:\n%s\n", disk->label);
#endif

  // Read tracks.
  while (p < end) {
    struct imd_track_header *hdr = (struct imd_track_header *) p;
    p += sizeof(struct imd_track_header);

#ifdef DEBUG_DISK
    printf("track: mode %d cyl %d head %d sectors %d sectsize %d\n",
           hdr->mode, hdr->cylinder, hdr->head, hdr->sectors, hdr->sector_size);
#endif

    if (hdr->cylinder >= MAX_CYLINDERS) {
      fprintf(stderr, "Too many tracks: cyl=%d", hdr->cylinder);
      return 0;
    }

    if (hdr->sectors >= MAX_SECTORS) {
      fprintf(stderr, "Too many sectors: sect=%d", hdr->sectors);
      return 0;
    }

    if (hdr->head & ~1) {
      fprintf(stderr, "Cylinder maps not supported\n");
      return 0;
    }

    struct track *track = &disk->tracks[hdr->cylinder][hdr->head];
    switch (hdr->mode) {
      case 0: track->transfer_rate = 500; track->mfm = 0; break;
      case 1: track->transfer_rate = 300; track->mfm = 0; break;
      case 2: track->transfer_rate = 250; track->mfm = 0; break;
      case 3: track->transfer_rate = 500; track->mfm = 1; break;
      case 4: track->transfer_rate = 300; track->mfm = 1; break;
      case 5: track->transfer_rate = 250; track->mfm = 1; break;
    }
    track->num_sectors = hdr->sectors;
    track->sector_size = (1 << hdr->sector_size) * 128;

#ifdef DEBUG_DISK
    printf("track %d side %d: encoding %d xferrate %d sectors %d size %d\n", 
           hdr->cylinder, hdr->head,
           track->encoding, track->transfer_rate, track->num_sectors, track->sector_size);
#endif

    // Skip physical-to-logical sector map.
    p += track->num_sectors;

    // Setup sectors for track.
    for (i = 0; i < track->num_sectors; ++i) {
      if (p >= end) {
        fprintf(stderr, "corrupted disk image\n");
        return 0;
      }
      struct sector *sector = &track->sectors[i];
      switch (*p++) {
        // 00      Sector data unavailable - could not be read
        case 0: sector->bad = 1; break;
        // 01 .... Normal data: (Sector Size) bytes follow
        case 1: sector->data = p; p += track->sector_size; break;
        // 02 xx   Compressed: All bytes in sector have same value (xx)
        case 2: sector->fill = *p++; break;
        // 03 .... Normal data with "Deleted-Data address mark"
        case 3: sector->data = p; p += track->sector_size; sector->deleted = 1; break;
        // 04 xx   Compressed  with "Deleted-Data address mark"
        case 4: sector->fill = *p++; sector->deleted = 1; break;
        // 05 .... Normal data read with data error
        case 5: sector->data = p; p += track->sector_size; sector->bad = 1; break;
        // 06 xx   Compressed  read with data error
        case 6: sector->fill = *p++; sector->bad = 1; break;
        // 07 .... Deleted data read with data error
        case 7: sector->data = p; p += track->sector_size; sector->bad = 1; sector->deleted = 1; break;
        // 08 xx   Compressed, Deleted read with data error
        case 8: sector->fill = *p++; sector->bad = 1; sector->deleted = 1; break;
      }
      if (sector->bad) bad_sectors++;

#ifdef DEBUG_DISK
      if (sector->data) {
        printf("  sector %d: deleted=%d bad=%d data=%d\n", i, sector->deleted, sector->bad, sector->data - disk->data);
      } else {
        printf("  sector %d: deleted=%d bad=%d fill=%02x\n", i, sector->deleted, sector->bad, sector->fill);
      }
#endif
    }
  }

  if (bad_sectors > 0) printf("warning: %s has %d bad sectors\n", imagefile, bad_sectors);
  return 1;
}

