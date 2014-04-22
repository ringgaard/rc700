//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
// Modified for RC700 simulator by Michael Ringgaard
//
// Emulation of multi byte opcodes starting with 0xdd 0xcb
//

#include "simglb.h"

// Trap for illegal 0xdd 0xcb multi byte opcodes.
static int trap_ddcb() {
  cpu_error = OPTRAP4;
  cpu_state = STOPPED;
  return 0;
}

// BIT 0,(IX+d)
static int op_tb0ixd(int data) {
  F &= ~(N_FLAG | S_FLAG);
  F |= H_FLAG;
  (*(ram + IX + data) & 1) ? (F &= ~(Z_FLAG | P_FLAG))
         : (F |= (Z_FLAG | P_FLAG));
  return 20;
}

// BIT 1,(IX+d)
static int op_tb1ixd(int data) {
  F &= ~(N_FLAG | S_FLAG);
  F |= H_FLAG;
  (*(ram + IX + data) & 2) ? (F &= ~(Z_FLAG | P_FLAG))
         : (F |= (Z_FLAG | P_FLAG));
  return 20;
}

// BIT 2,(IX+d)
static int op_tb2ixd(int data) {
  F &= ~(N_FLAG | S_FLAG);
  F |= H_FLAG;
  (*(ram + IX + data) & 4) ? (F &= ~(Z_FLAG | P_FLAG))
         : (F |= (Z_FLAG | P_FLAG));
  return 20;
}

// BIT 3,(IX+d)
static int op_tb3ixd(int data) {
  F &= ~(N_FLAG | S_FLAG);
  F |= H_FLAG;
  (*(ram + IX + data) & 8) ? (F &= ~(Z_FLAG | P_FLAG))
         : (F |= (Z_FLAG | P_FLAG));
  return 20;
}

// BIT 4,(IX+d)
static int op_tb4ixd(int data) {
  F &= ~(N_FLAG | S_FLAG);
  F |= H_FLAG;
  (*(ram + IX + data) & 16) ? (F &= ~(Z_FLAG | P_FLAG))
          : (F |= (Z_FLAG | P_FLAG));
  return 20;
}

// BIT 5,(IX+d)
static int op_tb5ixd(int data) {
  F &= ~(N_FLAG | S_FLAG);
  F |= H_FLAG;
  (*(ram + IX + data) & 32) ? (F &= ~(Z_FLAG | P_FLAG))
          : (F |= (Z_FLAG | P_FLAG));
  return 20;
}

// BIT 6,(IX+d)
static int op_tb6ixd(int data) {
  F &= ~(N_FLAG | S_FLAG);
  F |= H_FLAG;
  (*(ram + IX + data) & 64) ? (F &= ~(Z_FLAG | P_FLAG))
          : (F |= (Z_FLAG | P_FLAG));
  return 20;
}

// BIT 7,(IX+d)
static int op_tb7ixd(int data) {
  F &= ~N_FLAG;
  F |= H_FLAG;
  if (*(ram + IX + data) & 128) {
    F &= ~(Z_FLAG | P_FLAG);
    F |= S_FLAG;
  } else {
    F |= (Z_FLAG | P_FLAG);
    F &= ~S_FLAG;
  }
  return 20;
}

// RES 0,(IX+d)
static int op_rb0ixd(int data) {
  *(ram + IX + data) &= ~1;
  return 23;
}

// RES 1,(IX+d)
static int op_rb1ixd(int data) {
  *(ram + IX + data) &= ~2;
  return 23;
}

// RES 2,(IX+d)
static int op_rb2ixd(int data) {
  *(ram + IX + data) &= ~4;
  return 23;
}

// RES 3,(IX+d)
static int op_rb3ixd(int data) {
  *(ram + IX + data) &= ~8;
  return 23;
}

// RES 4,(IX+d)
static int op_rb4ixd(int data) {
  *(ram + IX + data) &= ~16;
  return 23;
}

// RES 5,(IX+d)
static int op_rb5ixd(int data) {
  *(ram + IX + data) &= ~32;
  return 23;
}

