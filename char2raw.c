#include <stdio.h>

extern unsigned char charram[];
extern unsigned char charrom[];

int main(int argc, char *argv[]) {
  int ch, x, y;

  //unsigned char *charmem = charrom;
  unsigned char *charmem = charram;

  for (ch = 0; ch < 128; ++ch) {
    char *p = charmem + (ch << 4);
    for (y = 0; y < 11; ++y) {
      int pixels = *p++;
      for (x = 0; x < 7; ++x) {
        putchar(pixels & 1 ? 0xFF : 0x00);
        pixels >>= 1;
      }
    }
  }

  return 0;
}

// ./char2raw > roa296.raw
// convert -size 7x1408 -depth 8 gray:roa296.raw -fill "#552200" -opaque black -fill "#CC9933" -opaque white roa296.gif

// ./char2raw > roa327.raw
// convert -size 7x1408 -depth 8 gray:roa327.raw -fill "#552200" -opaque black -fill "#CC9933" -opaque white roa327.gif

