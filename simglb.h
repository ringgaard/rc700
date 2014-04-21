//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
//

BYTE A, B, C, D, E, H, L;
BYTE A_, B_, C_, D_, E_, H_, L_;
BYTE *PC, *STACK, I, IFF;
WORD IX, IY;
BYTE F, F_;
long R;
extern BYTE ram[];

int cpu_state, cpu_error;
int int_type, int_mode, int_vec;
extern int int_chain[];
float freq;

void (cpu_poll)();

extern int parity[];

int break_flag;
int i_flag;
int x_flag;
int f_flag;

#ifdef HISIZE
extern struct history his[];
int  h_next, h_flag;
#endif

#ifdef SBSIZE
extern struct softbreak soft[];
int sb_next;
#endif

#ifdef WANT_TIM
long t_states;
int  t_flag;
BYTE *t_start, *t_end;
#endif

