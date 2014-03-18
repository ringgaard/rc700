#include <stdio.h>

int main(int argc, char *argv[]) {
  int count;
  int fd;
  int i;
  char *filename;
  char buffer[2014];

  for (i = 1; i < argc; i++) {
    filename = argv[i];
    if ((fd = open(filename, 0)) < 0) {
      fprintf(stderr, "%s: %s\n", filename, strerror(fd));
      return 1;
    }
    while ((count = read(fd, buffer, sizeof buffer)) > 0) {
      int eof = 0;
      char *p = buffer;
      while (count > 0) {
        switch (*p) {
          case 0x1a: eof = 1; break;
          case '{': fputs("æ", stdout); break;
          case '|': fputs("ø", stdout); break;
          case '}': fputs("å", stdout); break;
          case '[': fputs("Æ", stdout); break;
          case '\\': fputs("Ø", stdout); break;
          case ']': fputs("Å", stdout); break;
          default: fputc(*p, stdout);
        }
        count--;
        p++;
      }
      if (eof) break;
    }
    close(fd);
  }
  return 0;
}

