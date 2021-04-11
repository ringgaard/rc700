//
// RC700  -  a Regnecentralen RC700 emulator
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
#define SIO_R0_DCD                   0x08  // Data Carrier Detect
#define SIO_R0_CTS                   0x20  // Clear To Send

struct serial_port {
  BYTE wr[8];
  BYTE rr[3];
  struct fifo rx;
  struct fifo tx;
};

struct serial_port sio[NUM_SIO_PORTS];

void sio_dump_status(BYTE value, BYTE bit, const char *func) {
  if (value & (1 << bit)) printf(" [D%d: %s]", bit, func);
}

void sio_dump_read_register(int dev, BYTE reg, BYTE value) {
  printf("sio%d: read %02X from reg %02X:", dev, value, reg);
  switch (reg) {
    case 0:
      sio_dump_status(value, 0, "Receive Character available");
      sio_dump_status(value, 1, "Interrupt Pending");
      sio_dump_status(value, 2, "Transmit Buffer Empty");
      sio_dump_status(value, 3, "DCD");
      sio_dump_status(value, 4, "Sync/Hunt Buffer Empty");
      sio_dump_status(value, 5, "CTS");
      sio_dump_status(value, 6, "Transmit Underrun EOM");
      sio_dump_status(value, 7, "Break/Abort");
      break;

    case 1:
      sio_dump_status(value, 0, "All sent");
      printf(" [Residue Code: %02X]", (value >> 1) & 7);
      sio_dump_status(value, 4, "Parity Error");
      sio_dump_status(value, 5, "Receiver Overrun Error");
      sio_dump_status(value, 6, "CRC/Framing Error");
      sio_dump_status(value, 7, "End of Frame (SDLC)");
      break;

    case 2:
      printf(" [Interrupt Vector %02X]", value);
      break;
  }
  printf("\n");
}

void sio_dump_write_register(int dev, BYTE reg, BYTE value) {
  printf("sio%d: write %02X to reg %02X:", dev, value, reg);
  switch (reg) {
    case 1:
      sio_dump_status(value, 0, "External Interrupts Enable");
      sio_dump_status(value, 1, "Transmit Interrupt Enable");
      sio_dump_status(value, 2, "Status Affects Vector");
      switch ((value >> 3) & 3) {
        case 0: printf(" [Receive Interrupts Disabled]"); break;
        case 1: printf(" [Receive Interrupt On First Character Only]"); break;
        case 2: printf(" [Interrupt On All Receive Characters, Parity Special]"); break;
        case 3: printf(" [Interrupt On All Receive Characters]"); break;
      }
      sio_dump_status(value, 5, "Wait/Ready on Receive Transmit");
      sio_dump_status(value, 6, "Wait or Ready Function");
      sio_dump_status(value, 7, "Wait/Ready Enable");
      break;

    case 2:
      printf(" [Interrupt Vector %02X]", value);
      break;

    case 3:
      sio_dump_status(value, 0, "Receiver Enable");
      sio_dump_status(value, 1, "Sync Char Load Inhibit");
      sio_dump_status(value, 2, "Address Search Mode");
      sio_dump_status(value, 3, "Receiver CRC Enable");
      sio_dump_status(value, 4, "Enter Hunt Phase");
      sio_dump_status(value, 5, "Auto Enables");
      printf(" [Receiver Bits/Char: %c]", "5768"[value >> 6]);
      break;

    case 4:
      sio_dump_status(value, 0, "Parity Enable");
      printf(" [Parity: %s]", value & 2 ? "even" : "odd");
      switch ((value >> 2) & 3) {
        case 0: printf(" [Sync Mode]"); break;
        case 1: printf(" [1 Stop Bit]"); break;
        case 2: printf(" [1-1/2 Stop Bits]"); break;
        case 3: printf(" [2 Stop Bits]"); break;
      }
      switch ((value >> 4) & 3) {
        case 0: printf(" [8-Bit SYNC Character]"); break;
        case 1: printf(" [16-Bit SYNC Character]"); break;
        case 2: printf(" [SDLC Mode]"); break;
        case 3: printf(" [External SYNC Mode]"); break;
      }
      switch ((value >> 6) & 3) {
        case 0: printf(" [X1 Clock Mode]"); break;
        case 1: printf(" [X16 Clock Mode]"); break;
        case 2: printf(" [X32 Clock Mode]"); break;
        case 3: printf(" [X64 Clock Mode]"); break;
      }
      break;

    case 5:
      sio_dump_status(value, 0, "Tx CRC Enable");
      sio_dump_status(value, 1, "RTS");
      sio_dump_status(value, 2, "CRC-16/!SDLC");
      sio_dump_status(value, 3, "Tx Enable");
      sio_dump_status(value, 4, "Send Break");
      switch ((value >> 5) & 3) {
        case 0: printf(" [8-Bit SYNC Character]"); break;
        case 1: printf(" [16-Bit SYNC Character]"); break;
        case 2: printf(" [SDLC Mode]"); break;
        case 3: printf(" [External SYNC Mode]"); break;
      }
      sio_dump_status(value, 7, "DTR");
      break;

    case 6:
      printf(" [SYNC Bits 0-7: %02X]", value);
      break;

    case 7:
      printf(" [SYNC Bits 8-15: %02X]", value);
      break;
  }
  printf("\n");
}

