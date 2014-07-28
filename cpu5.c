//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
// Modified for RC700 simulator by Michael Ringgaard
//
// Emulation of multi byte opcodes starting with 0xfd
//

#include "cpu.h"

int op_fdcb_handler();

// Trap for illegal 0xfd multi byte opcodes.
static int trap_fd() {
  cpu_error = OPTRAP2;
  cpu_state = STOPPED;
  return 0;
}

// POP IY
static int op_popiy() {
  CHECK_STACK_UNDERRUN();
  IY = *STACK++;
  CHECK_STACK_UNDERRUN();
  IY += *STACK++ << 8;
  return 14;
}

// PUSH IY
static int op_pusiy() {
  CHECK_STACK_UNDERRUN();
  *--STACK = IY >> 8;
  CHECK_STACK_UNDERRUN();
  *--STACK = IY;
  return 15;
}

// JP (IY)
static int op_jpiy() {
  PC = ram + IY;
  return 8;
}

// EX (SP),IY
static int op_exspy() {
  int i;

  i = *STACK + (*(STACK + 1) << 8);
  *STACK = IY;
  *(STACK + 1) = IY >> 8;
  IY = i;
  return 23;
}

// LD SP,IY
static int op_ldspy() {
  STACK = ram + IY;
  return 10;
}

// LD IY,nn
static int op_ldiynn() {
  IY = *PC++;
  IY += *PC++ << 8;
  return 14;
}

// LD IY,(nn)
static int op_ldiyinn() {
  BYTE *p;

  p = ram + *PC++;
  p += *PC++ << 8;
  IY = *p++;
  IY += *p << 8;
  return 20;
}

// LD (nn),IY
static int op_ldiny() {
  BYTE *p;

  p = ram + *PC++;
  p += *PC++ << 8;
  *p++ = IY;
  *p = IY >> 8;
  return 20;
}

