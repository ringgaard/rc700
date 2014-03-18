#include <stdio.h>
#include <stdlib.h>

#include "sim.h"
#include "disk.h"

//
// AnaDisk header
//
//     +------+------+------+------+------+------+----------+
//     | ACYL | ASID | LCYL | LSID | LSEC | LLEN |  COUNT   |
//     +------+------+------+------+------+------+----------+
//
//      ACYL	Actual cylinder, 1 byte
//      ASID	Actual side, 1 byte
//      LCYL	Logical cylinder; cylinder as read, 1 byte
//      LSID	Logical side; or side as read, 1 byte
//      LSEC	Sector number as read, 1 byte
//      LLEN	Length code as read, 1 byte
//      COUNT	Byte count of data to follow,  2 bytes.	  If zero,
//		no data is contained in this sector.
//

struct anahdr {
  BYTE acyl;
  BYTE asid;
  BYTE lcyl;
  BYTE lsid;
  BYTE lsec;
  BYTE llen;
  WORD count;
};

int main(int argc, char *argv[]) {
  FILE *f;
  int size;
  BYTE *p;
  BYTE *end;
  int i;
  BYTE *data;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <ana disk> <imd disk>\n", argv[0]);
    return 1;
  }

  // Read image into memory.
  f = fopen(argv[1], "rb");
  if (!f) {
    perror(argv[1]);
    return 1;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
  printf("image in %s is %d bytes\n", argv[1], size);

  data = malloc(size);
  if (fread(data, 1, size, f) != size) {
    perror(argv[1]);
    fclose(f);
    return 0;
  }
  fclose(f);

  p = data;
  end = data + size;
  while (p < end) {

    struct anahdr *hdr = (struct anahdr *) p;
    printf("acyl=%d asid=%d lcyl=%d lsid=%d lsec=%d llen=%d count=%d\n",
           hdr->acyl, hdr->asid, hdr->lcyl, hdr->lsid,
           hdr->lsec, hdr->llen, hdr->count);
           
    p += sizeof(struct anahdr) + hdr->count;
  }

  return 0;
}

