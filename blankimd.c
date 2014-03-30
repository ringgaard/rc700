#include <stdio.h>
#include <stdlib.h>

#include "sim.h"
#include "disk.h"

int main(int argc, char *argv[]) {
  struct disk *disk;
  int c, h, s, n;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <imd disk>\n", argv[0]);
    return 1;
  }

  // Read image into memory.
  disk = load_disk_image(argv[1]);
  if (!disk) return 1;
  disk->label = "";

  // Clear all data.
  for (c = 0; c < disk->num_tracks; ++c) {
    for (h = 0; h < MAX_SIDES; ++h) {
      struct track *track = &disk->tracks[c][h];
      for (s = 0; s < track->num_sectors; ++s) {
        fill_disk_sector(disk, c, h, s, 0xe5);
      }
    }
  }

  save_disk_image(disk, argv[1]);
  free_disk_image(disk);

  return 0;
}

