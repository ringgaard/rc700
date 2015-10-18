#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "disk.h"

int main(int argc, char *argv[]) {
  FILE *f;
  struct disk *disk;
  struct track *track;
  int c, h, s, n;

  if (argc != 3) {
    fprintf(stderr, "usage: %s <imd disk> <bios img file>\n", argv[0]);
    return 1;
  }

  // Read image into memory.
  disk = load_disk_image(argv[1]);
  if (!disk) return 1;

  // Write first track from disk image which contains the BIOS
  f = fopen(argv[2], "wb");
  if (!f) {
    perror(argv[1]);
    return 1;
  }

  c = 0;
  for (h = 0; h < MAX_SIDES; ++h) {
    struct track *track = &disk->tracks[c][h];
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

  fclose(f);
  free_disk_image(disk);

  return 0;
}

