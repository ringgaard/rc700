/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * I/O handler for RC700
 *
 */

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

/*
 * This array contain function pointers for for every 
 * I/O port (0 - 255), to dispatch I/O operations.
 */
static struct port ports[256];

/*
 * Refresh callback.
 */
void (*refresh)();

/*
 *  I/O trap funtions
 *  These function should be added into all unused
 *  entrys of the port array. It stops the emulation
 *  with an I/O error.
 */
static BYTE in_trap(int dev) {
  if (i_flag) {
    W(printf("Unhandled I/O, input from port %02X\n", dev));
    cpu_error = IOTRAP;
    cpu_state = STOPPED;
  }
  return((BYTE) 0);
}

static void out_trap(BYTE data, int dev) {
  if (i_flag) {
    W(printf("Unhandled I/O, output %02X to port %02X\n", data, dev));
    cpu_error = IOTRAP;
    cpu_state = STOPPED;
  }
}

/*
 * Register handlers for I/O port
 */
void register_port(int adr, BYTE (*in)(int dev), void (*out)(BYTE data, int dev), int dev) {
  ports[adr].in = in ? in : in_trap;
  ports[adr].out = out ? out : out_trap;
  ports[adr].dev = dev;
}

/*
 *  This function initializes  the I/O devices.
 *  It will be called from the CPU simulation before
 *  any operation with the Z80 is possible.
 */
void init_io(void) {
  register int i;

  /* Setup trap handler for all unused I/O ports */
  for (i = 0; i <= 255; i++) {
    register_port(i, in_trap, out_trap, i);
  }
  refresh = NULL;
}

/*
 *  This function is to stop the I/O devices. It is
 *  called from the CPU simulation on exit.
 */
void exit_io(void) {
}

/*
 *  This is the main handler for all IN op-codes,
 *  called by the simulator. It calls the input
 *  function for port adr.
 */
BYTE io_in(BYTE adr) {
  LL(printf("in: port %x\n", adr));
  return((*ports[adr].in)(ports[adr].dev));
}

/*
 *  This is the main handler for all OUT op-codes,
 *  called by the simulator. It calls the output
 *  function for port adr.
 */
void io_out(BYTE adr, BYTE data) {
  LL(printf("out: port %x,%x\n", adr, data));
  (*ports[adr].out)(data, ports[adr].dev);
}

