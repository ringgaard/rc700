/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * ROM interface
 */

#include <stdio.h>
#include <string.h>
#include "sim.h"
#include "simglb.h"

#include "bootrom"

#define L(x)

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

void init_rom(void) {
  int i;

  // Initialize memory with contents of boot prom.
  for (i = 0; i < ROM_SIZE; ++i) ram[i] = bootrom[i];

  register_port(0x14, dip_switch_in, fdc_floppy_motor, 0);
  register_port(0x18, NULL, disable_prom, 0);
  register_port(0x1c, NULL, speaker_out, 0);
}

