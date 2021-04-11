#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "disk.h"

#define SECTOR_OFFSET 1

int main(int argc, char *argv[]) {
  struct disk *disk;
  int c, h, s;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <imd disk>\n", argv[0]);
    return 1;
  }

  // Read image into memory.
  disk = load_disk_image(argv[1]);
  if (!disk) return 1;

  // Add sector offset.
  for (c = 0; c < MAX_CYLINDERS; ++c) {
    for (h = 0; h < MAX_SIDES; ++h) {
      struct track *track = &disk->tracks[c][h];
      for (s = 0; s < track->num_sectors; ++s) {
        track->sectors[s].physical = s + SECTOR_OFFSET;
      }
    }
  }

  save_disk_image(disk);
  free_disk_image(disk);

  return 0;
}

