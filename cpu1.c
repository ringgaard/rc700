//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
// Modified for RC700 simulator by Michael Ringgaard
//

#include "cpu.h"

int op_cb_handler(), op_dd_handler();
int op_ed_handler(), op_fd_handler();

// Generate CPU interrupt.
void genintr() {
  int *c;
  
  if (IFF == 3) {
    for (c = int_chain; c < int_chain + 8; ++c) {
      if (*c != -1) {
        int_type = INT_INT;
        int_vec = *c;
        *c = -1;
        return;
      }
    }
  }
}

// Interrupt CPU.
void interrupt(int vec, int priority)
{
  if (int_type == INT_NONE && IFF == 3) {
    int_type = INT_INT;
    int_vec = vec;
  } else {
    int_chain[priority] = vec;
  }
}

// Trap for unimplemented opcodes.
static int op_notimpl() {
  cpu_error = OPTRAP1;
  cpu_state = STOPPED;
  return 0;
}

// NOP
static int op_nop() {
  return 4;
}

// HALT
static int op_halt() {
  while (int_type == 0 && cpu_state == CONTIN_RUN) {
    cpu_halt();
    R += 99999;
    cpu_poll(99999);
  }
  return 0;
}

// SCF
static int op_scf() {
  F |= C_FLAG;
  F &= ~(N_FLAG | H_FLAG);
  return 4;
}

// CCF
static int op_ccf() {
  if (F & C_FLAG) {
    F |= H_FLAG;
    F &= ~C_FLAG;
  } else {
    F &= ~H_FLAG;
    F |= C_FLAG;
  }
  F &= ~N_FLAG;
  return 4;
}

// CPL
static int op_cpl() {
  A = ~A;
  F |= H_FLAG | N_FLAG;
  return 4;
}

// DAA
static int op_daa() {
  if (F & N_FLAG) {
    // Subtractions.
    if (((A & 0x0f) > 9) || (F & H_FLAG)) {
      (((A & 0x0f) - 6) < 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A -= 6;
    }
    if (((A & 0xf0) > 0x90) || (F & C_FLAG)) {
      if (((A & 0xf0) - 0x60) < 0) F |= C_FLAG;
      A -= 0x60;
    }
  } else {
    // Additions
    if (((A & 0x0f) > 9) || (F & H_FLAG)) {
      (((A & 0x0f) + 6) > 0x0f) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A += 6;
    }
    if (((A & 0xf0) > 0x90) || (F & C_FLAG)) {
      if (((A & 0xf0) + 0x60) > 0xf0) F |= C_FLAG;
      A += 0x60;
    }
  }
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) :	(F |= P_FLAG);
  return 4;
}

// EI
static int op_ei() {
  IFF = 3;
  genintr();
  return 4;
}

// DI
static int op_di() {
  IFF = 0;
  return 4;
}

// IN A,(n)
static int op_in() {
  A = cpu_in(*PC++);
  return 11;
}

// OUT (n),A
static int op_out() {
  cpu_out(*PC++, A);
  return 11;
}

// LD A,n
static int op_ldan() {
  A = *PC++;
  return 7;
}

// LD B,n
static int op_ldbn() {
  B = *PC++;
  return 7;
}

// LD C,n
static int op_ldcn() {
  C = *PC++;
  return 7;
}

// LD D,n
static int op_lddn() {
  D = *PC++;
  return 7;
}

// LD E,n
static int op_lden() {
  E = *PC++;
  return 7;
}

// LD H,n
static int op_ldhn() {
  H = *PC++;
  return 7;
}

// LD L,n
static int op_ldln() {
  L = *PC++;
  return 7;
}

// LD A,(BC)
static int op_ldabc() {
  A = *(ram + (B << 8) + C);
  return 7;
}

// LD A,(DE)
static int op_ldade() {
  A = *(ram + (D << 8) + E);
  return 7;
}

// LD A,(nn)
static int op_ldann() {
  unsigned i;

  i = *PC++;
  i += *PC++ << 8;
  A = *(ram + i);
  return 13;
}

// LD (BC),A
static int op_ldbca() {
  *(ram + (B << 8) + C) = A;
  return 7;
}

// LD (DE),A
static int op_lddea() {
  *(ram + (D << 8) + E) = A;
  return 7;
}

// LD (nn),A
static int op_ldnna() {
  unsigned i;

  i = *PC++;
  i += *PC++ << 8;
  *(ram + i) = A;
  return 13;
}

// LD (HL),A
static int op_ldhla() {
  *(ram + (H << 8) + L) = A;
  return 7;
}

// LD (HL),B
static int op_ldhlb() {
  *(ram + (H << 8) + L) = B;
  return 7;
}

// LD (HL),C
static int op_ldhlc() {
  *(ram + (H << 8) + L) = C;
  return 7;
}

// LD (HL),D
static int op_ldhld() {
  *(ram + (H << 8) + L) = D;
  return 7;
}

// LD (HL),E
static int op_ldhle() {
  *(ram + (H << 8) + L) = E;
  return 7;
}

// LD (HL),H
static int op_ldhlh() {
  *(ram + (H << 8) + L) = H;
  return 7;
}

// LD (HL),L
static int op_ldhll() {
  *(ram + (H << 8) + L) = L;
  return 7;
}

// LD (HL),n
static int op_ldhl1() {
  *(ram + (H << 8) + L) = *PC++;
  return 10;
}

// LD A,A
static int op_ldaa() {
  return 4;
}

// LD A,B
static int op_ldab() {
  A = B;
  return 4;
}

// LD A,C
static int op_ldac() {
  A = C;
  return 4;
}

// LD A,D
static int op_ldad() {
  A = D;
  return 4;
}

// LD A,E
static int op_ldae() {
  A = E;
  return 4;
}