// ADD A,(IY+d)
static int op_adayd() {
  int i;
  BYTE P;

  P = *(ram + IY + (signed char) *PC++);
  ((A & 0xf) + (P & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 19;
}

// ADC A,(IY+d)
static int op_acayd() {
  int i, carry;
  BYTE P;

  carry = (F & C_FLAG) ? 1 : 0;
  P = *(ram + IY + (signed char) *PC++);
  ((A & 0xf) + (P & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) P + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 19;
}

// SUB A,(IY+d)
static int op_suayd() {
  int i;
  BYTE P;

  P = *(ram + IY + (signed char) *PC++);
  ((P & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 19;
}

// SBC A,(IY+d)
static int op_scayd() {
  int i, carry;
  BYTE P;

  carry = (F & C_FLAG) ? 1 : 0;
  P = *(ram + IY + (signed char) *PC++);
  ((P & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) P - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 19;
}

// AND (IY+d)
static int op_andyd() {
  A &= *(ram + IY + (signed char) *PC++);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 19;
}

// XOR (IY+d)
static int op_xoryd() {
  A ^= *(ram + IY + (signed char) *PC++);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 19;
}

// OR (IY+d)
static int op_oryd() {
  A |= *(ram + IY + (signed char) *PC++);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 19;
}

// CP (IY+d)
static int op_cpyd() {
  int i;
  BYTE P;

  P = *(ram + IY + (signed char) *PC++);
  ((P & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 19;
}

// INC (IY+d)
static int op_incyd() {
  BYTE *p;

  p = ram + IY + (signed char) *PC++;
  ((*p & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (*p)++;
  (*p == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 23;
}

// DEC (IY+d)
static int op_decyd() {
  BYTE *p;

  p = ram + IY + (signed char) *PC++;
  (((*p - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (*p)--;
  (*p == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 23;
}

// ADD IY,BC
static int op_addyb() {
  int carry;
  BYTE iyl = IY & 0xff;
  BYTE iyh = IY >> 8;
  
  carry = (iyl + C > 255) ? 1 : 0;
  iyl += C;
  ((iyh & 0xf) + (B & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (iyh + B + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  iyh += B + carry;
  IY = (iyh << 8) + iyl;
  F &= ~N_FLAG;
  return 15;
}

// ADD IY,DE
static int op_addyd() {
  int carry;
  BYTE iyl = IY & 0xff;
  BYTE iyh = IY >> 8;
  
  carry = (iyl + E > 255) ? 1 : 0;
  iyl += E;
  ((iyh & 0xf) + (D & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (iyh + D + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  iyh += D + carry;
  IY = (iyh << 8) + iyl;
  F &= ~N_FLAG;
  return 15;
}

// ADD IY,SP
static int op_addys() {
  int carry;
  BYTE iyl = IY & 0xff;
  BYTE iyh = IY >> 8;
  BYTE spl = (STACK - ram) & 0xff;
  BYTE sph = (STACK - ram) >> 8;
  
  carry = (iyl + spl > 255) ? 1 : 0;
  iyl += spl;
  ((iyh & 0xf) + (sph & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (iyh + sph + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  iyh += sph + carry;
  IY = (iyh << 8) + iyl;
  F &= ~N_FLAG;
  return 15;
}

// ADD IY,IY
static int op_addyy() {
  int carry;
  BYTE iyl = IY & 0xff;
  BYTE iyh = IY >> 8;
  
  carry = (iyl << 1 > 255) ? 1 : 0;
  iyl <<= 1;
  ((iyh & 0xf) + (iyh & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (iyh + iyh + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  iyh += iyh + carry;
  IY = (iyh << 8) + iyl;
  F &= ~N_FLAG;
  return 15;
}

// INC IY
static int op_inciy() {
  IY++;
  return 10;
}

// DEC IY
static int op_deciy() {
  IY--;
  return 10;
}

// LD A,(IY+d)
static int op_ldayd() {
  A = *(IY + (signed char) *PC++ + ram);
  return 19;
}

// LD B,(IY+d)
static int op_ldbyd() {
  B = *(IY + (signed char) *PC++ + ram);
  return 19;
}

// LD C,(IY+d)
static int op_ldcyd() {
  C = *(IY + (signed char) *PC++ + ram);
  return 19;
}

// LD D,(IY+d)
static int op_lddyd() {
  D = *(IY + (signed char) *PC++ + ram);
  return 19;
}

// LD E,(IY+d)
static int op_ldeyd() {
  E = *(IY + (signed char) *PC++ + ram);
  return 19;
}

// LD H,(IY+d)
static int op_ldhyd() {
  H = *(IY + (signed char) *PC++ + ram);
  return 19;
}

// LD L,(IY+d)
static int op_ldlyd() {
  L = *(IY + (signed char) *PC++ + ram);
  return 19;
}

// LD (IY+d),A
static int op_ldyda() {
  *(IY + (signed char) *PC++ + ram) = A;
  return 19;
}

// LD (IY+d),B
static int op_ldydb() {
  *(IY + (signed char) *PC++ + ram) = B;
  return 19;
}

// LD (IY+d),C
static int op_ldydc() {
  *(IY + (signed char) *PC++ + ram) = C;
  return 19;
}

// LD (IY+d),D
static int op_ldydd() {
  *(IY + (signed char) *PC++ + ram) = D;
  return 19;
}

// LD (IY+d),E
static int op_ldyde() {
  *(IY + (signed char) *PC++ + ram) = E;
  return 19;
}

// LD (IY+d),H
static int op_ldydh() {
  *(IY + (signed char) *PC++ + ram) = H;
  return 19;
}

// LD (IY+d),L
static int op_ldydl() {
  *(IY + (signed char) *PC++ + ram) = L;
  return 19;
}

// LD (IY+d),n
static int op_ldydn() {
  int d;

  d = (signed char) *PC++;
  *(IY + d + ram) = *PC++;
  return 19;
}

int op_fd_handler() {
  int t;

  static int (*op_fd[256])() = {
    trap_fd,         /* 0x00 */
    trap_fd,         /* 0x01 */
    trap_fd,         /* 0x02 */
    trap_fd,         /* 0x03 */
    trap_fd,         /* 0x04 */
    trap_fd,         /* 0x05 */
    trap_fd,         /* 0x06 */
    trap_fd,         /* 0x07 */
    trap_fd,         /* 0x08 */
    op_addyb,        /* 0x09 */
    trap_fd,         /* 0x0a */
    trap_fd,         /* 0x0b */
    trap_fd,         /* 0x0c */
    trap_fd,         /* 0x0d */
    trap_fd,         /* 0x0e */
    trap_fd,         /* 0x0f */
    trap_fd,         /* 0x10 */
    trap_fd,         /* 0x11 */
    trap_fd,         /* 0x12 */
    trap_fd,         /* 0x13 */
    trap_fd,         /* 0x14 */
    trap_fd,         /* 0x15 */
    trap_fd,         /* 0x16 */
    trap_fd,         /* 0x17 */
    trap_fd,         /* 0x18 */
    op_addyd,        /* 0x19 */
    trap_fd,         /* 0x1a */
    trap_fd,         /* 0x1b */
    trap_fd,         /* 0x1c */
    trap_fd,         /* 0x1d */
    trap_fd,         /* 0x1e */
    trap_fd,         /* 0x1f */
    trap_fd,         /* 0x20 */
    op_ldiynn,       /* 0x21 */
    op_ldiny,        /* 0x22 */
    op_inciy,        /* 0x23 */
    trap_fd,         /* 0x24 */
    trap_fd,         /* 0x25 */
    trap_fd,         /* 0x26 */
    trap_fd,         /* 0x27 */
    trap_fd,         /* 0x28 */
    op_addyy,        /* 0x29 */
    op_ldiyinn,      /* 0x2a */
    op_deciy,        /* 0x2b */
    trap_fd,         /* 0x2c */
    trap_fd,         /* 0x2d */
    trap_fd,         /* 0x2e */
    trap_fd,         /* 0x2f */
    trap_fd,         /* 0x30 */
    trap_fd,         /* 0x31 */
    trap_fd,         /* 0x32 */
    trap_fd,         /* 0x33 */
    op_incyd,        /* 0x34 */
    op_decyd,        /* 0x35 */
    op_ldydn,        /* 0x36 */
    trap_fd,         /* 0x37 */
    trap_fd,         /* 0x38 */
    op_addys,        /* 0x39 */
    trap_fd,         /* 0x3a */
    trap_fd,         /* 0x3b */
    trap_fd,         /* 0x3c */
    trap_fd,         /* 0x3d */
    trap_fd,         /* 0x3e */
    trap_fd,         /* 0x3f */
    trap_fd,         /* 0x40 */
    trap_fd,         /* 0x41 */
    trap_fd,         /* 0x42 */
    trap_fd,         /* 0x43 */
    trap_fd,         /* 0x44 */
    trap_fd,         /* 0x45 */
    op_ldbyd,        /* 0x46 */
    trap_fd,         /* 0x47 */
    trap_fd,         /* 0x48 */
    trap_fd,         /* 0x49 */
    trap_fd,         /* 0x4a */
    trap_fd,         /* 0x4b */
    trap_fd,         /* 0x4c */
    trap_fd,         /* 0x4d */
    op_ldcyd,        /* 0x4e */
    trap_fd,         /* 0x4f */
    trap_fd,         /* 0x50 */
    trap_fd,         /* 0x51 */
    trap_fd,         /* 0x52 */
    trap_fd,         /* 0x53 */
    trap_fd,         /* 0x54 */
    trap_fd,         /* 0x55 */
    op_lddyd,        /* 0x56 */
    trap_fd,         /* 0x57 */
    trap_fd,         /* 0x58 */
    trap_fd,         /* 0x59 */
    trap_fd,         /* 0x5a */
    trap_fd,         /* 0x5b */
    trap_fd,         /* 0x5c */
    trap_fd,         /* 0x5d */
    op_ldeyd,        /* 0x5e */
    trap_fd,         /* 0x5f */
    trap_fd,         /* 0x60 */
    trap_fd,         /* 0x61 */
    trap_fd,         /* 0x62 */
    trap_fd,         /* 0x63 */
    trap_fd,         /* 0x64 */
    trap_fd,         /* 0x65 */
    op_ldhyd,        /* 0x66 */
    trap_fd,         /* 0x67 */
    trap_fd,         /* 0x68 */
    trap_fd,         /* 0x69 */
    trap_fd,         /* 0x6a */
    trap_fd,         /* 0x6b */
    trap_fd,         /* 0x6c */
    trap_fd,         /* 0x6d */
    op_ldlyd,        /* 0x6e */
    trap_fd,         /* 0x6f */
    op_ldydb,        /* 0x70 */
    op_ldydc,        /* 0x71 */
    op_ldydd,        /* 0x72 */
    op_ldyde,        /* 0x73 */
    op_ldydh,        /* 0x74 */
    op_ldydl,        /* 0x75 */
    trap_fd,         /* 0x76 */
    op_ldyda,        /* 0x77 */
    trap_fd,         /* 0x78 */
    trap_fd,         /* 0x79 */
    trap_fd,         /* 0x7a */
    trap_fd,         /* 0x7b */
    trap_fd,         /* 0x7c */
    trap_fd,         /* 0x7d */
    op_ldayd,        /* 0x7e */
    trap_fd,         /* 0x7f */
    trap_fd,         /* 0x80 */
    trap_fd,         /* 0x81 */
    trap_fd,         /* 0x82 */
    trap_fd,         /* 0x83 */
    trap_fd,         /* 0x84 */
    trap_fd,         /* 0x85 */
    op_adayd,        /* 0x86 */
    trap_fd,         /* 0x87 */
    trap_fd,         /* 0x88 */
    trap_fd,         /* 0x89 */
    trap_fd,         /* 0x8a */
    trap_fd,         /* 0x8b */
    trap_fd,         /* 0x8c */
    trap_fd,         /* 0x8d */
    op_acayd,        /* 0x8e */
    trap_fd,         /* 0x8f */
    trap_fd,         /* 0x90 */
    trap_fd,         /* 0x91 */
    trap_fd,         /* 0x92 */
    trap_fd,         /* 0x93 */
    trap_fd,         /* 0x94 */
    trap_fd,         /* 0x95 */
    op_suayd,        /* 0x96 */
    trap_fd,         /* 0x97 */
    trap_fd,         /* 0x98 */
    trap_fd,         /* 0x99 */
    trap_fd,         /* 0x9a */
    trap_fd,         /* 0x9b */
    trap_fd,         /* 0x9c */
    trap_fd,         /* 0x9d */
    op_scayd,        /* 0x9e */
    trap_fd,         /* 0x9f */
    trap_fd,         /* 0xa0 */
    trap_fd,         /* 0xa1 */
    trap_fd,         /* 0xa2 */
    trap_fd,         /* 0xa3 */
    trap_fd,         /* 0xa4 */
    trap_fd,         /* 0xa5 */
    op_andyd,        /* 0xa6 */
    trap_fd,         /* 0xa7 */
    trap_fd,         /* 0xa8 */
    trap_fd,         /* 0xa9 */
    trap_fd,         /* 0xaa */
    trap_fd,         /* 0xab */
    trap_fd,         /* 0xac */
    trap_fd,         /* 0xad */
    op_xoryd,        /* 0xae */
    trap_fd,         /* 0xaf */
    trap_fd,         /* 0xb0 */
    trap_fd,         /* 0xb1 */
    trap_fd,         /* 0xb2 */
    trap_fd,         /* 0xb3 */
    trap_fd,         /* 0xb4 */
    trap_fd,         /* 0xb5 */
    op_oryd,         /* 0xb6 */
    trap_fd,         /* 0xb7 */
    trap_fd,         /* 0xb8 */
    trap_fd,         /* 0xb9 */
    trap_fd,         /* 0xba */
    trap_fd,         /* 0xbb */
    trap_fd,         /* 0xbc */
    trap_fd,         /* 0xbd */
    op_cpyd,         /* 0xbe */
    trap_fd,         /* 0xbf */
    trap_fd,         /* 0xc0 */
    trap_fd,         /* 0xc1 */
    trap_fd,         /* 0xc2 */
    trap_fd,         /* 0xc3 */
    trap_fd,         /* 0xc4 */
    trap_fd,         /* 0xc5 */
    trap_fd,         /* 0xc6 */
    trap_fd,         /* 0xc7 */
    trap_fd,         /* 0xc8 */
    trap_fd,         /* 0xc9 */
    trap_fd,         /* 0xca */
    op_fdcb_handler, /* 0xcb */
    trap_fd,         /* 0xcc */
    trap_fd,         /* 0xcd */
    trap_fd,         /* 0xce */
    trap_fd,         /* 0xcf */
    trap_fd,         /* 0xd0 */
    trap_fd,         /* 0xd1 */
    trap_fd,         /* 0xd2 */
    trap_fd,         /* 0xd3 */
    trap_fd,         /* 0xd4 */
    trap_fd,         /* 0xd5 */
    trap_fd,         /* 0xd6 */
    trap_fd,         /* 0xd7 */
    trap_fd,         /* 0xd8 */
    trap_fd,         /* 0xd9 */
    trap_fd,         /* 0xda */
    trap_fd,         /* 0xdb */
    trap_fd,         /* 0xdc */
    trap_fd,         /* 0xdd */
    trap_fd,         /* 0xde */
    trap_fd,         /* 0xdf */
    trap_fd,         /* 0xe0 */
    op_popiy,        /* 0xe1 */
    trap_fd,         /* 0xe2 */
    op_exspy,        /* 0xe3 */
    trap_fd,         /* 0xe4 */
    op_pusiy,        /* 0xe5 */
    trap_fd,         /* 0xe6 */
    trap_fd,         /* 0xe7 */
    trap_fd,         /* 0xe8 */
    op_jpiy,         /* 0xe9 */
    trap_fd,         /* 0xea */
    trap_fd,         /* 0xeb */
    trap_fd,         /* 0xec */
    trap_fd,         /* 0xed */
    trap_fd,         /* 0xee */
    trap_fd,         /* 0xef */
    trap_fd,         /* 0xf0 */
    trap_fd,         /* 0xf1 */
    trap_fd,         /* 0xf2 */
    trap_fd,         /* 0xf3 */
    trap_fd,         /* 0xf4 */
    trap_fd,         /* 0xf5 */
    trap_fd,         /* 0xf6 */
    trap_fd,         /* 0xf7 */
    trap_fd,         /* 0xf8 */
    op_ldspy,        /* 0xf9 */
    trap_fd,         /* 0xfa */
    trap_fd,         /* 0xfb */
    trap_fd,         /* 0xfc */
    trap_fd,         /* 0xfd */
    trap_fd,         /* 0xfe */
    trap_fd          /* 0xff */
  };

  // Execute next opcode.
  t = (*op_fd[*PC++])();

#ifdef ENABLE_PCC
  // Correct PC overrun.
  if (PC > ram + 65535) PC = ram;
#endif

  return t;
}

