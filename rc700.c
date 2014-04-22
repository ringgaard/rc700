//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "rc700.h"

// I/O handlers for all I/O ports.
static struct port ports[256];

// Mounted virtual floppy images.
#define MAX_FLOPPIES 4

char *floppy[MAX_FLOPPIES];
int num_floppies = 0;

// Idle timing.
int refresh_ticks = 100000;
int active_delay  =   0; //5;
int idle_delay    = 0; //200;

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

// CPU poll handler.
void cpu_poll() {
  static int tick = 0;
  static int active = 1000;

  if (--tick > 0) return;
  tick = refresh_ticks;

  if (!pio_poll() && !crt_poll()) {
    if (!active) {
      delay(idle_delay);
    } else {
      --active;
      delay(active_delay);
    }
  } else {
    active = 1000;
  }
}

// CPU halt handler.
void cpu_halt() {
  delay(10);
};

// Initialize RC700 simulator.
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
  init_ftp();
  
  // Mount floppy disks images.
  for (i = 0; i < num_floppies; ++i) {
    fdc_mount_disk(i, floppy[i]);
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

int main(int argc, char *argv[]) {
#ifndef WIN32
  static struct sigaction sa;
#endif
  int suspend = 0;
  int monitor = 0;
  int i;

  char *s, *p;
  char *pn = argv[0];

  printf("RC700 Simulator version 1.0, Copyright (C) 2014 by Michael Ringgaard\n");
  printf("Z80-SIM Release 1.9, Copyright (C) 1987-2006 by Udo Munk\n\n");
  fflush(stdout);

  // Get command line parameters.
  for (i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      char *s = argv[i] + 1;
      while (s && *s) {
        switch (*s) {
          case 'm':
            monitor = 1;
            s++;
            break;

          case 's':
            suspend = 1;
            s++;
            break;

          case '?':
            goto usage;

          default:
            printf("illegal option %c\n", *s);
          usage:  
            printf("usage:\t%s -ms [IMD FILES...]\n", argv[0]);
            printf("-m (exit into monitor)\n");
            printf("-s (start suspended)\n");
            exit(1);
        }
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

  // Initialize simulator.
  init_rc700();
  rcterm_init();

  // Run simulator.
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

