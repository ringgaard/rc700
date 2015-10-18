//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// Disk image interface
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cpu.h"
#include "disk.h"

//#define DEBUG_DISK

struct imd_track_header {
  BYTE mode;
  BYTE cylinder;
  BYTE head;
  BYTE sectors;
  BYTE sector_size;
};

struct disk *load_disk_image(char *imagefile) {
  struct disk *disk;
  struct track *track;
  struct sector *sector;
  FILE *f;
  int size;
  BYTE *p;
  BYTE *end;
  int i;
  int bad_sectors = 0;

  // Initialize disk structure.
  disk = malloc(sizeof(struct disk));
  if (!disk) return NULL;
  memset(disk, 0, sizeof(struct disk));
  strcpy(disk->filename, imagefile);

  // Read image into memory.
  f = fopen(imagefile, "rb");
  if (!f) {
    perror(imagefile);
    free(disk);
    return NULL;
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
    free(disk);
    return NULL;
  }
  fclose(f);

  // Check that it is in IMD format.
  if (size < 3 || memcmp(disk->data, "IMD", 3) != 0) {
    fprintf(stderr, "unknown image format\n");
    free(disk);
    return NULL;
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
      free(disk);
      return NULL;
    }

    if (hdr->sectors >= MAX_SECTORS) {
      fprintf(stderr, "Too many sectors: sect=%d", hdr->sectors);
      free(disk);
      return NULL;
    }

    if (hdr->head & ~1) {
      fprintf(stderr, "Cylinder maps not supported\n");
      free(disk);
      return NULL;
    }

    track = &disk->tracks[hdr->cylinder][hdr->head];
    track->present = 1;
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
    if (hdr->cylinder >= disk->num_tracks) disk->num_tracks = hdr->cylinder + 1;

#ifdef DEBUG_DISK
    printf("track %d side %d: mfm %d xferrate %d sectors %d size %d\n", 
           hdr->cylinder, hdr->head,
           track->mfm, track->transfer_rate, track->num_sectors, track->sector_size);
#endif

    // Skip physical-to-logical sector map.
    p += track->num_sectors;

    // Setup sectors for track.
    for (i = 0; i < track->num_sectors; ++i) {
      if (p >= end) {
        fprintf(stderr, "corrupted disk image\n");
        free(disk);
        return NULL;
      }
      sector = &track->sectors[i];
      sector->present = 1;
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

  if (bad_sectors > 0) fprintf(stderr, "warning: %s has %d bad sectors\n", imagefile, bad_sectors);
  return disk;
}

int save_disk_image(struct disk *disk) {
  FILE *f;
  struct tm *tm;
  time_t now;
  int c, h, s;
  struct track *track;
  struct sector *sector;
  struct imd_track_header hdr;
  int ss;
  int type;

#ifdef DEBUG_DISK
  printf("write disk image to %s\n", imagefile);
#endif

  // Check for read-only image.
  if (disk->readonly) return -1;

  // Create output file.
  f = fopen(disk->filename, "wb");
  if (!f) {
    perror(disk->filename);
    return -1;
  }
  
  // Write header.
  time(&now);
  tm = localtime(&now);
  fprintf(f, "IMD 1.18: %2d/%2d/%4d %02d:%02d:%02d\r\n%s\032",
          tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
          tm->tm_hour, tm->tm_min, tm->tm_sec, disk->label);

  // Write tracks.  
  for (c = 0; c < disk->num_tracks; ++c) {
    for (h = 0; h < MAX_SIDES; ++h) {
      // Write track header.
      track = &disk->tracks[c][h];
      hdr.cylinder = c;
      hdr.head = h;
      hdr.sectors = track->num_sectors;
      hdr.sector_size = 0;
      ss = track->sector_size / 128;
      while (ss > 1) {
        ss /= 2;
        hdr.sector_size++;
      }
      hdr.mode = track->mfm ? 3 : 0;
      if (track->transfer_rate == 300) hdr.mode |= 1;
      if (track->transfer_rate == 250) hdr.mode |= 2;
      fwrite(&hdr, 1, sizeof(hdr), f);

      // Write sector map.
      for (s = 0; s < track->num_sectors; ++s) putc(s, f);
      
      // Write sectors.
      for (s = 0; s < track->num_sectors; ++s) {
        sector = &track->sectors[s];
        type = 0;
        if (sector->bad) {
          type = 5;
        } else if (sector->deleted) {
          type = 7;
        } else {
          type = 1;
        }
        if (!sector->data) type++;
        putc(type, f);

        if (sector->data) {
          fwrite(sector->data, 1, track->sector_size, f);
        } else {
          putc(sector->fill, f);
        }
      }
    }
  }

  disk->dirty = 0;
  fclose(f);
  return 0;
}

int write_disk_sector(struct disk *disk, int c, int h, int s, BYTE *data, int size) {
  struct track *track = &disk->tracks[c][h];
  struct sector *sector = &track->sectors[s];

#ifdef DEBUG_DISK
  printf("write disk sector c=%d,h=%d,s=%d, %d bytes\n", c, h, s, size);
#endif

  if (!sector->dirty || !sector->data) {
    sector->data = malloc(track->sector_size);
    if (!sector->data) return -1;
  }
  if (size > track->sector_size) size = track->sector_size;
  memcpy(sector->data, data, size);
  sector->present = 1;
  sector->dirty = 1;
  sector->bad = 0;
  sector->deleted = 0;
  disk->dirty = 1;
  return size;
}

int fill_disk_sector(struct disk *disk, int c, int h, int s, BYTE fill) {
  struct track *track = &disk->tracks[c][h];
  struct sector *sector = &track->sectors[s];

#ifdef DEBUG_DISK
  printf("fill disk sector c=%d,h=%d,s=%d, fill=%02x\n", c, h, s, fill);
#endif

  if (sector->dirty && sector->data) free(sector->data);
  sector->data = NULL;
  sector->fill = fill;
  sector->present = 1;
  sector->dirty = 1;
  sector->bad = 0;
  sector->deleted = 0;
  disk->dirty = 1;
  return track->sector_size;
}

void free_disk_image(struct disk *disk) {
  int c, h, s;
  struct track *track;
  struct sector *sector;
  
  for (c = 0; c < disk->num_tracks; ++c) {
    for (h = 0; h < MAX_SIDES; ++h) {
      track = &disk->tracks[c][h];
      for (s = 0; s < track->num_sectors; ++s) {
        sector = &track->sectors[s];
        if (sector->dirty && sector->data) free(sector->data);
      }
    }
  }
  free(disk->data);
  free(disk);
}

