//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
// Modified for RC700 simulator by Michael Ringgaard
//

#define ENABLE_INT  // Enable CPU interrupt handling

//#define ENABLE_SPC  // Enable SP over-/underrun handling 0000<->FFFF
//#define ENABLE_PCC  // Enable PC overrun handling FFFF->0000
//#define ENABLE_TIM  // Enable runtime measurement

//#define HISIZE  100 // Number of entrys in history
//#define SBSIZE  4   // Number of software breakpoints

// CPU flags.
#define S_FLAG    128   
#define Z_FLAG     64
#define N2_FLAG    32
#define H_FLAG     16
#define N1_FLAG     8
#define P_FLAG      4
#define N_FLAG      2
#define C_FLAG      1

// Operation of simulated CPU.
#define SINGLE_STEP 0   // Single step
#define CONTIN_RUN  1   // Continual run
#define STOPPED     0   // Stop CPU because of error

// Simulator error codes.
#define NONE        0   // No error
#define OPHALT      1   // HALT op-code trap
#define IOTRAP      2   // IN/OUT trap
#define OPTRAP1     3   // Illegal 1 byte op-code trap
#define OPTRAP2     4   // Illegal 2 byte op-code trap
#define OPTRAP4     5   // Illegal 4 byte op-code trap
#define USERINT     6   // User interrupt

// Type of CPU interrupt.
#define INT_NONE    0   // No pending interrupt
#define INT_NMI     1   // Non maskable interrupt
#define INT_INT     2   // Maskable interrupt

// Register types.
typedef unsigned short WORD;    // 16 bit unsigned
typedef unsigned char  BYTE;    // 8 bit unsigned

// Z-80 registers.
BYTE A, B, C, D, E, H, L;
BYTE A_, B_, C_, D_, E_, H_, L_;
BYTE *PC, *STACK, I, IFF;
WORD IX, IY;
BYTE F, F_;
long R;

// 64K RAM image.
extern BYTE ram[];

// CPU simulator state.
int cpu_state, cpu_error;
int int_type, int_mode, int_vec;
extern int int_chain[];
float freq;

// Pre-computed parity table.
extern int parity[];

#ifdef ENABLE_SPC
#define CHECK_STACK_OVERRUN() if (STACK >= ram + 65536L) STACK = ram
#define CHECK_STACK_UNDERRUN() if (STACK <= ram) STACK = ram + 65536L
#else
#define CHECK_STACK_OVERRUN()
#define CHECK_STACK_UNDERRUN()
#endif

//
// Trace history.
//

#ifdef HISIZE
struct history {
  WORD  h_adr;    // Address of execution
  WORD  h_af;     // Rregister AF
  WORD  h_bc;     // Register BC
  WORD  h_de;     // Register DE
  WORD  h_hl;     // Register HL
  WORD  h_ix;     // Register IX
  WORD  h_iy;     // Register IY
  WORD  h_sp;     // Register SP
};

extern struct history his[];
int  h_next, h_flag;
#endif

//
// Breakpoints.
//

#ifdef SBSIZE
struct softbreak {
  WORD  sb_adr;       // Address of breakpoint
  BYTE  sb_oldopc;    // Op-code at address of breakpoint
  int sb_passcount;   // Pass counter
  int sb_pass;        // Number of pass to break
};

extern struct softbreak soft[];
int sb_next;
#endif

//
// Runtime measurement.
//

#ifdef ENABLE_TIM
long t_states;
int  t_flag;
BYTE *t_start, *t_end;
#endif

// CPU simluator functions.
void cpu();
void interrupt(int vec, int priority);

// CPU simulation callbacks.
BYTE cpu_in(BYTE adr);
void cpu_out(BYTE adr, BYTE data);
void cpu_poll();
void cpu_halt();

