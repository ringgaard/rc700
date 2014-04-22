//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// ROM interface
//

#include <stdio.h>
#include <string.h>

#include "rc700.h"

#ifdef WIN32
#include "winbootrom"
#else
#include "bootrom"
#endif

#define L(x)

// SEM702 programmable charater ram.
extern unsigned char charram[];
BYTE sem_char = 0;
BYTE sem_line = 0;

// DIP switches:
//   0x00: Maxi floppy (8") 
//   0x80: Mini floppy (5 1/4")
BYTE dip_switches = 0x80;

void fdc_floppy_motor(BYTE data, int dev);

BYTE dip_switch_in(int dev) {
  L(printf("switch: read %02X PC=%04X\n", dip_switches, PC - ram));
  return dip_switches;
}

void speaker_out(BYTE data, int dev) {
  L(printf("beep: value %x\n", data));
}

void disable_prom(BYTE data, int dev) {
  L(printf("prom: disable=%x\n", data));
  memset(ram, 0, 2048);
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

  // Initialize memory with contents of boot prom.
  for (i = 0; i < ROM_SIZE; ++i) ram[i] = bootrom[i];

  register_port(0x14, dip_switch_in, fdc_floppy_motor, 0);
  register_port(0x18, NULL, disable_prom, 0);
  register_port(0x1c, NULL, speaker_out, 0);

  // SEM702 programmable character RAM.
  register_port(0xd1, NULL, sem_char_out, 0);
  register_port(0xd2, NULL, sem_line_out, 0);
  register_port(0xd3, NULL, sem_data_out, 0);
}

