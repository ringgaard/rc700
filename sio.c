//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// Z80 SIO - Serial I/O
//
// SIO port A: terminal
// SIO port B: printer
//

#include <stdio.h>
#include <string.h>

#include "rc700.h"

#define L(x)
#define LL(x)
#define W(x)

#define NUM_SIO_PORTS   2

#define FIFOSIZE        128

#define SIO_W1_EXE_INT_ENABLE        0x01  // External Interrupts Enable
#define SIO_W1_TX_INT_ENABLE         0x02  // Transmit Interrupt Enable
#define SIO_W1_STATUS_AFFECT_VECTOR  0x04  // Status Affects Vector

#define SIO_W1_RX_INT_MODE           0x18  // Receive Interrupt Mode Mask
#define SIO_W1_RX_INT_DISABLE        0x00  // Receive Interrupt Disable
#define SIO_W1_RX_INT_FIRST          0x08  // Receive Interrupt on First Character
#define SIO_W1_RX_INT_PARITY         0x10  // Receive Interrupt on All Characters (Parity affects vector)
#define SIO_W1_RX_INT_ALL            0x18  // Receive Interrupt on All Characters (Parity does not affect vector)

#define SIO_W1_WR_RXTX               0x20  // Wait/Ready on Receive Transmit
#define SIO_W1_WR_FUNC               0x40  // Wait/Ready Function
#define SIO_W1_WR_ENABLE             0x80  // Wait/Ready Enable

#define SIO_R0_RX_AVAIL              0x01  // Receive Character available
#define SIO_R0_INT                   0x02  // Interrupt Pending
#define SIO_R0_TX_PEND               0x04  // Transmit Buffer Empty

struct serial_port {
  BYTE wr[8];
  BYTE rr[3];
  struct fifo rx;
  struct fifo tx;
};

struct serial_port sio[NUM_SIO_PORTS];

void sio_check_receive(int dev) {
  if (fifo_empty(&sio[dev].rx)) {
    sio[dev].rr[0] &= ~SIO_R0_RX_AVAIL;
  } else {
    sio[dev].rr[0] |= SIO_R0_RX_AVAIL;
    if (sio[dev].wr[1] & SIO_W1_RX_INT_MODE) {
      if (!(sio[dev].rr[0] & SIO_R0_INT)) {
        // TODO: Check for Status Affects Vector bit to determine intvec
        L(printf("sio%d: gen intr %02X\n", dev, sio[1].wr[2]));
        sio[dev].rr[0] |= SIO_R0_INT;
        interrupt(sio[1].wr[2], dev + 4);
      }
    }
  }
}

void sio_receive(int dev, char *data, int size) {
  while (size-- > 0) fifo_put(&sio[dev].rx, *data++);
  sio_check_receive(dev);
}

void sio_reset(int dev) {
  memset(&sio[dev], 0, sizeof(struct serial_port));
  sio[dev].rr[0] |= SIO_R0_TX_PEND;
}

BYTE sio_data_in(int dev) {
  BYTE data;

  sio[dev].rr[0] &= ~SIO_R0_INT;
  if (fifo_empty(&sio[dev].rx)) {
    W(printf("sio%d: rx queue empty\n", dev));
    data = 0;
  } else {
    data = fifo_get(&sio[dev].rx);
    L(printf("sio%d: data in, data=%02X\n", dev, data));
  }

  sio_check_receive(dev);
  return data; 
}

void sio_data_out(BYTE data, int dev) {
  LL(printf("sio%d: data out %02X\n", dev, data));
  putchar(data);
  sio[dev].rr[0] |= SIO_R0_TX_PEND;
  if (sio[dev].wr[1] & SIO_W1_TX_INT_ENABLE) {
    sio[dev].rr[0] |= SIO_R0_INT;
    interrupt(sio[1].wr[2], dev + 4);
  }
}

BYTE sio_ctrl_in(int dev)  {
  int ptr;

  LL(printf("sio%d: ctrl in\n", dev));
  ptr = sio[dev].wr[0] & 0x07;
  sio[dev].wr[0] &= ~0x07;
  L(printf("sio%d: read %02X from reg %d\n", dev, sio[dev].rr[ptr], ptr));
  return sio[dev].rr[ptr];
}

void sio_ctrl_out(BYTE data, int dev) {
  int ptr;

  LL(printf("sio%d: ctrl out %02X\n", dev, data));
  ptr = sio[dev].wr[0] & 0x07;
  sio[dev].wr[0] &= ~0x07;
  sio[dev].wr[ptr] = data;

  if (ptr == 0) {
    int cmd = (data >> 3) & 0x07;
    LL(printf("sio%d: command %d\n", dev, cmd));
    switch (cmd) {
      // Null
      case 0: 
        break;

      // Send abort 
      case 1:
        W(printf("sio%d: send abort, not implemented\n", dev));
        break;

      // Reset Ext/Status Interrupts
      case 2: 
        W(printf("sio%d: reset status, not implemented\n", dev));
        break;

      // Channel reset
      case 3: 
        L(printf("sio%d: reset\n", dev));
        sio_reset(dev); 
        break;

      // Enable INT on Next Ax Character
      case 4:
        W(printf("sio%d: enable int, not implemented\n", dev));
        break;

      // Reset TxINT Pending
      case 5: 
        W(printf("sio%d: reset tx int pending, not implemented\n", dev));
        break;
 
      // Error Reset
      case 6:
        W(printf("sio%d: error reset, not implemented\n", dev));
        break;

      // Return from INT (CH-A Only)
      case 7:
        W(printf("sio%d: return from int, not implemented\n", dev));
        break; 
    }
  } else {
    L(printf("sio%d: write %02X to reg %02X\n", dev, data, ptr));
  }

  sio_check_receive(dev);
}

void init_sio() {
  sio_reset(0);
  sio_reset(1);

  register_port(0x08, sio_data_in, sio_data_out, 0);
  register_port(0x09, sio_data_in, sio_data_out, 1);
  register_port(0x0a, sio_ctrl_in, sio_ctrl_out, 0);
  register_port(0x0b, sio_ctrl_in, sio_ctrl_out, 1);
}

