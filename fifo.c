//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// FIFO buffers
//

#include <stdio.h>
#include <string.h>

#include "simglb.h"
#include "sim.h"

int fifo_put(struct fifo *f, BYTE data) {
  if (f->count >= FIFOSIZE) return 0;
  f->queue[f->head++] = data;
  f->count++;
  if (f->head >= FIFOSIZE) f->head = 0;
  return 1;
}

BYTE fifo_get(struct fifo *f) {
  BYTE data;

  if (f->count == 0) return 0;
  data = f->queue[f->tail++];
  f->count--;
  if (f->tail == FIFOSIZE) f->tail = 0;
  return data;
}

int fifo_empty(struct fifo *f) {
  return f->count == 0;
}

int fifo_full(struct fifo *f) {
  return f->count == FIFOSIZE;
}