void sio_check_receive(int dev) {
  if (fifo_empty(&sio[dev].rx)) {
    sio[dev].rr[0] &= ~SIO_R0_RX_AVAIL;
  } else {
    sio[dev].rr[0] |= SIO_R0_RX_AVAIL;
    if (sio[dev].wr[1] & SIO_W1_RX_INT_MODE) {
      if (!(sio[0].rr[0] & SIO_R0_INT)) {
        // TODO: Check for Status Affects Vector bit to determine intvec
        L(printf("sio%d: gen rx intr %02X\n", dev, sio[1].wr[2]));
        sio[0].rr[0] |= SIO_R0_INT;
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
  sio[dev].rr[0] |= SIO_R0_TX_PEND | SIO_R0_DCD | SIO_R0_CTS;
}

BYTE sio_data_in(int dev) {
  BYTE data;

  sio[0].rr[0] &= ~SIO_R0_INT;
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
  rcterm_print(data);
  sio[dev].rr[0] |= SIO_R0_TX_PEND;
  if (sio[dev].wr[1] & SIO_W1_TX_INT_ENABLE) {
    // TODO: Check for Status Affects Vector bit to determine intvec
    if (sio[1].wr[1] & SIO_W1_STATUS_AFFECT_VECTOR) {
      L(printf("sio%d: status affect vector\n", dev));
    }
    L(printf("sio%d: gen tx intr %02X\n", dev, sio[1].wr[2]));
    sio[0].rr[0] |= SIO_R0_INT;
    interrupt(sio[1].wr[2], dev + 4);
  }
}

BYTE sio_ctrl_in(int dev)  {
  int ptr;

  LL(printf("sio%d: ctrl in\n", dev));
  ptr = sio[dev].wr[0] & 0x07;
  sio[dev].wr[0] &= ~0x07;
  L(sio_dump_read_register(dev, ptr, sio[dev].rr[ptr]));
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
        L(printf("sio%d: reset status\n", dev));
        if (dev == 0) sio[dev].rr[0] &= ~SIO_R0_INT;
        sio[dev].rr[0] |= SIO_R0_TX_PEND;
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
        L(printf("sio%d: reset tx int pending\n", dev));
        sio[0].rr[0] &= ~SIO_R0_INT;
        sio[dev].rr[0] &= ~SIO_R0_TX_PEND;
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
    L(sio_dump_write_register(dev, ptr, data));
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

