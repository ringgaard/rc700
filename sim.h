//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
//

#define WANT_INT  // Enable CPU interrupt handling

//#define WANT_SPC  // Enable SP over-/underrun handling 0000<->FFFF
//#define WANT_PCC  // Enable PC overrun handling FFFF->0000
//#define WANT_TIM  // Enable runtime measurement

//#define HISIZE  100 // Number of entrys in history
//#define SBSIZE  4   // Number of software breakpoints

#define USR_COM "RC700 Simulator"
#define USR_REL "1.0"
#define USR_CPR "Copyright (C) 2014 by Michael Ringgaard"

#define COPYR "Copyright (C) 1987-2006 by Udo Munk"
#define RELEASE "1.9"

#define CMDLEN     80    // Length of command buffers etc.

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

typedef unsigned short WORD;    // 16 bit unsigned
typedef unsigned char  BYTE;    // 8 bit unsigned

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
#endif

#ifdef SBSIZE
struct softbreak {
  WORD  sb_adr;       // Address of breakpoint
  BYTE  sb_oldopc;    // Op-code at address of breakpoint
  int sb_passcount;   // Pass counter
  int sb_pass;        // Number of pass to break
};
#endif

#ifdef WANT_SPC
#define CHECK_STACK_OVERRUN() if (STACK >= ram + 65536L) STACK = ram
#define CHECK_STACK_UNDERRUN() if (STACK <= ram) STACK = ram + 65536L
#else
#define CHECK_STACK_OVERRUN()
#define CHECK_STACK_UNDERRUN()
#endif

void cpu();
void mon();

void init_rom();
void init_pio();
void init_sio();
void init_ctc();
void init_dma();
void init_crt();
void init_fdc();
void init_ftp();

// CTC channels.
#define CTC_CHANNEL_SIOA  0
#define CTC_CHANNEL_SIOB  1
#define CTC_CHANNEL_CRT   2
#define CTC_CHANNEL_FDC   3

void interrupt(int vec, int priority);
void genintr();
void register_port(int adr, BYTE (*in)(int dev), void (*out)(BYTE data, int dev), int dev);
void register_refresh(void (*handler)());
void ctc_trigger(int channel);
void delay(int ms);

int fdc_mount_disk(int drive, char *imagefile);
void fdc_flush_disk(int drive, char *imagefile);

WORD dma_address(int channel);
WORD dma_count(int channel);
int dma_completed(int channel);
void dma_transfer_start(int channel);
void dma_transfer(int channel, BYTE *data, int  bytes);
void dma_fill(int channel, BYTE value, int bytes);
WORD dma_fetch(int channel, WORD *size);
void dma_transfer_done(int channel);

#define FIFOSIZE        128

struct fifo {
  BYTE queue[FIFOSIZE];
  int count;
  int head;
  int tail;
};

int fifo_put(struct fifo *f, BYTE data);
BYTE fifo_get(struct fifo *f);
int fifo_empty(struct fifo *f);
int fifo_full(struct fifo *f);

void rcterm_init();
void rcterm_exit();
void rcterm_clear_screen(int cols, int rows);
void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows);
void rcterm_set_cursor(int type, int underline);
int rcterm_gotoxy(int col, int row);
int rcterm_keypressed();

