//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "sim.h"
#include "simglb.h"
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void init_io();
void exit_io();
int poll_crt();
int poll_pio();

#define MAX_FLOPPIES 4

char *floppy[MAX_FLOPPIES];
int num_floppies = 0;

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

static void init_rc700() {
  int i;

  // Clear RAM and start executing at 0x000.
  PC = STACK = ram;
  memset(ram, 0, 65536);

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

  printf("%s Release %s, %s\n", USR_COM, USR_REL, USR_CPR);
  printf("Z80-SIM Release %s, %s\n\n", RELEASE, COPYR);
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
  init_io();
  init_rc700();

  // Run simulator.
  if (suspend) {
    mon();
    rcterm_init();
    cpu();
    rcterm_exit();
  } else if (monitor) {
    rcterm_init();
    cpu_state = CONTIN_RUN;
    cpu_error = NONE;
    cpu();
    rcterm_exit();
    mon();
  } else {
    rcterm_init();
    cpu_state = CONTIN_RUN;
    cpu_error = NONE;
    cpu();
    rcterm_exit();
  }

  exit_io();

  //dump_ram("core.bin");

#ifndef WIN32
  sa.sa_handler = SIG_DFL;
  sigaction(SIGINT, &sa, NULL);
#endif

  // Flush changes to floppy disk images.  
  for (i = 0; i < num_floppies; ++i) {
    if (floppy[i]) fdc_flush_disk(i, floppy[i]);
  }

  return 0;
}

