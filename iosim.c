//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// I/O handler for RC700
//

#include <stdio.h>
#include "sim.h"
#include "simglb.h"

#define W(x) x
#define L(x)
#define LL(x)

struct port {
  BYTE (*in)(int dev);
  void (*out)(BYTE data, int dev);
  int dev;
};

// I/O handlers for every I/O port.
static struct port ports[256];

// Refresh callback.
void (*refresh)();

// I/O trap funtions for unused ports.
static BYTE in_trap(int dev) {
  if (i_flag) {
    W(printf("io: unhandled I/O, input from port %02X\n", dev));
    //cpu_error = IOTRAP;
    //cpu_state = STOPPED;
  }
  return((BYTE) 0);
}

static void out_trap(BYTE data, int dev) {
  if (i_flag) {
    W(printf("io: unhandled I/O, output %02X to port %02X\n", data, dev));
    //cpu_error = IOTRAP;
    //cpu_state = STOPPED;
  }
}

// Register handlers for I/O port.
void register_port(int adr, BYTE (*in)(int dev), void (*out)(BYTE data, int dev), int dev) {
  ports[adr].in = in ? in : in_trap;
  ports[adr].out = out ? out : out_trap;
  ports[adr].dev = dev;
}

void init_io() {
  int i;

  // Setup trap handler for all unused I/O ports.
  for (i = 0; i <= 255; i++) {
    register_port(i, in_trap, out_trap, i);
  }
  refresh = NULL;
}

void exit_io() {
}

BYTE io_in(BYTE adr) {
  LL(printf("in: port %x\n", adr));
  return((*ports[adr].in)(ports[adr].dev));
}

void io_out(BYTE adr, BYTE data) {
  LL(printf("out: port %x,%x\n", adr, data));
  (*ports[adr].out)(data, ports[adr].dev);
}