// LD A,H
static int op_ldah() {
  A = H;
  return 4;
}

// LD A,L
static int op_ldal() {
  A = L;
  return 4;
}

// LD A,(HL)
static int op_ldahl() {
  A = *(ram + (H << 8) + L);
  return 7;
}

// LD B,A
static int op_ldba() {
  B = A;
  return 4;
}

// LD B,B
static int op_ldbb() {
  return 4;
}

// LD B,C
static int op_ldbc() {
  B = C;
  return 4;
}

// LD B,D
static int op_ldbd() {
  B = D;
  return 4;
}

// LD B,E
static int op_ldbe() {
  B = E;
  return 4;
}

// LD B,H
static int op_ldbh() {
  B = H;
  return 4;
}

// LD B,L
static int op_ldbl() {
  B = L;
  return 4;
}

// LD B,(HL)
static int op_ldbhl() {
  B = *(ram + (H << 8) + L);
  return 7;
}

// LD C,A
static int op_ldca() {
  C = A;
  return 4;
}

// LD C,B
static int op_ldcb() {
  C = B;
  return 4;
}

// LD C,C
static int op_ldcc() {
  return 4;
}

// LD C,D
static int op_ldcd() {
  C = D;
  return 4;
}

// LD C,E
static int op_ldce() {
  C = E;
  return 4;
}

// LD C,H
static int op_ldch() {
  C = H;
  return 4;
}

// LD C,L
static int op_ldcl() {
  C = L;
  return 4;
}

// LD C,(HL)
static int op_ldchl() {
  C = *(ram + (H << 8) + L);
  return 7;
}

// LD D,A
static int op_ldda() {
  D = A;
  return 4;
}

// LD D,B
static int op_lddb() {
  D = B;
  return 4;
}

// LD D,C
static int op_lddc() {
  D = C;
  return 4;
}

// LD D,D
static int op_lddd() {
  return 4;
}

// LD D,E
static int op_ldde() {
  D = E;
  return 4;
}

// LD D,H
static int op_lddh() {
  D = H;
  return 4;
}

// LD D,L
static int op_lddl() {
  D = L;
  return 4;
}

// LD D,(HL)
static int op_lddhl() {
  D = *(ram + (H << 8) + L);
  return 7;
}

// LD E,A
static int op_ldea() {
  E = A;
  return 4;
}

// LD E,B
static int op_ldeb() {
  E = B;
  return 4;
}

// LD E,C
static int op_ldec() {
  E = C;
  return 4;
}

// LD E,D
static int op_lded() {
  E = D;
  return 4;
}

// LD E,E
static int op_ldee() {
  return 4;
}

// LD E,H
static int op_ldeh() {
  E = H;
  return 4;
}

// LD E,L
static int op_ldel() {
  E = L;
  return 4;
}

// LD E,(HL)
static int op_ldehl() {
  E = *(ram + (H << 8) + L);
  return 7;
}

// LD H,A
static int op_ldha() {
  H = A;
  return 4;
}

// LD H,B
static int op_ldhb() {
  H = B;
  return 4;
}

// LD H,C
static int op_ldhc() {
  H = C;
  return 4;
}

// LD H,D
static int op_ldhd() {
  H = D;
  return 4;
}

// LD H,E
static int op_ldhe() {
  H = E;
  return 4;
}

// LD H,H
static int op_ldhh() {
  return 4;
}

// LD H,L
static int op_ldhl() {
  H = L;
  return 4;
}

// LD H,(HL)
static int op_ldhhl() {
  H = *(ram + (H << 8) + L);
  return 7;
}

// LD L,A
static int op_ldla() {
  L = A;
  return 4;
}

// LD L,B
static int op_ldlb() {
  L = B;
  return 4;
}

// LD L,C
static int op_ldlc() {
  L = C;
  return 4;
}

// LD L,D
static int op_ldld() {
  L = D;
  return 4;
}

// LD L,E
static int op_ldle() {
  L = E;
  return 4;
}

// LD L,H
static int op_ldlh() {
  L = H;
  return 4;
}

// LD L,L
static int op_ldll() {
  return 4;
}

// LD L,(HL)
static int op_ldlhl() {
  L = *(ram + (H << 8) + L);
  return 7;
}

// LD BC,nn
static int op_ldbcnn() {
  C = *PC++;
  B = *PC++;
  return 10;
}

// LD DE,nn
static int op_lddenn() {
  E = *PC++;
  D = *PC++;
  return 10;
}

// LD HL,nn
static int op_ldhlnn() {
  L = *PC++;
  H = *PC++;
  return 10;
}

// LD SP,nn
static int op_ldspnn() {
  STACK = ram + *PC++;
  STACK += *PC++ << 8;

  return 10;
}

// LD SP,HL
static int op_ldsphl() {
  STACK = ram + (H << 8) + L;
  return 6;
}

// LD HL,(nn)
static int op_ldhlin() {
  unsigned i;

  i = *PC++;
  i += *PC++ << 8;
  L = *(ram + i);
  H = *(ram + i + 1);
  return 16;
}

// LD (nn),HL
static int op_ldinhl() {
  unsigned i;

  i = *PC++;
  i += *PC++ << 8;
  *(ram + i) = L;
  *(ram + i + 1) = H;
  return 16;
}

// INC BC
static int op_incbc() {
  C++;
  if (!C) B++;
  return 6;
}

// INC DE
static int op_incde() {
  E++;
  if (!E) D++;
  return 6;
}

// INC HL
static int op_inchl() {
  L++;
  if (!L) H++;
  return 6;
}

// INC SP
static int op_incsp() {
  STACK++;
#ifdef ENABLE_SPC
  if (STACK > ram + 65535) STACK = ram;
#endif
  return 6;
}

// DEC BC
static int op_decbc() {
  C--;
  if (C == 0xff) B--;
  return 6;
}

