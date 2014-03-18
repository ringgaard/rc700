#include <stdio.h>
#include <stdlib.h>

#define ROM_SIZE 2048

int main(int argc, char *argv[]) {
  unsigned char rom[ROM_SIZE];
  int i;
  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    perror(argv[1]);
    abort();
  }
  fread(rom, 1, ROM_SIZE, f);
  fclose(f);

  printf("/* Generated from %s by %s on %s */\n", argv[1], __FILE__, __DATE__);
  printf("#define ROM_SIZE %d\n", ROM_SIZE);
  printf("BYTE bootrom[] = {");
  for (i = 0; i < ROM_SIZE; ++i) {
    if (i % 16 == 0) printf("\n  ");
    printf("0x%02X, ", rom[i]);
  }
  printf("\n};\n");
  return 0;
}

