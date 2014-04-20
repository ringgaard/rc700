/*
 * RC700  -  a Regnecentralen RC700 simulator
 *
 * Copyright (C) 2012 by Michael Ringgaard
 *
 * Z80 CTC - Counter and Timer Channels 
 *
 * Channel 0: Baudrate SIO A
 * Channel 1: Baudrate SIO B
 * Channel 2: CRT interrupt bridge
 * Channel 3: FDC interrupt bridge
 */

#include <stdio.h>
#include <string.h>
#include "sim.h"
#include "simglb.h"

#define L(x)
#define W(x) x

#define NUM_CTC_CHANNELS  4

#define CTC_CTRL_RESET        0x02
#define CTC_CTRL_TIME_FOLLOWS 0x04
#define CTC_CTRL_TRIGGER      0x08
#define CTC_CTRL_EDGE         0x10
#define CTC_CTRL_PRESCALAR    0x20
#define CTC_CTRL_MODE         0x40
#define CTC_CTRL_INT          0x80

struct counter_channel {
  int int_vec;      /* Interrupt vector */
  BYTE control;     /* Control register */
  BYTE time;        /* Time constant register */
  int counter;
};

struct counter_channel ctc[NUM_CTC_CHANNELS];

BYTE ctc_in(int channel) {
  W(printf("ctc%d: read, not yet implemented\n", channel));
  return 0; 
}

void ctc_out(BYTE data, int channel) {
  if (ctc[channel].control & CTC_CTRL_TIME_FOLLOWS) {
    L(printf("ctc%d: set time constant %02X\n", channel, data));
    ctc[channel].time = data;
    if (ctc[channel].counter == 0) ctc[channel].counter = data;
    ctc[channel].control &= ~CTC_CTRL_TIME_FOLLOWS;
  } else if (data & 1) {
    L(printf("ctc%d: set control reg %02X\n", channel, data));
    ctc[channel].control = data;
  } else {
    int i;
    L(printf("ctc%d: set int vector %02X\n", channel, data));
    if (channel != 0) W(printf("ctc%d: should only set channel 0 int vector\n", channel));
    for (i = 0; i < NUM_CTC_CHANNELS; ++i) {
      ctc[i].int_vec = (data & 0xf8) | (i << 1);
    }
  }
}

void ctc_trigger(int channel) {
  if (--ctc[channel].counter == 0) {
    ctc[channel].counter = ctc[channel].time;
    if (ctc[channel].control & CTC_CTRL_INT)  {
      L(printf("ctc%d: intr vec %02X\n", channel, ctc[channel].int_vec));
      interrupt(ctc[channel].int_vec, channel);
    }
  }
}

void init_ctc(void) {
  int i;
  
  for (i = 0; i < NUM_CTC_CHANNELS; ++i) {
    memset(&ctc[i], 0, sizeof(struct counter_channel));
  }

  register_port(0x0c, ctc_in, ctc_out, 0);
  register_port(0x0d, ctc_in, ctc_out, 1);
  register_port(0x0e, ctc_in, ctc_out, 2);
  register_port(0x0f, ctc_in, ctc_out, 3);
}

