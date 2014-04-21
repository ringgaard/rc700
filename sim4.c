//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
//
// Emulation of multi byte opcodes starting with 0xed
//

#include <stdio.h>

#include "sim.h"
#include "simglb.h"

// Trap for illegal 0xed multi byte opcodes.
static int trap_ed() {
  cpu_error = OPTRAP2;
  cpu_state = STOPPED;
  return 0;
}

// IM 0
static int op_im0() {
  int_mode = 0;
  return 8;
}

// IM 1
static int op_im1() {
  int_mode = 1;
  return 8;
}

// IM 2
static int op_im2() {
  int_mode = 2;
  return 8;
}

// RETI
static int op_reti() {
  unsigned i;

  i = *STACK++;
  CHECK_STACK_OVERRUN();
  i += *STACK++ << 8;
  CHECK_STACK_OVERRUN();
  PC = ram + i;
  genintr();
  return 14;
}

// RETN
static int op_retn() {
  unsigned i;

  i = *STACK++;
  CHECK_STACK_OVERRUN();
  i += *STACK++ << 8;
  CHECK_STACK_OVERRUN();
  PC = ram + i;
  if (IFF & 2) IFF |= 1;
  return 14;
}

// NEG
static int op_neg() {
  (A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (A == 0x80) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (0 - ((char) A & 0xf) < 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A = 0 - A;
  F |= N_FLAG;
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 8;
}

// IN A,(C)
static int op_inaic() {
  BYTE io_in();

  A = io_in(C);
  F &= ~(N_FLAG | H_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 12;
}

// IN B,(C)
static int op_inbic() {
  BYTE io_in();

  B = io_in(C);
  F &= ~(N_FLAG | H_FLAG);
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[B]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 12;
}

// IN C,(C)
static int op_incic() {
  BYTE io_in();

  C = io_in(C);
  F &= ~(N_FLAG | H_FLAG);
  (C) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (C & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[C]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 12;
}

// IN D,(C)
static int op_indic() {
  BYTE io_in();

  D = io_in(C);
  F &= ~(N_FLAG | H_FLAG);
  (D) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (D & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[D]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 12;
}

// IN E,(C)
static int op_ineic() {
  BYTE io_in();

  E = io_in(C);
  F &= ~(N_FLAG | H_FLAG);
  (E) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (E & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[E]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 12;
}

// IN H,(C)
static int op_inhic() {
  BYTE io_in();

  H = io_in(C);
  F &= ~(N_FLAG | H_FLAG);
  (H) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[H]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 12;
}

// IN L,(C)
static int op_inlic() {
  BYTE io_in();

  L = io_in(C);
  F &= ~(N_FLAG | H_FLAG);
  (L) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (L & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[L]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 12;
}

// OUT (C),A
static int op_outca() {
  BYTE io_out();

  io_out(C, A);
  return 12;
}

// OUT (C),B
static int op_outcb() {
  BYTE io_out();

  io_out(C, B);
  return 12;
}

// OUT (C),C
static int op_outcc() {
  BYTE io_out();

  io_out(C, C);
  return 12;
}

// OUT (C),D
static int op_outcd() {
  BYTE io_out();

  io_out(C, D);
  return 12;
}

// OUT (C),E
static int op_outce() {
  BYTE io_out();

  io_out(C, E);
  return 12;
}

// OUT (C),H
static int op_outch() {
  BYTE io_out();

  io_out(C, H);
  return 12;
}

// OUT (C),L
static int op_outcl() {
  BYTE io_out();

  io_out(C, L);
  return 12;
}

// INI
static int op_ini() {
  BYTE io_in();

  *(ram + (H << 8) + L) = io_in(C);
  L++;
  if (!L) H++;
  B--;
  F |= N_FLAG;
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return 16;
}

// INIR
static int op_inir() {
  int t  = -21;
  BYTE *d;
  BYTE io_in();

  d = ram + (H << 8) + L;
  do {
    *d++ = io_in(C);
    B--;
    t += 21;
  } while (B);
  H = (d - ram) >> 8;
  L = d - ram;
  F |= N_FLAG | Z_FLAG;
  return t + 16;
}

// IND
static int op_ind() {
  BYTE io_in();

  *(ram + (H << 8) + L) = io_in(C);
  L--;
  if (L == 0xff) H--;
  B--;
  F |= N_FLAG;
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return 16;
}

// INDR
static int op_indr() {
  int t  = -21;
  BYTE *d;
  BYTE io_in();

  d = ram + (H << 8) + L;
  do {
    *d-- = io_in(C);
    B--;
    t += 21;
  } while (B);
  H = (d - ram) >> 8;
  L = d - ram;
  F |= N_FLAG | Z_FLAG;
  return t + 16;
}

// OUTI
static int op_outi() {
  BYTE io_out();

  io_out(C, *(ram + (H << 8) * L));
  L++;
  if (!L) H++;
  B--;
  F |= N_FLAG;
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return 16;
}

// OTIR
static int op_otir() {
  int t  = -21;
  BYTE *d;
  BYTE io_out();

  d = ram + (H << 8) + L;
  do {
    io_out(C, *d++);
    B--;
    t += 21;
  } while (B);
  H = (d - ram) >> 8;
  L = d - ram;
  F |= N_FLAG | Z_FLAG;
  return t + 16;
}

// OUTD
static int op_outd() {
  BYTE io_out();

  io_out(C, *(ram + (H << 8) * L));
  L--;
  if (L == 0xff) H--;
  B--;
  F |= N_FLAG;
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return 16;
}

// OTDR
static int op_otdr() {
  int t  = -21;
  BYTE *d;
  BYTE io_out();

  d = ram + (H << 8) + L;
  do {
    io_out(C, *d--);
    B--;
    t += 21;
  } while (B);
  H = (d - ram) >> 8;
  L = d - ram;
  F |= N_FLAG | Z_FLAG;
  return t + 16;
}

// LD A,I
static int op_ldai() {
  A = I;
  F &= ~(N_FLAG | H_FLAG);
  (IFF & 2) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 9;
}

// LD A,R
static int op_ldar() {
  A = (BYTE) R;
  F &= ~(N_FLAG | H_FLAG);
  (IFF & 2) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 9;
}

// LD I,A
static int op_ldia() {
  I = A;
  return 9;
}

// LD R,A
static int op_ldra() {
  R = A;
  return 9;
}

// LD BC,(nn)
static int op_ldbcinn() {
  BYTE *p;

  p = ram + *PC++;
  p += *PC++ << 8;
  C = *p++;
  B = *p;
  return 20;
}

// LD DE,(nn)
static int op_lddeinn() {
  BYTE *p;

  p = ram + *PC++;
  p += *PC++ << 8;
  E = *p++;
  D = *p;
  return 20;
}

// LD SP,(nn)
static int op_ldspinn() {
  BYTE *p;

  p = ram + *PC++;
  p += *PC++ << 8;
  STACK = ram + *p++;
  STACK += *p << 8;
  return 20;
}

// LD (nn),BC
static int op_ldinbc() {
  BYTE *p;

  p = ram + *PC++;
  p += *PC++ << 8;
  *p++ = C;
  *p = B;
  return 20;
}

// LD (nn),DE
static int op_ldinde() {
  BYTE *p;

  p = ram + *PC++;
  p += *PC++ << 8;
  *p++ = E;
  *p = D;
  return 20;
}

// LD (nn),SP
static int op_ldinsp() {
  BYTE *p;
  int i;

  p = ram + *PC++;
  p += *PC++ << 8;
  i = STACK - ram;
  *p++ = i;
  *p = i >> 8;
  return 20;
}

// ADC HL,BC
static int op_adchb() {
  int carry;
  int lcarry;
  WORD hl, bc;
  long i;

  carry = (F & C_FLAG) ? 1 : 0;
  lcarry = (L + C > 255) ? 1 : 0;
  ((H & 0xf) + (B & 0xf) + carry + lcarry > 0xf) ? (F |= H_FLAG)
                   : (F &= ~H_FLAG);
  hl = (H << 8) + L;
  bc = (B << 8) + C;
  i = ((long)hl) + ((long)bc) + carry;
  ((hl < 0x8000) && (i > 0x7fffL)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i > 0xffffL) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  H = i >> 8;
  L = i;
  F &= ~N_FLAG;
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 15;
}

// ADC HL,DE
static int op_adchd() {
  int carry;
  int lcarry;
  WORD hl, de;
  long i;

  carry = (F & C_FLAG) ? 1 : 0;
  lcarry = (L + E > 255) ? 1 : 0;
  ((H & 0xf) + (D & 0xf) + carry + lcarry > 0xf) ? (F |= H_FLAG)
                   : (F &= ~H_FLAG);
  hl = (H << 8) + L;
  de = (D << 8) + E;
  i = ((long)hl) + ((long)de) + carry;
  ((hl < 0x8000) && (i > 0x7fffL)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i > 0xffffL) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  H = i >> 8;
  L = i;
  F &= ~N_FLAG;
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 15;
}

// ADC HL,HL
static int op_adchh() {
  int carry;
  int lcarry;
  WORD hl;
  long i;

  carry = (F & C_FLAG) ? 1 : 0;
  lcarry = (L + L > 255) ? 1 : 0;
  ((H & 0xf) + (H & 0xf) + carry + lcarry > 0xf) ? (F |= H_FLAG)
                   : (F &= ~H_FLAG);
  hl = (H << 8) + L;
  i = ((((long)hl) << 1) + carry);
  ((hl < 0x8000) && (i > 0x7fffL)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i > 0xffffL) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  H = i >> 8;
  L = i;
  F &= ~N_FLAG;
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 15;
}

// ADC HL,SP
static int op_adchs() {
  int carry;
  int lcarry;
  WORD hl, sp;
  long i;

  carry = (F & C_FLAG) ? 1 : 0;
  hl = (H << 8) + L;
  sp = STACK - ram;
  lcarry = (L + (sp & 0xff) > 255) ? 1 : 0;
  ((H & 0xf) + ((sp >> 8) & 0xf) + carry + lcarry > 0xf) ? (F |= H_FLAG)
                     : (F &= ~H_FLAG);
  i = ((long)hl) + ((long)sp) + carry;
  ((hl < 0x8000) && (i > 0x7fffL)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i > 0xffffL) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  H = i >> 8;
  L = i;
  F &= ~N_FLAG;
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 15;
}

// SBC HL,BC
static int op_sbchb() {
  int carry;
  int lcarry;
  WORD hl, bc;
  long i;

  carry = (F & C_FLAG) ? 1 : 0;
  lcarry = (C > L) ? 1 : 0;
  ((B & 0xf) + carry + lcarry > (H & 0xf)) ? (F |= H_FLAG)
             : (F &= ~H_FLAG);
  hl = (H << 8) + L;
  bc = (B << 8) + C;
  i = ((long)hl) - ((long)bc) - carry;
  ((hl > 0x7fff) && (i < 0x8000L)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i < 0L) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  H = i >> 8;
  L = i;
  F |= N_FLAG;
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 15;
}

// SBC HL,DE
static int op_sbchd() {
  int carry;
  int lcarry;
  WORD hl, de;
  long i;

  carry = (F & C_FLAG) ? 1 : 0;
  lcarry = (E > L) ? 1 : 0;
  ((D & 0xf) + carry + lcarry > (H & 0xf)) ? (F |= H_FLAG)
             : (F &= ~H_FLAG);
  hl = (H << 8) + L;
  de = (D << 8) + E;
  i = ((long)hl) - ((long)de) - carry;
  ((hl > 0x7fff) && (i < 0x8000L)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i < 0L) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  H = i >> 8;
  L = i;
  F |= N_FLAG;
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 15;
}

// SBC HL,HL
static int op_sbchh() {
  if (F & C_FLAG) {
    F |= S_FLAG | P_FLAG | N_FLAG | C_FLAG | H_FLAG;
    F &= ~Z_FLAG;
    H = L = 255;
  } else {
    F |= Z_FLAG | N_FLAG;
    F &= ~(S_FLAG | P_FLAG | C_FLAG | H_FLAG);
    H = L = 0;
  }
  return 15;
}

// SBC HL,SP
static int op_sbchs() {
  int carry;
  int lcarry;
  WORD hl, sp;
  long i;

  carry = (F & C_FLAG) ? 1 : 0;
  hl = (H << 8) + L;
  sp = STACK - ram;
  lcarry = ((sp & 0xff) > L) ? 1 : 0;
  (((sp >> 8) & 0xf) + carry + lcarry > (H & 0xf)) ? (F |= H_FLAG)
               : (F &= ~H_FLAG);
  i = ((long)hl) - ((long)sp) - carry;
  ((hl > 0x7fff) && (i < 0x8000L)) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i < 0L) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  H = i >> 8;
  L = i;
  F |= N_FLAG;
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 15;
}

// LDI
static int op_ldi() {
  *(ram + (D << 8) + E) = *(ram + (H << 8) + L);
  E++;
  if (!E) D++;
  L++;
  if (!L) H++;
  C--;
  if (C == 0xff) B--;
  (B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  F &= ~(N_FLAG | H_FLAG);
  return 16;
}

// LDIR
static int op_ldir() {
  int t  = -21;
  WORD i;
  BYTE *s, *d;

  i = (B << 8) + C;
  d = ram + (D << 8) + E;
  s = ram + (H << 8) + L;
  do {
    *d++ = *s++;
    t += 21;
  } while (--i);
  B = C = 0;
  D = (d - ram) >> 8;
  E = d - ram;
  H = (s - ram) >> 8;
  L = s - ram;
  F &= ~(N_FLAG | P_FLAG | H_FLAG);
  return t + 16;
}

// LDD
static int op_ldd() {
  *(ram + (D << 8) + E) = *(ram + (H << 8) + L);
  E--;
  if (E == 0xff) D--;
  L--;
  if (L == 0xff) H--;
  C--;
  if (C == 0xff) B--;
  (B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  F &= ~(N_FLAG | H_FLAG);
  return 16;
}

// LDDR
static int op_lddr() {
  int t  = -21;
  WORD i;
  BYTE *s, *d;

  i = (B << 8) + C;
  d = ram + (D << 8) + E;
  s = ram + (H << 8) + L;
  do {
    *d-- = *s--;
    t += 21;
  } while (--i);
  B = C = 0;
  D = (d - ram) >> 8;
  E = d - ram;
  H = (s - ram) >> 8;
  L = s - ram;
  F &= ~(N_FLAG | P_FLAG | H_FLAG);
  return t + 16;
}

// CPI
static int op_cpi() {
  BYTE i;

  i = *(ram + ((H << 8) + L));
  ((i & 0xf) > (A & 0xF)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  i = A - i;
  L++;
  if (!L) H++;
  C--;
  if (C == 0xff)  B--;
  F |= N_FLAG;
  (B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 16;
}

// CPIR
static int op_cpir() {
  // H Flag not set!!!
  int t  = -21;
  BYTE *s;
  BYTE d;
  WORD i;

  i = (B << 8) + C;
  s = ram + (H << 8) + L;
  do {
    d = A - *s++;
    t += 21;
  } while (--i && d);
  F |= N_FLAG;
  B = i >> 8;
  C = i;
  H = (s - ram) >> 8;
  L = s - ram;
  (i) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (d) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (d & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return t + 16;
}

// CPD
static int op_cpdop() {
  BYTE i;

  i = *(ram + ((H << 8) + L));
  ((i & 0xf) > (A & 0xF)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  i = A - i;
  L--;
  if (L == 0xff) H--;
  C--;
  if (C == 0xff) B--;
  F |= N_FLAG;
  (B | C) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return 16;
}

// CPDR
static int op_cpdr() {
  // H Flag not set!!!
  int t  = -21;
  BYTE *s;
  BYTE d;
  WORD i;

  i = (B << 8) + C;
  s = ram + (H << 8) + L;
  do {
    d = A - *s--;
    t += 21;
  } while (--i && d);
  F |= N_FLAG;
  B = i >> 8;
  C = i;
  H = (s - ram) >> 8;
  L = s - ram;
  (i) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (d) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (d & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  return t + 16;
}

// RLD (HL)
static int op_oprld() {
  int i, j;

  i = *(ram + (H << 8) + L);
  j = A & 0x0f;
  A = (A & 0xf0) | (i >> 4);
  i = (i << 4) | j;
  *(ram + (H << 8) + L) = i;
  F &= ~(H_FLAG | N_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 18;
}

// RRD (HL)
static int op_oprrd() {
  int i, j;

  i = *(ram + (H << 8) + L);
  j = A & 0x0f;
  A = (A & 0xf0) | (i & 0x0f);
  i = (i >> 4) | (j << 4);
  *(ram + (H << 8) + L) = i;
  F &= ~(H_FLAG | N_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return 18;
}

int op_ed_handler() {
  int t;

  static int (*op_ed[256])() = {
    trap_ed,      /* 0x00 */
    trap_ed,      /* 0x01 */
    trap_ed,      /* 0x02 */
    trap_ed,      /* 0x03 */
    trap_ed,      /* 0x04 */
    trap_ed,      /* 0x05 */
    trap_ed,      /* 0x06 */
    trap_ed,      /* 0x07 */
    trap_ed,      /* 0x08 */
    trap_ed,      /* 0x09 */
    trap_ed,      /* 0x0a */
    trap_ed,      /* 0x0b */
    trap_ed,      /* 0x0c */
    trap_ed,      /* 0x0d */
    trap_ed,      /* 0x0e */
    trap_ed,      /* 0x0f */
    trap_ed,      /* 0x10 */
    trap_ed,      /* 0x11 */
    trap_ed,      /* 0x12 */
    trap_ed,      /* 0x13 */
    trap_ed,      /* 0x14 */
    trap_ed,      /* 0x15 */
    trap_ed,      /* 0x16 */
    trap_ed,      /* 0x17 */
    trap_ed,      /* 0x18 */
    trap_ed,      /* 0x19 */
    trap_ed,      /* 0x1a */
    trap_ed,      /* 0x1b */
    trap_ed,      /* 0x1c */
    trap_ed,      /* 0x1d */
    trap_ed,      /* 0x1e */
    trap_ed,      /* 0x1f */
    trap_ed,      /* 0x20 */
    trap_ed,      /* 0x21 */
    trap_ed,      /* 0x22 */
    trap_ed,      /* 0x23 */
    trap_ed,      /* 0x24 */
    trap_ed,      /* 0x25 */
    trap_ed,      /* 0x26 */
    trap_ed,      /* 0x27 */
    trap_ed,      /* 0x28 */
    trap_ed,      /* 0x29 */
    trap_ed,      /* 0x2a */
    trap_ed,      /* 0x2b */
    trap_ed,      /* 0x2c */
    trap_ed,      /* 0x2d */
    trap_ed,      /* 0x2e */
    trap_ed,      /* 0x2f */
    trap_ed,      /* 0x30 */
    trap_ed,      /* 0x31 */
    trap_ed,      /* 0x32 */
    trap_ed,      /* 0x33 */
    trap_ed,      /* 0x34 */
    trap_ed,      /* 0x35 */
    trap_ed,      /* 0x36 */
    trap_ed,      /* 0x37 */
    trap_ed,      /* 0x38 */
    trap_ed,      /* 0x39 */
    trap_ed,      /* 0x3a */
    trap_ed,      /* 0x3b */
    trap_ed,      /* 0x3c */
    trap_ed,      /* 0x3d */
    trap_ed,      /* 0x3e */
    trap_ed,      /* 0x3f */
    op_inbic,     /* 0x40 */
    op_outcb,     /* 0x41 */
    op_sbchb,     /* 0x42 */
    op_ldinbc,    /* 0x43 */
    op_neg,       /* 0x44 */
    op_retn,      /* 0x45 */
    op_im0,       /* 0x46 */
    op_ldia,      /* 0x47 */
    op_incic,     /* 0x48 */
    op_outcc,     /* 0x49 */
    op_adchb,     /* 0x4a */
    op_ldbcinn,   /* 0x4b */
    trap_ed,      /* 0x4c */
    op_reti,      /* 0x4d */
    trap_ed,      /* 0x4e */
    op_ldra,      /* 0x4f */
    op_indic,     /* 0x50 */
    op_outcd,     /* 0x51 */
    op_sbchd,     /* 0x52 */
    op_ldinde,    /* 0x53 */
    trap_ed,      /* 0x54 */
    trap_ed,      /* 0x55 */
    op_im1,       /* 0x56 */
    op_ldai,      /* 0x57 */
    op_ineic,     /* 0x58 */
    op_outce,     /* 0x59 */
    op_adchd,     /* 0x5a */
    op_lddeinn,   /* 0x5b */
    trap_ed,      /* 0x5c */
    trap_ed,      /* 0x5d */
    op_im2,       /* 0x5e */
    op_ldar,      /* 0x5f */
    op_inhic,     /* 0x60 */
    op_outch,     /* 0x61 */
    op_sbchh,     /* 0x62 */
    trap_ed,      /* 0x63 */
    trap_ed,      /* 0x64 */
    trap_ed,      /* 0x65 */
    trap_ed,      /* 0x66 */
    op_oprrd,     /* 0x67 */
    op_inlic,     /* 0x68 */
    op_outcl,     /* 0x69 */
    op_adchh,     /* 0x6a */
    trap_ed,      /* 0x6b */
    trap_ed,      /* 0x6c */
    trap_ed,      /* 0x6d */
    trap_ed,      /* 0x6e */
    op_oprld,     /* 0x6f */
    trap_ed,      /* 0x70 */
    trap_ed,      /* 0x71 */
    op_sbchs,     /* 0x72 */
    op_ldinsp,    /* 0x73 */
    trap_ed,      /* 0x74 */
    trap_ed,      /* 0x75 */
    trap_ed,      /* 0x76 */
    trap_ed,      /* 0x77 */
    op_inaic,     /* 0x78 */
    op_outca,     /* 0x79 */
    op_adchs,     /* 0x7a */
    op_ldspinn,   /* 0x7b */
    trap_ed,      /* 0x7c */
    trap_ed,      /* 0x7d */
    trap_ed,      /* 0x7e */
    trap_ed,      /* 0x7f */
    trap_ed,      /* 0x80 */
    trap_ed,      /* 0x81 */
    trap_ed,      /* 0x82 */
    trap_ed,      /* 0x83 */
    trap_ed,      /* 0x84 */
    trap_ed,      /* 0x85 */
    trap_ed,      /* 0x86 */
    trap_ed,      /* 0x87 */
    trap_ed,      /* 0x88 */
    trap_ed,      /* 0x89 */
    trap_ed,      /* 0x8a */
    trap_ed,      /* 0x8b */
    trap_ed,      /* 0x8c */
    trap_ed,      /* 0x8d */
    trap_ed,      /* 0x8e */
    trap_ed,      /* 0x8f */
    trap_ed,      /* 0x90 */
    trap_ed,      /* 0x91 */
    trap_ed,      /* 0x92 */
    trap_ed,      /* 0x93 */
    trap_ed,      /* 0x94 */
    trap_ed,      /* 0x95 */
    trap_ed,      /* 0x96 */
    trap_ed,      /* 0x97 */
    trap_ed,      /* 0x98 */
    trap_ed,      /* 0x99 */
    trap_ed,      /* 0x9a */
    trap_ed,      /* 0x9b */
    trap_ed,      /* 0x9c */
    trap_ed,      /* 0x9d */
    trap_ed,      /* 0x9e */
    trap_ed,      /* 0x9f */
    op_ldi,       /* 0xa0 */
    op_cpi,       /* 0xa1 */
    op_ini,       /* 0xa2 */
    op_outi,      /* 0xa3 */
    trap_ed,      /* 0xa4 */
    trap_ed,      /* 0xa5 */
    trap_ed,      /* 0xa6 */
    trap_ed,      /* 0xa7 */
    op_ldd,       /* 0xa8 */
    op_cpdop,     /* 0xa9 */
    op_ind,       /* 0xaa */
    op_outd,      /* 0xab */
    trap_ed,      /* 0xac */
    trap_ed,      /* 0xad */
    trap_ed,      /* 0xae */
    trap_ed,      /* 0xaf */
    op_ldir,      /* 0xb0 */
    op_cpir,      /* 0xb1 */
    op_inir,      /* 0xb2 */
    op_otir,      /* 0xb3 */
    trap_ed,      /* 0xb4 */
    trap_ed,      /* 0xb5 */
    trap_ed,      /* 0xb6 */
    trap_ed,      /* 0xb7 */
    op_lddr,      /* 0xb8 */
    op_cpdr,      /* 0xb9 */
    op_indr,      /* 0xba */
    op_otdr,      /* 0xbb */
    trap_ed,      /* 0xbc */
    trap_ed,      /* 0xbd */
    trap_ed,      /* 0xbe */
    trap_ed,      /* 0xbf */
    trap_ed,      /* 0xc0 */
    trap_ed,      /* 0xc1 */
    trap_ed,      /* 0xc2 */
    trap_ed,      /* 0xc3 */
    trap_ed,      /* 0xc4 */
    trap_ed,      /* 0xc5 */
    trap_ed,      /* 0xc6 */
    trap_ed,      /* 0xc7 */
    trap_ed,      /* 0xc8 */
    trap_ed,      /* 0xc9 */
    trap_ed,      /* 0xca */
    trap_ed,      /* 0xcb */
    trap_ed,      /* 0xcc */
    trap_ed,      /* 0xcd */
    trap_ed,      /* 0xce */
    trap_ed,      /* 0xcf */
    trap_ed,      /* 0xd0 */
    trap_ed,      /* 0xd1 */
    trap_ed,      /* 0xd2 */
    trap_ed,      /* 0xd3 */
    trap_ed,      /* 0xd4 */
    trap_ed,      /* 0xd5 */
    trap_ed,      /* 0xd6 */
    trap_ed,      /* 0xd7 */
    trap_ed,      /* 0xd8 */
    trap_ed,      /* 0xd9 */
    trap_ed,      /* 0xda */
    trap_ed,      /* 0xdb */
    trap_ed,      /* 0xdc */
    trap_ed,      /* 0xdd */
    trap_ed,      /* 0xde */
    trap_ed,      /* 0xdf */
    trap_ed,      /* 0xe0 */
    trap_ed,      /* 0xe1 */
    trap_ed,      /* 0xe2 */
    trap_ed,      /* 0xe3 */
    trap_ed,      /* 0xe4 */
    trap_ed,      /* 0xe5 */
    trap_ed,      /* 0xe6 */
    trap_ed,      /* 0xe7 */
    trap_ed,      /* 0xe8 */
    trap_ed,      /* 0xe9 */
    trap_ed,      /* 0xea */
    trap_ed,      /* 0xeb */
    trap_ed,      /* 0xec */
    trap_ed,      /* 0xed */
    trap_ed,      /* 0xee */
    trap_ed,      /* 0xef */
    trap_ed,      /* 0xf0 */
    trap_ed,      /* 0xf1 */
    trap_ed,      /* 0xf2 */
    trap_ed,      /* 0xf3 */
    trap_ed,      /* 0xf4 */
    trap_ed,      /* 0xf5 */
    trap_ed,      /* 0xf6 */
    trap_ed,      /* 0xf7 */
    trap_ed,      /* 0xf8 */
    trap_ed,      /* 0xf9 */
    trap_ed,      /* 0xfa */
    trap_ed,      /* 0xfb */
    trap_ed,      /* 0xfc */
    trap_ed,      /* 0xfd */
    trap_ed,      /* 0xfe */
    trap_ed       /* 0xff */
  };

  // Execute next opcode.
  t = (*op_ed[*PC++])();

#ifdef WANT_PCC
  // Correct PC overrun.
  if (PC > ram + 65535) PC = ram;
#endif

  return t;
}