// DEC DE
static int op_decde() {
  E--;
  if (E == 0xff) D--;
  return 6;
}

// DEC HL
static int op_dechl() {
  L--;
  if (L == 0xff) H--;
  return 6;
}

// DEC SP
static int op_decsp() {
  STACK--;
#ifdef ENABLE_SPC
  if (STACK < ram) STACK = ram + 65535;
#endif
  return 6;
}

// ADD HL,BC
static int op_adhlbc() {
  int carry;

  carry = (L + C > 255) ? 1 : 0;
  L += C;
  ((H & 0xf) + (B & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (H + B + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += B + carry;
  F &= ~N_FLAG;
  return 11;
}

// ADD HL,DE
static int op_adhlde() {
  int carry;

  carry = (L + E > 255) ? 1 : 0;
  L += E;
  ((H & 0xf) + (D & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (H + D + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += D + carry;
  F &= ~N_FLAG;
  return 11;
}

// ADD HL,HL
static int op_adhlhl() {
  int carry;

  carry = (L << 1 > 255) ? 1 : 0;
  L <<= 1;
  ((H & 0xf) + (H & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (H + H + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += H + carry;
  F &= ~N_FLAG;
  return 11;
}

// ADD HL,SP
static int op_adhlsp() {
  int carry;

  BYTE spl = (STACK - ram) & 0xff;
  BYTE sph = (STACK - ram) >> 8;
  
  carry = (L + spl > 255) ? 1 : 0;
  L += spl;
  ((H & 0xf) + (sph & 0xf) + carry > 0xf) ? (F |= H_FLAG)
            : (F &= ~H_FLAG);
  (H + sph + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += sph + carry;
  F &= ~N_FLAG;
  return 11;
}

// AND A
static int op_anda() {
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 4;
}

// AND B
static int op_andb() {
  A &= B;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 4;
}

// AND C
static int op_andc() {
  A &= C;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 4;
}

// AND D
static int op_andd() {
  A &= D;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 4;
}

// AND E
static int op_ande() {
  A &= E;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 4;
}

// AND H
static int op_andh() {
  A &= H;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 4;
}

// AND L
static int op_andl() {
  A &= L;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 4;
}

// AND (HL)
static int op_andhl() {
  A &= *(ram + (H << 8) + L);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 7;
}

// AND n
static int op_andn() {
  A &= *PC++;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= H_FLAG;
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(N_FLAG | C_FLAG);
  return 7;
}

// OR A
static int op_ora() {
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// OR B
static int op_orb() {
  A |= B;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// OR C
static int op_orc() {
  A |= C;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// OR D
static int op_ord() {
  A |= D;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// OR E
static int op_ore() {
  A |= E;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// OR H
static int op_orh() {
  A |= H;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// OR L
static int op_orl() {
  A |= L;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// OR (HL)
static int op_orhl() {
  A |= *(ram + (H << 8) + L);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 7;
}

// OR n
static int op_orn() {
  A |= *PC++;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 7;
}

// XOR A
static int op_xora() {
  A = 0;
  F &= ~(S_FLAG | H_FLAG | N_FLAG | C_FLAG);
  F |= Z_FLAG | P_FLAG;
  return 4;
}

// XOR B
static int op_xorb() {
  A ^= B;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// XOR C
static int op_xorc() {
  A ^= C;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// XOR D
static int op_xord() {
  A ^= D;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// XOR E
static int op_xore() {
  A ^= E;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// XOR H
static int op_xorh() {
  A ^= H;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// XOR L
static int op_xorl() {
  A ^= L;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 4;
}

// XOR (HL)
static int op_xorhl() {
  A ^= *(ram + (H << 8) + L);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 7;
}

// XOR n
static int op_xorn() {
  A ^= *PC++;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (parity[A]) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | N_FLAG | C_FLAG);
  return 7;
}

// ADD A,A
static int op_adda() {
  int i;

  ((A & 0xf) + (A & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  ((A << 1) > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = ((signed char) A) << 1;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADD A,B
static int op_addb() {
  int i;

  ((A & 0xf) + (B & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + B > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) B;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADD A,C
static int op_addc() {
  int i;

  ((A & 0xf) + (C & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + C > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) C;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADD A,D
static int op_addd() {
  int i;

  ((A & 0xf) + (D & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + D > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) D;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADD A,E
static int op_adde() {
  int i;

  ((A & 0xf) + (E & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + E > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) E;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADD A,H
static int op_addh() {
  int i;

  ((A & 0xf) + (H & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + H > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) H;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADD A,L
static int op_addl() {
  int i;

  ((A & 0xf) + (L & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + L > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) L;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADD A,(HL)
static int op_addhl() {
  int i;
  BYTE P;

  P = *(ram + (H << 8) + L);
  ((A & 0xf) + (P & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 7;
}

// ADD A,n
static int op_addn() {
  int i;
  BYTE P;

  P = *PC++;
  ((A & 0xf) + (P & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 7;
}

// ADC A,A
static int op_adca() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (A & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  ((A << 1) + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (((signed char) A) << 1) + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADC A,B
static int op_adcb() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (B & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + B + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) B + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADC A,C
static int op_adcc() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (C & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + C + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) C + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADC A,D
static int op_adcd() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (D & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + D + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) D + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADC A,E
static int op_adce() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (E & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + E + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) E + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADC A,H
static int op_adch() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (H & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + H + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) H + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADC A,L
static int op_adcl() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (L & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + L + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) L + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// ADC A,(HL)
static int op_adchl() {
  int i, carry;
  BYTE P;

  P = *(ram + (H << 8) + L);
  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (P & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) P + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 7;
}

// ADC A,n
static int op_adcn() {
  int i, carry;
  BYTE P;

  carry = (F & C_FLAG) ? 1 : 0;
  P = *PC++;
  ((A & 0xf) + (P & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A + (signed char) P + carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 7;
}

// SUB A,A
static int op_suba() {
  A = 0;
  F &= ~(S_FLAG | H_FLAG | P_FLAG | C_FLAG);
  F |= Z_FLAG | N_FLAG;
  return 4;
}

// SUB A,B
static int op_subb() {
  int i;

  ((B & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (B > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) B;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SUB A,C
static int op_subc() {
  int i;

  ((C & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (C > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) C;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SUB A,D
static int op_subd() {
  int i;

  ((D & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (D > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) D;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SUB A,E
static int op_sube() {
  int i;

  ((E & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (E > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) E;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SUB A,H
static int op_subh() {
  int i;

  ((H & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (H > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) H;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SUB A,L
static int op_subl() {
  int i;

  ((L & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (L > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) L;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SUB A,(HL)
static int op_subhl() {
  int i;
  BYTE P;

  P = *(ram + (H << 8) + L);
  ((P & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 7;
}

// SUB A,n
static int op_subn() {
  int i;
  BYTE P;

  P = *PC++;
  ((P & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 7;
}

// SBC A,A
static int op_sbca() {
  if (F & C_FLAG) {
    F |= S_FLAG | H_FLAG | N_FLAG | C_FLAG;
    F &= ~(Z_FLAG | P_FLAG);
    A = 255;
  } else {
    F |= Z_FLAG | N_FLAG;
    F &= ~(S_FLAG | H_FLAG | P_FLAG | C_FLAG);
    A = 0;
  }
  return 4;
}

// SBC A,B
static int op_sbcb() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((B & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (B + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) B - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SBC A,C
static int op_sbcc() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((C & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (C + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) C - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SBC A,D
static int op_sbcd() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((D & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (D + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) D - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SBC A,E
static int op_sbce() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((E & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (E + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) E - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SBC A,H
static int op_sbch() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((H & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (H + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) H - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SBC A,L
static int op_sbcl() {
  int i, carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((L & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (L + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) L - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// SBC A,(HL)
static int op_sbchl() {
  int i, carry;
  BYTE P;

  P = *(ram + (H << 8) + L);
  carry = (F & C_FLAG) ? 1 : 0;
  ((P & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) P - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 7;
}

// SBC A,n
static int op_sbcn() {
  int i, carry;
  BYTE P;

  P = *PC++;
  carry = (F & C_FLAG) ? 1 : 0;
  ((P & 0xf) + carry > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = i = (signed char) A - (signed char) P - carry;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 7;
}

// CP A
static int op_cpa() {
  F &= ~(S_FLAG | H_FLAG | P_FLAG | C_FLAG);
  F |= Z_FLAG | N_FLAG;
  return 4;
}

// CP B
static int op_cpb() {
  int i;

  ((B & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (B > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) B;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// CP C
static int op_cpc() {
  int i;

  ((C & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (C > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) C;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// CP D
static int op_cpd() {
  int i;

  ((D & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (D > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) D;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// CP E
static int op_cpe() {
  int i;

  ((E & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (E > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) E;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// CP H
static int op_cph() {
  int i;

  ((H & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (H > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) H;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// CP L
static int op_cplr() {
  int i;

  ((L & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (L > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) L;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// CP (HL)
static int op_cphl() {
  int i;
  BYTE P;

  P = *(ram + (H << 8) + L);
  ((P & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 7;
}

// CP n
static int op_cpn() {
  int i;
  BYTE P;

  P = *PC++;
  ((P & 0xf) > (A & 0xf)) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = (signed char) A - (signed char) P;
  (i < -128 || i > 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 7;
}

// INC A
static int op_inca() {
  ((A & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A++;
  (A == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// INC B
static int op_incb() {
  ((B & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  B++;
  (B == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// INC C
static int op_incc() {
  ((C & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  C++;
  (C == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (C & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (C) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// INC D
static int op_incd() {
  ((D & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  D++;
  (D == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (D & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (D) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// INC E
static int op_ince() {
  ((E & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  E++;
  (E == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (E & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (E) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// INC H
static int op_inch() {
  ((H & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  H++;
  (H == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (H) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// INC L
static int op_incl() {
  ((L & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  L++;
  (L == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (L & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (L) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 4;
}

// INC (HL)
static int op_incihl() {
  BYTE *p;

  p = ram + (H << 8) + L;
  ((*p & 0xf) + 1 > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (*p)++;
  (*p == 128) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F &= ~N_FLAG;
  return 11;
}

// DEC A
static int op_deca() {
  (((A - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A--;
  (A == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// DEC B
static int op_decb() {
  (((B - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  B--;
  (B == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// DEC C
static int op_decc() {
  (((C - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  C--;
  (C == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (C & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (C) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// DEC D
static int op_decd() {
  (((D - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  D--;
  (D == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (D & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (D) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// DEC E
static int op_dece() {
  (((E - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  E--;
  (E == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (E & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (E) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// DEC H
static int op_dech() {
  (((H - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  H--;
  (H == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (H) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// DEC L
static int op_decl() {
  (((L - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  L--;
  (L == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (L & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (L) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 4;
}

// DEC (HL)
static int op_decihl() {
  BYTE *p;

  p = ram + (H << 8) + L;
  (((*p - 1) & 0xf) == 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (*p)--;
  (*p == 127) ? (F |= P_FLAG) : (F &= ~P_FLAG);
  (*p & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (*p) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  F |= N_FLAG;
  return 11;
}

// RLCA
static int op_rlca() {
  int i;

  i = (A & 128) ? 1 : 0;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  F &= ~(H_FLAG | N_FLAG);
  A <<= 1;
  A |= i;
  return 4;
}

// RRCA
static int op_rrca() {
  int i;

  i = A & 1;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  F &= ~(H_FLAG | N_FLAG);
  A >>= 1;
  if (i) A |= 128;
  return 4;
}

// RLA
static int op_rla() {
  int old_c_flag;

  old_c_flag = F & C_FLAG;
  (A & 128) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  F &= ~(H_FLAG | N_FLAG);
  A <<= 1;
  if (old_c_flag) A |= 1;
  return 4;
}

// RRA
static int op_rra() {
  int i, old_c_flag;

  old_c_flag = F & C_FLAG;
  i = A & 1;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  F &= ~(H_FLAG | N_FLAG);
  A >>= 1;
  if (old_c_flag) A |= 128;
  return 4;
}

// EX DE,HL
static int op_exdehl()
{
  unsigned i;

  i = D;
  D = H;
  H = i;
  i = E;
  E = L;
  L = i;
  return 4;
}

// EX AF,AF'
static int op_exafaf() {
  unsigned i;

  i = A;
  A = A_;
  A_ = i;
  i = F;
  F = F_;
  F_ = i;
  return 4;
}

// EXX
static int op_exx() {
  unsigned i;

  i = B;
  B = B_;
  B_ = i;
  i = C;
  C = C_;
  C_ = i;
  i = D;
  D = D_;
  D_ = i;
  i = E;
  E = E_;
  E_ = i;
  i = H;
  H = H_;
  H_ = i;
  i = L;
  L = L_;
  L_ = i;
  return 4;
}

// EX (SP),HL
static int op_exsphl() {
  int i;

  i = *STACK;
  *STACK = L;
  L = i;
  i = *(STACK + 1);
  *(STACK + 1) = H;
  H = i;
  return 19;
}

// PUSH AF
static int op_pushaf() {
  CHECK_STACK_UNDERRUN();
  *--STACK = A;
  CHECK_STACK_UNDERRUN();
  *--STACK = F;
  return 11;
}

// PUSH BC
static int op_pushbc() {
  CHECK_STACK_UNDERRUN();
  *--STACK = B;
  CHECK_STACK_UNDERRUN();
  *--STACK = C;
  return 11;
}

// PUSH DE
static int op_pushde() {
  CHECK_STACK_UNDERRUN();
  *--STACK = D;
  CHECK_STACK_UNDERRUN();

  *--STACK = E;
  return 11;
}

// PUSH HL
static int op_pushhl() {
  CHECK_STACK_UNDERRUN();
  *--STACK = H;
  CHECK_STACK_UNDERRUN();
  *--STACK = L;
  return 11;
}

// POP AF
static int op_popaf() {
  F = *STACK++;
  CHECK_STACK_OVERRUN();
  A = *STACK++;
  CHECK_STACK_OVERRUN();
  return 10;
}

// POP BC
static int op_popbc() {
  C = *STACK++;
  CHECK_STACK_OVERRUN();
  B = *STACK++;
  CHECK_STACK_OVERRUN();
  return 10;
}

// POP DE
static int op_popde() {
  E = *STACK++;
  CHECK_STACK_OVERRUN();
  D = *STACK++;
  CHECK_STACK_OVERRUN();
  return 10;
}

// POP HL
static int op_pophl() {
  L = *STACK++;
  CHECK_STACK_OVERRUN();
  H = *STACK++;
  CHECK_STACK_OVERRUN();
  return 10;
}

// JP addr
static int op_jp() {
  unsigned i;

  i = *PC++;
  i += *PC << 8;
  PC = ram + i;
  return 10;
}

// JP (HL)
static int op_jphl() {
  PC = ram + (H << 8) + L;
  return 4;
}

// JR addr
static int op_jr() {
  PC += (signed char) *PC + 1;
  return 12;
}

// DJNZ
static int op_djnz() {
  if (--B) {
    PC += (signed char) *PC + 1;
    return 13;
  } else {
    PC++;
    return 8;
  }
}

// CALL addr
static int op_call() {
  unsigned i;

  i = *PC++;
  i += *PC++ << 8;
  CHECK_STACK_UNDERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_UNDERRUN();
  *--STACK = (PC - ram);
  PC = ram + i;
  return 17;
}

// RET
static int op_ret() {
  unsigned i;

  i = *STACK++;
  CHECK_STACK_OVERRUN();
  i += *STACK++ << 8;
  CHECK_STACK_OVERRUN();
  PC = ram + i;
  return 10;
}

// JP Z,addr
static int op_jpz() {
  unsigned i;

  if (F & Z_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// JP NZ,addr
static int op_jpnz() {
  unsigned i;

  if (!(F & Z_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// JP C,addr
static int op_jpc() {
  unsigned i;

  if (F & C_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// JP NC,addr
static int op_jpnc() {
  unsigned i;

  if (!(F & C_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// JP PE,addr
static int op_jppe() {
  unsigned i;

  if (F & P_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// JP PO,addr
static int op_jppo() {
  unsigned i;

  if (!(F & P_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// JP M,addr
static int op_jpm() {
  unsigned i;

  if (F & S_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// JP P,addr
static int op_jpp() {
  unsigned i;

  if (!(F & S_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    PC = ram + i;
  } else {
    PC += 2;
  }
  return 10;
}

// CALL Z,addr
static int op_calz() {
  unsigned i;

  if (F & Z_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// CALL NZ,addr
static int op_calnz() {
  unsigned i;

  if (!(F & Z_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// CALL C,addr
static int op_calc() {
  unsigned i;

  if (F & C_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// CALL NC,addr
static int op_calnc() {
  unsigned i;

  if (!(F & C_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// CALL PE,addr
static int op_calpe() {
  unsigned i;

  if (F & P_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// CALL PO,addr
static int op_calpo() {
  unsigned i;

  if (!(F & P_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// CALL M,addr
static int op_calm() {
  unsigned i;

  if (F & S_FLAG) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// CALL P,addr
static int op_calp() {
  unsigned i;

  if (!(F & S_FLAG)) {
    i = *PC++;
    i += *PC++ << 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram) >> 8;
    CHECK_STACK_UNDERRUN();
    *--STACK = (PC - ram);
    PC = ram + i;
    return 17;
  } else {
    PC += 2;
    return 10;
  }
}

// RET Z
static int op_retz() {
  unsigned i;

  if (F & Z_FLAG) {
    i = *STACK++;
    CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
    CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// RET NZ
static int op_retnz() {
  unsigned i;

  if (!(F & Z_FLAG)) {
    i = *STACK++;
  CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
  CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// RET C
static int op_retc() {
  unsigned i;

  if (F & C_FLAG) {
    i = *STACK++;
    CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
    CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// RET NC
static int op_retnc() {
  unsigned i;

  if (!(F & C_FLAG)) {
    i = *STACK++;
    CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
    CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// RET PE
static int op_retpe() {
  unsigned i;

  if (F & P_FLAG) {
    i = *STACK++;
    CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
    CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// RET PO
static int op_retpo() {
  unsigned i;

  if (!(F & P_FLAG)) {
    i = *STACK++;
    CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
    CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// RET M
static int op_retm() {
  unsigned i;

  if (F & S_FLAG) {
    i = *STACK++;
    CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
    CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// RET P
static int op_retp() {
  unsigned i;

  if (!(F & S_FLAG)) {
    i = *STACK++;
    CHECK_STACK_OVERRUN();
    i += *STACK++ << 8;
    CHECK_STACK_OVERRUN();
    PC = ram + i;
    return 11;
  } else {
    return 5;
  }
}

// JR Z,nn
static int op_jrz() {
  if (F & Z_FLAG) {
    PC += (signed char) *PC + 1;
    return 12;
  } else {
    PC++;
    return 7;
  }
}

// JR NZ,n
static int op_jrnz() {
  if (!(F & Z_FLAG)) {
    PC += (signed char) *PC + 1;
    return 12;
  } else {
    PC++;
    return 7;
  }
}

// JR C,n
static int op_jrc() {
  if (F & C_FLAG) {
    PC += (signed char) *PC + 1;
    return 12;
  } else {
    PC++;
    return 7;
  }
}

// JR NC,n
static int op_jrnc() {
  if (!(F & C_FLAG)) {
    PC += (signed char) *PC + 1;
    return 12;
  } else {
    PC++;
    return 7;
  }
}

// RST 00
static int op_rst00() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram;
  return 11;
}

// RST 08
static int op_rst08() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram + 0x08;
  return 11;
}

// RST 10
static int op_rst10() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram + 0x10;
  return 11;
}

// RST 18
static int op_rst18() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram + 0x18;
  return 11;
}

// RST 20
static int op_rst20() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram + 0x20;
  return 11;
}

// RST 28
static int op_rst28() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram + 0x28;
  return 11;
}

// RST 30
static int op_rst30() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram + 0x30;
  return 11;
}

// RST 38
static int op_rst38() {
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram) >> 8;
  CHECK_STACK_OVERRUN();
  *--STACK = (PC - ram);
  PC = ram + 0x38;
  return 11;
}

//
// Main Z80 CPU simulation routine.
//
// The opcode where PC points to is fetched from the memory and PC incremented
// by one. The opcode is used as an index to an array with function pointers
// that execute a function which emulates this Z80 opcode.
void cpu() {
  int t;

  static int (*op_sim[256])() = {
    op_nop,        /* 0x00 */
    op_ldbcnn,     /* 0x01 */
    op_ldbca,      /* 0x02 */
    op_incbc,      /* 0x03 */
    op_incb,       /* 0x04 */
    op_decb,       /* 0x05 */
    op_ldbn,       /* 0x06 */
    op_rlca,       /* 0x07 */
    op_exafaf,     /* 0x08 */
    op_adhlbc,     /* 0x09 */
    op_ldabc,      /* 0x0a */
    op_decbc,      /* 0x0b */
    op_incc,       /* 0x0c */
    op_decc,       /* 0x0d */
    op_ldcn,       /* 0x0e */
    op_rrca,       /* 0x0f */
    op_djnz,       /* 0x10 */
    op_lddenn,     /* 0x11 */
    op_lddea,      /* 0x12 */
    op_incde,      /* 0x13 */
    op_incd,       /* 0x14 */
    op_decd,       /* 0x15 */
    op_lddn,       /* 0x16 */
    op_rla,        /* 0x17 */
    op_jr,         /* 0x18 */
    op_adhlde,     /* 0x19 */
    op_ldade,      /* 0x1a */
    op_decde,      /* 0x1b */
    op_ince,       /* 0x1c */
    op_dece,       /* 0x1d */
    op_lden,       /* 0x1e */
    op_rra,        /* 0x1f */
    op_jrnz,       /* 0x20 */
    op_ldhlnn,     /* 0x21 */
    op_ldinhl,     /* 0x22 */
    op_inchl,      /* 0x23 */
    op_inch,       /* 0x24 */
    op_dech,       /* 0x25 */
    op_ldhn,       /* 0x26 */
    op_daa,        /* 0x27 */
    op_jrz,        /* 0x28 */
    op_adhlhl,     /* 0x29 */
    op_ldhlin,     /* 0x2a */
    op_dechl,      /* 0x2b */
    op_incl,       /* 0x2c */
    op_decl,       /* 0x2d */
    op_ldln,       /* 0x2e */
    op_cpl,        /* 0x2f */
    op_jrnc,       /* 0x30 */
    op_ldspnn,     /* 0x31 */
    op_ldnna,      /* 0x32 */
    op_incsp,      /* 0x33 */
    op_incihl,     /* 0x34 */
    op_decihl,     /* 0x35 */
    op_ldhl1,      /* 0x36 */
    op_scf,        /* 0x37 */
    op_jrc,        /* 0x38 */
    op_adhlsp,     /* 0x39 */
    op_ldann,      /* 0x3a */
    op_decsp,      /* 0x3b */
    op_inca,       /* 0x3c */
    op_deca,       /* 0x3d */
    op_ldan,       /* 0x3e */
    op_ccf,        /* 0x3f */
    op_ldbb,       /* 0x40 */
    op_ldbc,       /* 0x41 */
    op_ldbd,       /* 0x42 */
    op_ldbe,       /* 0x43 */
    op_ldbh,       /* 0x44 */
    op_ldbl,       /* 0x45 */
    op_ldbhl,      /* 0x46 */
    op_ldba,       /* 0x47 */
    op_ldcb,       /* 0x48 */
    op_ldcc,       /* 0x49 */
    op_ldcd,       /* 0x4a */
    op_ldce,       /* 0x4b */
    op_ldch,       /* 0x4c */
    op_ldcl,       /* 0x4d */
    op_ldchl,      /* 0x4e */
    op_ldca,       /* 0x4f */
    op_lddb,       /* 0x50 */
    op_lddc,       /* 0x51 */
    op_lddd,       /* 0x52 */
    op_ldde,       /* 0x53 */
    op_lddh,       /* 0x54 */
    op_lddl,       /* 0x55 */
    op_lddhl,      /* 0x56 */
    op_ldda,       /* 0x57 */
    op_ldeb,       /* 0x58 */
    op_ldec,       /* 0x59 */
    op_lded,       /* 0x5a */
    op_ldee,       /* 0x5b */
    op_ldeh,       /* 0x5c */
    op_ldel,       /* 0x5d */
    op_ldehl,      /* 0x5e */
    op_ldea,       /* 0x5f */
    op_ldhb,       /* 0x60 */
    op_ldhc,       /* 0x61 */
    op_ldhd,       /* 0x62 */
    op_ldhe,       /* 0x63 */
    op_ldhh,       /* 0x64 */
    op_ldhl,       /* 0x65 */
    op_ldhhl,      /* 0x66 */
    op_ldha,       /* 0x67 */
    op_ldlb,       /* 0x68 */
    op_ldlc,       /* 0x69 */
    op_ldld,       /* 0x6a */
    op_ldle,       /* 0x6b */
    op_ldlh,       /* 0x6c */
    op_ldll,       /* 0x6d */
    op_ldlhl,      /* 0x6e */
    op_ldla,       /* 0x6f */
    op_ldhlb,      /* 0x70 */
    op_ldhlc,      /* 0x71 */
    op_ldhld,      /* 0x72 */
    op_ldhle,      /* 0x73 */
    op_ldhlh,      /* 0x74 */
    op_ldhll,      /* 0x75 */
    op_halt,       /* 0x76 */
    op_ldhla,      /* 0x77 */
    op_ldab,       /* 0x78 */
    op_ldac,       /* 0x79 */
    op_ldad,       /* 0x7a */
    op_ldae,       /* 0x7b */
    op_ldah,       /* 0x7c */
    op_ldal,       /* 0x7d */
    op_ldahl,      /* 0x7e */
    op_ldaa,       /* 0x7f */
    op_addb,       /* 0x80 */
    op_addc,       /* 0x81 */
    op_addd,       /* 0x82 */
    op_adde,       /* 0x83 */
    op_addh,       /* 0x84 */
    op_addl,       /* 0x85 */
    op_addhl,      /* 0x86 */
    op_adda,       /* 0x87 */
    op_adcb,       /* 0x88 */
    op_adcc,       /* 0x89 */
    op_adcd,       /* 0x8a */
    op_adce,       /* 0x8b */
    op_adch,       /* 0x8c */
    op_adcl,       /* 0x8d */
    op_adchl,      /* 0x8e */
    op_adca,       /* 0x8f */
    op_subb,       /* 0x90 */
    op_subc,       /* 0x91 */
    op_subd,       /* 0x92 */
    op_sube,       /* 0x93 */
    op_subh,       /* 0x94 */
    op_subl,       /* 0x95 */
    op_subhl,      /* 0x96 */
    op_suba,       /* 0x97 */
    op_sbcb,       /* 0x98 */
    op_sbcc,       /* 0x99 */
    op_sbcd,       /* 0x9a */
    op_sbce,       /* 0x9b */
    op_sbch,       /* 0x9c */
    op_sbcl,       /* 0x9d */
    op_sbchl,      /* 0x9e */
    op_sbca,       /* 0x9f */
    op_andb,       /* 0xa0 */
    op_andc,       /* 0xa1 */
    op_andd,       /* 0xa2 */
    op_ande,       /* 0xa3 */
    op_andh,       /* 0xa4 */
    op_andl,       /* 0xa5 */
    op_andhl,      /* 0xa6 */
    op_anda,       /* 0xa7 */
    op_xorb,       /* 0xa8 */
    op_xorc,       /* 0xa9 */
    op_xord,       /* 0xaa */
    op_xore,       /* 0xab */
    op_xorh,       /* 0xac */
    op_xorl,       /* 0xad */
    op_xorhl,      /* 0xae */
    op_xora,       /* 0xaf */
    op_orb,        /* 0xb0 */
    op_orc,        /* 0xb1 */
    op_ord,        /* 0xb2 */
    op_ore,        /* 0xb3 */
    op_orh,        /* 0xb4 */
    op_orl,        /* 0xb5 */
    op_orhl,       /* 0xb6 */
    op_ora,        /* 0xb7 */
    op_cpb,        /* 0xb8 */
    op_cpc,        /* 0xb9 */
    op_cpd,        /* 0xba */
    op_cpe,        /* 0xbb */
    op_cph,        /* 0xbc */
    op_cplr,       /* 0xbd */
    op_cphl,       /* 0xbe */
    op_cpa,        /* 0xbf */
    op_retnz,      /* 0xc0 */
    op_popbc,      /* 0xc1 */
    op_jpnz,       /* 0xc2 */
    op_jp,         /* 0xc3 */
    op_calnz,      /* 0xc4 */
    op_pushbc,     /* 0xc5 */
    op_addn,       /* 0xc6 */
    op_rst00,      /* 0xc7 */
    op_retz,       /* 0xc8 */
    op_ret,        /* 0xc9 */
    op_jpz,        /* 0xca */
    op_cb_handler, /* 0xcb */
    op_calz,       /* 0xcc */
    op_call,       /* 0xcd */
    op_adcn,       /* 0xce */
    op_rst08,      /* 0xcf */
    op_retnc,      /* 0xd0 */
    op_popde,      /* 0xd1 */
    op_jpnc,       /* 0xd2 */
    op_out,        /* 0xd3 */
    op_calnc,      /* 0xd4 */
    op_pushde,     /* 0xd5 */
    op_subn,       /* 0xd6 */
    op_rst10,      /* 0xd7 */
    op_retc,       /* 0xd8 */
    op_exx,        /* 0xd9 */
    op_jpc,        /* 0xda */
    op_in,         /* 0xdb */
    op_calc,       /* 0xdc */
    op_dd_handler, /* 0xdd */
    op_sbcn,       /* 0xde */
    op_rst18,      /* 0xdf */
    op_retpo,      /* 0xe0 */
    op_pophl,      /* 0xe1 */
    op_jppo,       /* 0xe2 */
    op_exsphl,     /* 0xe3 */
    op_calpo,      /* 0xe4 */
    op_pushhl,     /* 0xe5 */
    op_andn,       /* 0xe6 */
    op_rst20,      /* 0xe7 */
    op_retpe,      /* 0xe8 */
    op_jphl,       /* 0xe9 */
    op_jppe,       /* 0xea */
    op_exdehl,     /* 0xeb */
    op_calpe,      /* 0xec */
    op_ed_handler, /* 0xed */
    op_xorn,       /* 0xee */
    op_rst28,      /* 0xef */
    op_retp,       /* 0xf0 */
    op_popaf,      /* 0xf1 */
    op_jpp,        /* 0xf2 */
    op_di,         /* 0xf3 */
    op_calp,       /* 0xf4 */
    op_pushaf,     /* 0xf5 */
    op_orn,        /* 0xf6 */
    op_rst30,      /* 0xf7 */
    op_retm,       /* 0xf8 */
    op_ldsphl,     /* 0xf9 */
    op_jpm,        /* 0xfa */
    op_ei,         /* 0xfb */
    op_calm,       /* 0xfc */
    op_fd_handler, /* 0xfd */
    op_cpn,        /* 0xfe */
    op_rst38       /* 0xff */
  };

  do {
#ifdef HISIZE
    // Update trace history.
    his[h_next].h_adr = PC - ram;
    his[h_next].h_af = (A << 8) + F;
    his[h_next].h_bc = (B << 8) + C;
    his[h_next].h_de = (D << 8) + E;
    his[h_next].h_hl = (H << 8) + L;
    his[h_next].h_ix = IX;
    his[h_next].h_iy = IY;
    his[h_next].h_sp = STACK - ram;

    h_next++;
    if (h_next == HISIZE) {
      h_flag = 1;
      h_next = 0;
    }
#endif

#ifdef ENABLE_TIM
    // Check for start address of runtime measurement.
    if (PC == t_start && !t_flag) {
      t_flag = 1; /* switch measurement on */
      t_states = 0L;  /* initialize counted T-states */
    }
#endif

#ifdef ENABLE_INT
    // CPU interrupt handling.
    if (int_type) {
      switch (int_type) {
        case INT_NMI:
          // Non-maskable interrupt.
          int_type = INT_NONE;
          IFF <<= 1;
          CHECK_STACK_UNDERRUN();
          *--STACK = (PC - ram) >> 8;
          CHECK_STACK_UNDERRUN();
          *--STACK = (PC - ram);
          PC = ram + 0x66;
          break;

        case INT_INT:
          // Maskable interrupt.
          if (IFF != 3) break;
          IFF = 0;
          switch (int_mode) {
            case 0:
              break;

            case 1:
              int_type = INT_NONE;
              CHECK_STACK_UNDERRUN();
              *--STACK = (PC - ram) >> 8;
              CHECK_STACK_UNDERRUN();
              *--STACK = (PC - ram);
              PC = ram + 0x38;
              break;

            case 2:
              int_type = INT_NONE;
              CHECK_STACK_UNDERRUN();
              *--STACK = (PC - ram) >> 8;
              CHECK_STACK_UNDERRUN();
              *--STACK = (PC - ram);
              PC = ram + (I << 8) + int_vec;
              PC = ram + *PC + (*(PC + 1) << 8);
              //printf("cpu: I=%x vec=%x isr %04X\n", 
              //       I, int_vec, (WORD) (PC - ram));
              break;
          }
          break;
      }
    }
#endif

    // Execute next opcode.
    t = (*op_sim[*PC++])();

#ifdef ENABLE_PCC
    // Check for PC overrun.
    if (PC > ram + 65535) PC = ram;
#endif

    // Increment refresh register.
    R++;
    cpu_poll(t);

#ifdef ENABLE_TIM
    // Perform runtime measurement.
    if (t_flag) {
      // Add T-states for this opcode.
      t_states += t;
      // Check for end address.
      if (PC == t_end) t_flag = 0;
    }
#endif
  } while (cpu_state);
}

