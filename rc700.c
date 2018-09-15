//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "rc700.h"

// I/O handlers for all I/O ports.
static struct port ports[256];

// Mounted virtual disk images.
#define MAX_FLOPPIES 4

char *floppy[MAX_FLOPPIES];
int num_floppies = 0;
char *harddisk = NULL;
char *ftpdir = NULL;

// Emulate a 4 Mhz Z80 CPU with 50 Hz screen refresh rate.
#define CPU_CLOCK_FREQUENCY   4000000
#define FRAMES_PER_SECOND     50
#define CYCLES_PER_FRAME      (CPU_CLOCK_FREQUENCY / FRAMES_PER_SECOND)
#define MILLISECS_PER_FRAME   (1000 / FRAMES_PER_SECOND)

// Number of CPU cycles executed in current frame.
int quantum = 0;

// Milliseconds delay per frame taking emulation speed into account.
int ms_per_frame = MILLISECS_PER_FRAME;

// Delay in milliseconds.
void delay(int ms) {
#ifdef WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

// CPU port input trap.
static BYTE in_trap(int dev) {
  printf("io: unhandled I/O, input from port %02X\n", dev);
  //cpu_error = IOTRAP;
  //cpu_state = STOPPED;
  return 0;
}

// CPU port output trap.
static void out_trap(BYTE data, int dev) {
  printf("io: unhandled I/O, output %02X to port %02X\n", data, dev);
  //cpu_error = IOTRAP;
  //cpu_state = STOPPED;
}

// Register handlers for CPU I/O port.
void register_port(int adr, BYTE (*in)(int dev), void (*out)(BYTE data, int dev), int dev) {
  ports[adr].in = in ? in : in_trap;
  ports[adr].out = out ? out : out_trap;
  ports[adr].dev = dev;
}

// CPU port input handler.
BYTE cpu_in(BYTE adr) {
  return (*ports[adr].in)(ports[adr].dev);
}

// CPU port output handler.
void cpu_out(BYTE adr, BYTE data) {
  (*ports[adr].out)(data, ports[adr].dev);
}

// Set emulator speed in percent.
void set_emulation_speed(int percent) {
  ms_per_frame = MILLISECS_PER_FRAME * 100 / percent;
  printf("speed: %d%%, %d ms/frame\n", percent, ms_per_frame);
}

// CPU poll handler.
void cpu_poll(int cycles) {
  int overhead;
  clock_t t;

  quantum += cycles;
  if (quantum < CYCLES_PER_FRAME) return;
  t = clock();

  if (pio_poll() || crt_poll()) {
    overhead = (clock() - t) * 1000 / CLOCKS_PER_SEC;
  } else {
    overhead = 0;
  }

  if (overhead < ms_per_frame) delay(ms_per_frame - overhead);
  quantum -= CYCLES_PER_FRAME;
}

// CPU halt handler.
void cpu_halt() {
#ifdef SBSIZE
  cpu_error = OPHALT;
  cpu_state = STOPPED;
#else
  delay(ms_per_frame);
#endif
};

// Initialize RC700 emulator.
static void init_rc700() {
  int i;

  // Clear RAM and start executing at 0x000.
  PC = STACK = ram;
  memset(ram, 0, 65536);

  // Setup trap handler for all unused I/O ports.
  for (i = 0; i <= 255; i++) {
    register_port(i, in_trap, out_trap, i);
  }

  // Initialize peripheral devices.
  init_rom();
  init_pio();
  init_sio();
  init_ctc();
  init_dma();
  init_crt();
  init_fdc();
  init_wdc();
  init_ftp(ftpdir);
#ifdef HAS_MEMDISK
  init_mdc();
#endif

  // Mount floppy disks images.
  for (i = 0; i < num_floppies; ++i) {
    fdc_mount_disk(i, floppy[i], 0);
  }

  // Mount hard disks image.
  if (harddisk) {
    wdc_mount_harddisk(0, harddisk);
  }
}

static void simbreak(int sig) {
  cpu_error = USERINT;
  cpu_state = STOPPED;
}

static void dump_ram(char *core) {
  FILE *f;

  printf("dump ram to %s\n", core);
  f = fopen(core, "wb");
  if (!f) {
    perror(core);
    return;
  }
  fwrite(ram, 65536, 1, f);
  fclose(f);
}

void usage(char *pgm) {
  printf("usage:\t%s [OPTIONS] [IMD FILES...]\n", pgm);
  printf("-monitor     (exit into monitor)\n");
  printf("-suspend     (start suspended)\n");
  printf("-speed PCT   (set emulation speed)\n");
  printf("-hd IMG      (mount hard disk)\n");
  printf("-bootf       (force boot from floppy)\n");
  printf("-ftp DIR     (ftp directory)\n");
}

#ifdef __APPLE__
int SDL_main(int argc, char *argv[]) {
#else
int main(int argc, char *argv[]) {
#endif
#ifndef WIN32
  static struct sigaction sa;
#endif
  int suspend = 0;
  int monitor = 0;
  int bootf = 0;
  int i;

  char *s, *p;
  char *pn = argv[0];

  printf("RC700 Emulator version 1.0, Copyright (C) 2014 by Michael Ringgaard\n");
  printf("Z80-SIM Release 1.23, Copyright (C) 1987-2014 by Udo Munk\n\n");
  fflush(stdout);

  // Get command line parameters.
  for (i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-monitor") == 0) {
        monitor = 1;
      } else if (strcmp(argv[i], "-suspend") == 0) {
        suspend = 1;
      } else if (strcmp(argv[i], "-bootf") == 0) {
        bootf = 1;
      } else if (strcmp(argv[i], "-speed") == 0 && i + 1 < argc) {
        set_emulation_speed(atoi(argv[i++ + 1]));
      } else if (strcmp(argv[i], "-hd") == 0 && i + 1 < argc) {
        harddisk = argv[i++ + 1];
      } else if (strcmp(argv[i], "-ftp") == 0 && i + 1 < argc) {
        ftpdir = argv[i++ + 1];
      } else {
        usage(argv[0]);
        exit(1);
      }
    } else {
      if (num_floppies == MAX_FLOPPIES) {
        fprintf(stderr, "Too many floppies\n");
        exit(1);
      }
      floppy[num_floppies++] = argv[i];
    }
  }

#ifndef WIN32
  sa.sa_handler = simbreak;
  sigaction(SIGINT, &sa, NULL);
#endif

  // Initialize emulator.
  init_rc700();
  rcterm_init();

  // Patch boot ROM to boot from floppy.
  if (bootf) rom_floppy_boot_patch();

  // Run emulator.
  if (suspend) {
    mon();
  } else if (monitor) {
    cpu_state = CONTIN_RUN;
    cpu_error = NONE;
    cpu();
    mon();
  } else {
    cpu_state = CONTIN_RUN;
    cpu_error = NONE;
    cpu();
  }
  rcterm_exit();

  //dump_ram("core.bin");

#ifndef WIN32
  sa.sa_handler = SIG_DFL;
  sigaction(SIGINT, &sa, NULL);
#endif

  // Flush changes to floppy disk images.
  for (i = 0; i < num_floppies; ++i) {
    fdc_flush_disk(i);
  }

  return 0;
}