// RES 6,(IX+d)
static int op_rb6ixd(int data) {
  *(ram + IX + data) &= ~64;
  return 23;
}

// RES 7,(IX+d)
static int op_rb7ixd(int data) {
  *(ram + IX + data) &= ~128;
  return 23;
}

// SET 0,(IX+d)
static int op_sb0ixd(int data) {
  *(ram + IX + data) |= 1;
  return 23;
}

// SET 1,(IX+d)
static int op_sb1ixd(int data) {
  *(ram + IX + data) |= 2;
  return 23;
}

// SET 2,(IX+d)
static int op_sb2ixd(int data) {
  *(ram + IX + data) |= 4;
  return 23;
}

// SET 3,(IX+d)
static int op_sb3ixd(int data) {
  *(ram + IX + data) |= 8;
  return 23;
}

// SET 4,(IX+d)
static int op_sb4ixd(int data) {
  *(ram + IX + data) |= 16;
  return 23;
}

// SET 5,(IX+d)
static int op_sb5ixd(int data) {
  *(ram + IX + data) |= 32;
  return 23;
}

// SET 6,(IX+d)
static int op_sb6ixd(int data) {
  *(ram + IX + data) |= 64;
  return 23;
}

// SET 7,(IX+d)
static int op_sb7ixd(int data) {
  *(ram + IX + data) |= 128;
  return 23;
}

// RLC (IX+d)
static int op_rlcixd(int data) {
  int i;
  BYTE *p;

  p = ram + IX + data;
  i = *p & 128;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  F &= ~(H_FLAG | N_FLAG);
  *p <<= 1;
  if (i) *p |= 1;
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[*p]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 23;
}

// RRC (IX+d)
static int op_rrcixd(int data) {
  int i;
  BYTE *p;

  p = ram + IX + data;
  i = *p & 1;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  F &= ~(H_FLAG | N_FLAG);
  *p >>= 1;
  if (i) *p |= 128;
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[*p]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 23;
}

