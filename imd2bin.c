#include <stdio.h>
#include <stdlib.h>

#include "sim.h"
#include "disk.h"

struct disk disk;

int main(int argc, char *argv[]) {
  FILE *f;
  int c, h, s, n;

  if (argc != 3) {
    fprintf(stderr, "usage: %s <imd disk> <bin disk>\n", argv[0]);
    return 1;
  }

  // Read image into memory.
  load_disk_image(argv[1], &disk);

  // Write raw disk image.
  f = fopen(argv[2], "wb");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  for (c = 0; c < MAX_CYLINDERS; ++c) {
    for (h = 0; h < MAX_SIDES; ++h) {
      struct track *track = &disk.tracks[c][h];
      for (s = 0; s < track->num_sectors; ++s) {
        struct sector *sector = &track->sectors[s];
        printf("%d/%d/%d %d bytes data=%p fill=%02x\n", c, h, s + 1, track->sector_size, sector->data, sector->fill);
        if (sector->data) {
          fwrite(sector->data, 1, track->sector_size, f);
        } else {
          for (n = 0; n < track->sector_size; ++n) fwrite(&sector->fill, 1, 1, f);
        }
      }
    }
  }
  fclose(f);

  return 0;
}

