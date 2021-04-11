#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "disk.h"

int main(int argc, char *argv[]) {
  struct disk *disk;
  int c, h, s, n;
  int tracks, sectors, bytes, used, unused, bad;
  int cur_sector_size, cur_num_sectors, num_tracks;
  int ds, dd, hd;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <imd disk>\n", argv[0]);
    return 1;
  }

  // Read image into memory.
  disk = load_disk_image(argv[1]);
  if (!disk) return 1;

  tracks = sectors = bytes = used = unused = bad = 0;
  cur_sector_size = cur_num_sectors = num_tracks = 0;
  ds = 0; dd = hd = 0;
  for (c = 0; c < MAX_CYLINDERS; ++c) {
    for (h = 0; h < MAX_SIDES; ++h) {
      struct track *track = &disk->tracks[c][h];
      if (!track->present) continue;
      tracks++;
      if (h > 0) ds = 1;
      if (track->mfm) dd = 1;
      if (c >= 40) hd = 1;
      printf("track %d side %d: %d sectors of %d bytes\n", c, h, track->num_sectors, track->sector_size);

      if (track->num_sectors != cur_num_sectors || track->sector_size != cur_sector_size) {
        if (num_tracks > 0) {
          printf("%d tracks: %d sectors of %d bytes\n", num_tracks, cur_num_sectors, cur_sector_size);
        }
        cur_num_sectors = track->num_sectors;
        cur_sector_size = track->sector_size;
        num_tracks = 0;
      }

      num_tracks++;
      for (s = 0; s < track->num_sectors; ++s) {
        struct sector *sector = &track->sectors[s];
        if (!sector->present) continue;
        sectors++;
        bytes += track->sector_size;
        if (sector->data) {
          used += track->sector_size;
        } else {
          unused += track->sector_size;
        }
        if (sector->bad) {
          printf("  bad sector %d/%d/%d %d bytes\n", c, h, s + 1, track->sector_size);
          bad++;
        }
        printf("  %d/%d/%d %d bytes data=%p fill=%02x phys=%d\n", c, h, s + 1, track->sector_size, sector->data, sector->fill, sector->physical);
      }
    }
  }
  if (num_tracks > 0) {
    printf("%d tracks: %d sectors of %d bytes\n", num_tracks, cur_num_sectors, cur_sector_size);
  }

  printf("\n");
  printf("summary: %s, %s\n", ds ? "double-side" : "single-side", dd ? (hd ? "high-density" : "double-density") : "single-density");
  printf("%6d tracks\n%6d sectors\n%6d bytes (%d used, %d unused)\n", tracks, sectors, bytes, used, unused);
  if (bad) printf("%6d bad sectors\n", bad);

  free_disk_image(disk);
  return 0;
}