// RL (IX+d)
static int op_rlixd(int data) {
  int old_c_flag;
  BYTE *p;

  p = ram + IX + data;
  old_c_flag = F & C_FLAG;
  (*p & 128) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  *p <<= 1;
  if (old_c_flag) *p |= 1;
  F &= ~(H_FLAG | N_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[*p]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 23;
}

// RR (IX+d)
static int op_rrixd(int data) {
  int old_c_flag;
  BYTE *p;

  old_c_flag = F & C_FLAG;
  p = ram + IX + data;
  (*p & 1) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  *p >>= 1;
  if (old_c_flag) *p |= 128;
  F &= ~(H_FLAG | N_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[*p]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 23;
}

// SLA (IX+d)
static int op_slaixd(int data) {
  BYTE *p;

  p = ram + IX + data;
  (*p & 128) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  *p <<= 1;
  F &= ~(H_FLAG | N_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[*p]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 23;
}

// SRA (IX+d)
static int op_sraixd(int data) {
  int i;
  BYTE *p;

  p = ram + IX + data;
  i = *p & 128;
  (*p & 1) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  *p >>= 1;
  *p |= i;
  F &= ~(H_FLAG | N_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[*p]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 23;
}

// SRL (IX+d)
static int op_srlixd(int data) {
  BYTE *p;

  p = ram + IX + data;
  (*p & 1) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  *p >>= 1;
  F &= ~(H_FLAG | N_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[*p]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 23;
}

int op_ddcb_handler() {
  static int (*op_ddcb[256])() = {
    trap_ddcb,      /* 0x00 */
    trap_ddcb,      /* 0x01 */
    trap_ddcb,      /* 0x02 */
    trap_ddcb,      /* 0x03 */
    trap_ddcb,      /* 0x04 */
    trap_ddcb,      /* 0x05 */
    op_rlcixd,      /* 0x06 */
    trap_ddcb,      /* 0x07 */
    trap_ddcb,      /* 0x08 */
    trap_ddcb,      /* 0x09 */
    trap_ddcb,      /* 0x0a */
    trap_ddcb,      /* 0x0b */
    trap_ddcb,      /* 0x0c */
    trap_ddcb,      /* 0x0d */
    op_rrcixd,      /* 0x0e */
    trap_ddcb,      /* 0x0f */
    trap_ddcb,      /* 0x10 */
    trap_ddcb,      /* 0x11 */
    trap_ddcb,      /* 0x12 */
    trap_ddcb,      /* 0x13 */
    trap_ddcb,      /* 0x14 */
    trap_ddcb,      /* 0x15 */
    op_rlixd,       /* 0x16 */
    trap_ddcb,      /* 0x17 */
    trap_ddcb,      /* 0x18 */
    trap_ddcb,      /* 0x19 */
    trap_ddcb,      /* 0x1a */
    trap_ddcb,      /* 0x1b */
    trap_ddcb,      /* 0x1c */
    trap_ddcb,      /* 0x1d */
    op_rrixd,       /* 0x1e */
    trap_ddcb,      /* 0x1f */
    trap_ddcb,      /* 0x20 */
    trap_ddcb,      /* 0x21 */
    trap_ddcb,      /* 0x22 */
    trap_ddcb,      /* 0x23 */
    trap_ddcb,      /* 0x24 */
    trap_ddcb,      /* 0x25 */
    op_slaixd,      /* 0x26 */
    trap_ddcb,      /* 0x27 */
    trap_ddcb,      /* 0x28 */
    trap_ddcb,      /* 0x29 */
    trap_ddcb,      /* 0x2a */
    trap_ddcb,      /* 0x2b */
    trap_ddcb,      /* 0x2c */
    trap_ddcb,      /* 0x2d */
    op_sraixd,      /* 0x2e */
    trap_ddcb,      /* 0x2f */
    trap_ddcb,      /* 0x30 */
    trap_ddcb,      /* 0x31 */
    trap_ddcb,      /* 0x32 */
    trap_ddcb,      /* 0x33 */
    trap_ddcb,      /* 0x34 */
    trap_ddcb,      /* 0x35 */
    trap_ddcb,      /* 0x36 */
    trap_ddcb,      /* 0x37 */
    trap_ddcb,      /* 0x38 */
    trap_ddcb,      /* 0x39 */
    trap_ddcb,      /* 0x3a */
    trap_ddcb,      /* 0x3b */
    trap_ddcb,      /* 0x3c */
    trap_ddcb,      /* 0x3d */
    op_srlixd,      /* 0x3e */
    trap_ddcb,      /* 0x3f */
    trap_ddcb,      /* 0x40 */
    trap_ddcb,      /* 0x41 */
    trap_ddcb,      /* 0x42 */
    trap_ddcb,      /* 0x43 */
    trap_ddcb,      /* 0x44 */
    trap_ddcb,      /* 0x45 */
    op_tb0ixd,      /* 0x46 */
    trap_ddcb,      /* 0x47 */
    trap_ddcb,      /* 0x48 */
    trap_ddcb,      /* 0x49 */
    trap_ddcb,      /* 0x4a */
    trap_ddcb,      /* 0x4b */
    trap_ddcb,      /* 0x4c */
    trap_ddcb,      /* 0x4d */
    op_tb1ixd,      /* 0x4e */
    trap_ddcb,      /* 0x4f */
    trap_ddcb,      /* 0x50 */
    trap_ddcb,      /* 0x51 */
    trap_ddcb,      /* 0x52 */
    trap_ddcb,      /* 0x53 */
    trap_ddcb,      /* 0x54 */
    trap_ddcb,      /* 0x55 */
    op_tb2ixd,      /* 0x56 */
    trap_ddcb,      /* 0x57 */
    trap_ddcb,      /* 0x58 */
    trap_ddcb,      /* 0x59 */
    trap_ddcb,      /* 0x5a */
    trap_ddcb,      /* 0x5b */
    trap_ddcb,      /* 0x5c */
    trap_ddcb,      /* 0x5d */
    op_tb3ixd,      /* 0x5e */
    trap_ddcb,      /* 0x5f */
    trap_ddcb,      /* 0x60 */
    trap_ddcb,      /* 0x61 */
    trap_ddcb,      /* 0x62 */
    trap_ddcb,      /* 0x63 */
    trap_ddcb,      /* 0x64 */
    trap_ddcb,      /* 0x65 */
    op_tb4ixd,      /* 0x66 */
    trap_ddcb,      /* 0x67 */
    trap_ddcb,      /* 0x68 */
    trap_ddcb,      /* 0x69 */
    trap_ddcb,      /* 0x6a */
    trap_ddcb,      /* 0x6b */
    trap_ddcb,      /* 0x6c */
    trap_ddcb,      /* 0x6d */
    op_tb5ixd,      /* 0x6e */
    trap_ddcb,      /* 0x6f */
    trap_ddcb,      /* 0x70 */
    trap_ddcb,      /* 0x71 */
    trap_ddcb,      /* 0x72 */
    trap_ddcb,      /* 0x73 */
    trap_ddcb,      /* 0x74 */
    trap_ddcb,      /* 0x75 */
    op_tb6ixd,      /* 0x76 */
    trap_ddcb,      /* 0x77 */
    trap_ddcb,      /* 0x78 */
    trap_ddcb,      /* 0x79 */
    trap_ddcb,      /* 0x7a */
    trap_ddcb,      /* 0x7b */
    trap_ddcb,      /* 0x7c */
    trap_ddcb,      /* 0x7d */
    op_tb7ixd,      /* 0x7e */
    trap_ddcb,      /* 0x7f */
    trap_ddcb,      /* 0x80 */
    trap_ddcb,      /* 0x81 */
    trap_ddcb,      /* 0x82 */
    trap_ddcb,      /* 0x83 */
    trap_ddcb,      /* 0x84 */
    trap_ddcb,      /* 0x85 */
    op_rb0ixd,      /* 0x86 */
    trap_ddcb,      /* 0x87 */
    trap_ddcb,      /* 0x88 */
    trap_ddcb,      /* 0x89 */
    trap_ddcb,      /* 0x8a */
    trap_ddcb,      /* 0x8b */
    trap_ddcb,      /* 0x8c */
    trap_ddcb,      /* 0x8d */
    op_rb1ixd,      /* 0x8e */
    trap_ddcb,      /* 0x8f */
    trap_ddcb,      /* 0x90 */
    trap_ddcb,      /* 0x91 */
    trap_ddcb,      /* 0x92 */
    trap_ddcb,      /* 0x93 */
    trap_ddcb,      /* 0x94 */
    trap_ddcb,      /* 0x95 */
    op_rb2ixd,      /* 0x96 */
    trap_ddcb,      /* 0x97 */
    trap_ddcb,      /* 0x98 */
    trap_ddcb,      /* 0x99 */
    trap_ddcb,      /* 0x9a */
    trap_ddcb,      /* 0x9b */
    trap_ddcb,      /* 0x9c */
    trap_ddcb,      /* 0x9d */
    op_rb3ixd,      /* 0x9e */
    trap_ddcb,      /* 0x9f */
    trap_ddcb,      /* 0xa0 */
    trap_ddcb,      /* 0xa1 */
    trap_ddcb,      /* 0xa2 */
    trap_ddcb,      /* 0xa3 */
    trap_ddcb,      /* 0xa4 */
    trap_ddcb,      /* 0xa5 */
    op_rb4ixd,      /* 0xa6 */
    trap_ddcb,      /* 0xa7 */
    trap_ddcb,      /* 0xa8 */
    trap_ddcb,      /* 0xa9 */
    trap_ddcb,      /* 0xaa */
    trap_ddcb,      /* 0xab */
    trap_ddcb,      /* 0xac */
    trap_ddcb,      /* 0xad */
    op_rb5ixd,      /* 0xae */
    trap_ddcb,      /* 0xaf */
    trap_ddcb,      /* 0xb0 */
    trap_ddcb,      /* 0xb1 */
    trap_ddcb,      /* 0xb2 */
    trap_ddcb,      /* 0xb3 */
    trap_ddcb,      /* 0xb4 */
    trap_ddcb,      /* 0xb5 */
    op_rb6ixd,      /* 0xb6 */
    trap_ddcb,      /* 0xb7 */
    trap_ddcb,      /* 0xb8 */
    trap_ddcb,      /* 0xb9 */
    trap_ddcb,      /* 0xba */
    trap_ddcb,      /* 0xbb */
    trap_ddcb,      /* 0xbc */
    trap_ddcb,      /* 0xbd */
    op_rb7ixd,      /* 0xbe */
    trap_ddcb,      /* 0xbf */
    trap_ddcb,      /* 0xc0 */
    trap_ddcb,      /* 0xc1 */
    trap_ddcb,      /* 0xc2 */
    trap_ddcb,      /* 0xc3 */
    trap_ddcb,      /* 0xc4 */
    trap_ddcb,      /* 0xc5 */
    op_sb0ixd,      /* 0xc6 */
    trap_ddcb,      /* 0xc7 */
    trap_ddcb,      /* 0xc8 */
    trap_ddcb,      /* 0xc9 */
    trap_ddcb,      /* 0xca */
    trap_ddcb,      /* 0xcb */
    trap_ddcb,      /* 0xcc */
    trap_ddcb,      /* 0xcd */
    op_sb1ixd,      /* 0xce */
    trap_ddcb,      /* 0xcf */
    trap_ddcb,      /* 0xd0 */
    trap_ddcb,      /* 0xd1 */
    trap_ddcb,      /* 0xd2 */
    trap_ddcb,      /* 0xd3 */
    trap_ddcb,      /* 0xd4 */
    trap_ddcb,      /* 0xd5 */
    op_sb2ixd,      /* 0xd6 */
    trap_ddcb,      /* 0xd7 */
    trap_ddcb,      /* 0xd8 */
    trap_ddcb,      /* 0xd9 */
    trap_ddcb,      /* 0xda */
    trap_ddcb,      /* 0xdb */
    trap_ddcb,      /* 0xdc */
    trap_ddcb,      /* 0xdd */
    op_sb3ixd,      /* 0xde */
    trap_ddcb,      /* 0xdf */
    trap_ddcb,      /* 0xe0 */
    trap_ddcb,      /* 0xe1 */
    trap_ddcb,      /* 0xe2 */
    trap_ddcb,      /* 0xe3 */
    trap_ddcb,      /* 0xe4 */
    trap_ddcb,      /* 0xe5 */
    op_sb4ixd,      /* 0xe6 */
    trap_ddcb,      /* 0xe7 */
    trap_ddcb,      /* 0xe8 */
    trap_ddcb,      /* 0xe9 */
    trap_ddcb,      /* 0xea */
    trap_ddcb,      /* 0xeb */
    trap_ddcb,      /* 0xec */
    trap_ddcb,      /* 0xed */
    op_sb5ixd,      /* 0xee */
    trap_ddcb,      /* 0xef */
    trap_ddcb,      /* 0xf0 */
    trap_ddcb,      /* 0xf1 */
    trap_ddcb,      /* 0xf2 */
    trap_ddcb,      /* 0xf3 */
    trap_ddcb,      /* 0xf4 */
    trap_ddcb,      /* 0xf5 */
    op_sb6ixd,      /* 0xf6 */
    trap_ddcb,      /* 0xf7 */
    trap_ddcb,      /* 0xf8 */
    trap_ddcb,      /* 0xf9 */
    trap_ddcb,      /* 0xfa */
    trap_ddcb,      /* 0xfb */
    trap_ddcb,      /* 0xfc */
    trap_ddcb,      /* 0xfd */
    op_sb7ixd,      /* 0xfe */
    trap_ddcb       /* 0xff */
  };

  int d;
  int t;

  d = (char) *PC++;

#ifdef ENABLE_PCC
  // Correct PC overrun.
  if (PC > ram + 65535) PC = ram;
#endif

  // Execute next opcode.
  t = (*op_ddcb[*PC++])(d);

#ifdef ENABLE_PCC
  // Correct PC overrun.
  if (PC > ram + 65535) PC = ram;
#endif

  return t;
}

