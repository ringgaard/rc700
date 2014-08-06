//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// ROM interface
//

#include <stdio.h>
#include <string.h>

#include "rc700.h"

#define L(x)

// PROM0 (autoloader) and PROM1 (test).

#define ROMSIZE 2048
#define PROM0_ADDR 0x0000
#define PROM1_ADDR 0x2000

unsigned char blankrom[ROMSIZE];

#ifndef PROM0
#define PROM0 rob358
#endif

#ifndef PROM1
#define PROM1 blankrom
#endif

extern unsigned char PROM0[];
extern unsigned char PROM1[];

// SEM702 programmable charater ram.
extern unsigned char charram[];
BYTE sem_char = 0;
BYTE sem_line = 0;

// DIP switches:
//   0x00: Maxi floppy (8") 
//   0x80: Mini floppy (5 1/4")

#ifndef DIPSWITCH
#define DIPSWITCH 0x80
#endif

BYTE dip_switches = DIPSWITCH;

BYTE dip_switch_in(int dev) {
  L(printf("switch: read %02X PC=%04X\n", dip_switches, (WORD) (PC - ram)));
  return dip_switches;
}

void speaker_out(BYTE data, int dev) {
  L(printf("beep: value %x\n", data));
}

void disable_prom(BYTE data, int dev) {
  L(printf("prom: disable %d\n", data));
  if (data == 0) memset(ram + PROM0_ADDR, 0, ROMSIZE);
  if (data == 1) memset(ram + PROM0_ADDR, 0, ROMSIZE);
}

void sem_char_out(BYTE data, int dev) {
  L(printf("sem: char %d\n", data));
  sem_char = data & 0x7F;
}

void sem_line_out(BYTE data, int dev) {
  L(printf("sem: line %d\n", data));
  sem_line = data & 0x0F;
}

void sem_data_out(BYTE data, int dev) {
  L(printf("sem: data %02x\n", data));
  charram[(sem_char << 4) + sem_line] = data;
}

void init_rom() {
  int i;

  // Initialize memory with contents of PROMs.
  for (i = 0; i < ROMSIZE; ++i) ram[i + PROM0_ADDR] = PROM0[i];
  for (i = 0; i < ROMSIZE; ++i) ram[i + PROM1_ADDR] = PROM1[i];

  register_port(0x14, dip_switch_in, fdc_floppy_motor, 0);
  register_port(0x18, NULL, disable_prom, 0);
  register_port(0x19, NULL, disable_prom, 1);
  register_port(0x1c, NULL, speaker_out, 0);

  // SEM702 programmable character RAM.
  register_port(0xd1, NULL, sem_char_out, 0);
  register_port(0xd2, NULL, sem_line_out, 0);
  register_port(0xd3, NULL, sem_data_out, 0);
}

