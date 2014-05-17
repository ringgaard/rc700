#include <stdio.h>

int main(int argc, char *argv[]) {
  unsigned char data[16];
  unsigned char sum, checksum;
  int addr = 0;
  int i, n;
  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  if (argc > 2) scanf("%x", &addr);

  while ((n = fread(data, 1, sizeof(data), f)) > 0) {
    printf(":%02X%04X00", n, addr);
    sum = n + addr + (addr >> 8);
    for (i = 0; i < n; ++i) {
      printf("%02X", data[i]);
      sum += data[i];
    }
    checksum = -sum;
    printf("%02X\n", checksum);
    addr += n;
  }
  printf(":0000000000\n");
  fclose(f);
  return 0;
}

