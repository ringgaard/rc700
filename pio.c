//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// Z80 PIO - Parallel I/O 
//
// PIO port A: keyboard
// PIO port B: parallel i/o
//

#include <stdio.h>
#include <string.h>

#include "rc700.h"

#define L(x)
#define LL(x)
#define W(x) x

#define PIO_KEYBOARD      0
#define PIO_PARPORT       1

#define NUM_PIO_PORTS     2

#define PIO_MODE_OUTPUT   0
#define PIO_MODE_INPUT    1
#define PIO_MODE_BIDIR    2
#define PIO_MODE_CONTROL  3

#define PIO_INT_MASK_FOLLOWS 0x10
#define PIO_INT_POLARITY     0x20
#define PIO_INT_AND          0x40
#define PIO_INT_ENABLE       0x80

struct parallel_port {
  BYTE int_vec;      // Interrupt vector
  BYTE mode;         // I/O mode
  BYTE int_ctrl;     // Interrupt control word
  BYTE mask;         // Interrupt data mask
  struct fifo rx;    // Receive buffer
  struct fifo tx;    // Transmit buffer
};

struct parallel_port pio[NUM_PIO_PORTS];

void pio_strobe(int dev) {
  if (pio[dev].int_ctrl & PIO_INT_ENABLE) {
    L(printf("pio%d: strobe\n", dev));
    interrupt(pio[dev].int_vec, dev + 6);
  }
}

void pio_receive(int dev, char *data, int size) {
  while (size-- > 0) fifo_put(&pio[dev].rx, *data++);
  if (!fifo_empty(&pio[dev].rx)) pio_strobe(dev);
}

BYTE pio_data_in(int dev) {
  BYTE data;
  
  if (fifo_empty(&pio[dev].rx)) {
    L(printf("pio%d: rx queue empty\n", dev));
    data = 0;
  } else {
    data = fifo_get(&pio[dev].rx);
    L(printf("pio%d: data in, data=%02X\n", dev, data));
  }

  if (!fifo_empty(&pio[dev].rx)) pio_strobe(dev);
  return data; 
}

void pio_data_out(BYTE data, int dev) {
  W(printf("pio%d: data out %02X (not implemented)\n", dev, data));
}

BYTE pio_ctrl_in(int dev) {
  L(printf("pio%d: ctrl in\n", dev));
  return 0; 
}

void pio_ctrl_out(BYTE data, int dev) {
  LL(printf("pio%d: ctrl out %02X\n", dev, data));
  if (pio[dev].int_ctrl & PIO_INT_MASK_FOLLOWS) {
    // Set data mask.
    L(printf("pio%d: set mask %02X\n", dev, data));
    pio[dev].mask = data;
    pio[dev].int_ctrl &= ~PIO_INT_MASK_FOLLOWS;
  } else if ((data & 0x01) == 0) {
    // Set interrupt vector.
    L(printf("pio%d: set int vec %02X\n", dev, data));
    pio[dev].int_vec = data;
  } else if ((data & 0x0f) == 0x0f) {
    // Set I/O mode.
    L(printf("pio%d: set io mode %02X\n", dev, data >> 6));
    pio[dev].mode = data >> 6;
  } else if ((data & 0x0f) == 0x07) {
    // Set interrupt control word.
    L(printf("pio%d: set int ctrl %02X\n", dev, data));
    pio[dev].int_ctrl = data & 0xf0;
  } else if ((data & 0x0f) == 0x03) {
    // Set interrupt enable.
    L(printf("pio%d: set int enable %02X\n", dev, data & 0x80));
    pio[dev].int_ctrl = (pio[dev].int_ctrl & 0x7f) | (data & 0x80);
  } else {
    W(printf("pio%d: unknown control command %02X\n", dev, data));
  }
}

int pio_poll() {
  char c;
  int ch = rcterm_keypressed();
  if (ch != (ch & 0xFF)) return 0;
  c = ch;
  L(printf("pio: key 0x%02x pressed\n", ch));
  pio_receive(0, &c, 1);
  return 1;
}

void init_pio() {
  int i;
  
  for (i = 0; i < NUM_PIO_PORTS; ++i) {
    memset(&pio[i], 0, sizeof(struct parallel_port));
  }
  register_port(0x10, pio_data_in, pio_data_out, 0);
  register_port(0x11, pio_data_in, pio_data_out, 1);
  register_port(0x12, pio_ctrl_in, pio_ctrl_out, 0);
  register_port(0x13, pio_ctrl_in, pio_ctrl_out, 1);
}

