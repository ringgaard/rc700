// Convert raw disk images to IMD format for RC700 emulator.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "disk.h"

#define RC_FORMAT   0
#define CPM_FORMAT  1
#define IBM_FORMAT  1

struct side {
  char *filename;
  char *data;
  int size;
};

// Read content of file.
int get_file_data(char *filename, char **data) {
  FILE *f;
  int size;
  char *image;

  f = fopen(filename, "rb");
  if (!f) {
    perror(filename);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);

  image = malloc(size);
  if (fread(image, 1, size, f) != size) {
    perror(filename);
    fclose(f);
    free(image);
    return -1;
  }
  fclose(f);
  *data = image;
  return size;
}

int main(int argc, char *argv[]) {
  int format;
  char *imdfn;
  struct side side[2];
  struct side t0side[2];
  struct disk *disk;
  int c, h, s;
  int cyls, sectsize, sect_per_track, mfm, sides;
  int tracks;
  BYTE *p;

  // Get arguments.
  if (argc < 5) {
    fprintf(stderr, "usage: %s [cpm|rc|ibm] <imd> <side 0> <side 1>\n",
            argv[0]);
    return 1;
  }
  if (strcmp(argv[1], "rc") == 0) {
    // 16 sectors per track, 128 bytes per sector, two sides, 35 tracks, FM.
    format = RC_FORMAT;
    cyls = 35;
    sectsize = 128;
    sect_per_track = 16;
    sides = 2;
    mfm = 0;
  } else if (strcmp(argv[1], "cpm") == 0) {
    // 9 sectors per track, 512 bytes per sector, two sides, 35 tracks, MFM.
    format = CPM_FORMAT;
    cyls = 35;
    sectsize = 512;
    sect_per_track = 9;
    sides = 2;
    mfm = 1;
  } else if (strcmp(argv[1], "ibm") == 0) {
    // 26 sectors, 128 bytes per sector, single side, 77 tracks, single-density.
    format = IBM_FORMAT;
    cyls = 77;
    sectsize = 128;
    sect_per_track = 26;
    sides = 1;
    mfm = 0;
  } else {
    fprintf(stderr, "error: unkown disk type: %s\n", argv[1]);
    return 1;
  }
  imdfn = argv[2];
  side[0].filename = argv[3];
  side[1].filename = argv[4];
  if (argc >= 7) {
    t0side[0].filename = argv[5];
    t0side[1].filename = argv[6];
  } else {
    t0side[0].filename = NULL;
    t0side[1].filename = NULL;
  }

  // Initialize blank disk image.
  disk = format_disk_image(cyls, sides, sect_per_track, sectsize, mfm);
  strcpy(disk->filename, imdfn);
  disk->label = "";

  // Read raw disk images.
  for (h = 0; h < sides; ++h) {
    // Read raw data for side from file.
    printf("reading side %d from %s\n", h, side[h].filename);
    side[h].size = get_file_data(side[h].filename, &side[h].data);
    if (side[h].size < 0) return 1;

    // Determine the number of tracks.
    tracks = (side[h].size / sectsize) / sect_per_track;
    if (tracks > cyls) tracks = cyls;

    // Copy tracks for side.
    printf("copy %d tracks for side %d\n", tracks, h);
    p = side[h].data;
    for (c = 0; c < tracks; ++c) {
      for (s = 0; s < sect_per_track; ++s) {
        write_disk_sector(disk, c, h, s, p, sectsize);
        p+= sectsize;
      }
    }
  }

  // Read raw disk images for track 0.
  for (h = 0; h < 2; ++h) {
    // Read raw track 0 data for side from file.
    if (!t0side[h].filename) continue;
    printf("reading track 9 side %d from %s\n", h, t0side[h].filename);
    t0side[h].size = get_file_data(t0side[h].filename, &t0side[h].data);
    if (t0side[h].size < 0) return 1;

    // Adjust format for track 0.
    if (format == CPM_FORMAT) {
      sect_per_track = 16;
      if (h == 0) {
        sectsize = 128;
        mfm = 0;
      } else {
        sectsize = 256;
        mfm = 1;
      }

      struct track *t0 = &disk->tracks[0][h];
      t0->num_sectors = sect_per_track;
      t0->sector_size = sectsize;
      t0->mfm = mfm;
    }

    // Copy track 0 for side.
    p = t0side[h].data;
    for (s = 0; s < sect_per_track; ++s) {
      write_disk_sector(disk, 0, h, s, p, sectsize);
      p+= sectsize;
    }
  }

  // Save disk image.
  printf("save disk image to %s\n", disk->filename);
  save_disk_image(disk);

  free_disk_image(disk);
  free(side[0].data);
  free(side[1].data);
  free(t0side[0].data);
  free(t0side[1].data);

  return 0;
}

