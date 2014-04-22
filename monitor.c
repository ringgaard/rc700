//
// Z80SIM  -  a Z80-CPU simulator
//
// Copyright (C) 1987-2006 by Udo Munk
// Modified for RC700 simulator by Michael Ringgaard
//
// ICE-type monitor for debugging Z80 programs on a host system.

#ifndef WIN32
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <ctype.h>
#include <signal.h>

#include "cpu.h"

#define CMDLEN     80    // Length of command buffers etc.

void disasm(unsigned char **p, int adr);
void dump_screen();
int fdc_mount_disk(int drive, char *imagefile);

BYTE *wrk_ram;     // Workpointer into memory for dumps etc.

// Parse hexadecimal number.
int exatoi(char *str) {
  int num = 0;
  while (isxdigit(*str)) {
    num *= 16;
    if (*str <= '9') {
      num += *str - '0';
    } else {
      num += toupper(*str) - '7';
    }
    str++;
  }
  return num;
}

// Handling of software breakpoints (HALT opcode):
//
// Output: 
//   0 breakpoint or other HALT opcode reached (stop)
//   1 breakpoint reached, passcounter not reached (continue)
static int handle_break() {
#ifdef SBSIZE
  int i;
  int break_address;

  // Search for breakpoint.
  for (i = 0; i < SBSIZE; i++) { 
    if (soft[i].sb_adr == PC - ram - 1) goto was_softbreak;
  }
  return 0;

was_softbreak:
#ifdef HISIZE
  // Update history.
  h_next--;     
  if (h_next < 0) h_next = 0;
#endif

  // Store address of breakpoint.
  break_address = PC - ram - 1;

  // HALT was a breakpoint.
  cpu_error = NONE;

  // Substitute HALT opcode by original opcode and execute it.
  PC--;
  *PC = soft[i].sb_oldopc;
  cpu_state = SINGLE_STEP;
  cpu();

  // Restore HALT opcode again.
  *(ram + soft[i].sb_adr) = 0x76;

  // Increment passcounter.
  soft[i].sb_passcount++;

  if (soft[i].sb_passcount != soft[i].sb_pass) {
    // Pass not reached, continue.
    return 1;
  }

  printf("Software breakpoint %d reached at %04x\n", i, break_address);
  soft[i].sb_passcount = 0;
#endif

  // Pass reached, stop.
  return 0;
}

// Output header for the CPU registers.
static void print_head() {
  printf("\nPC   A  SZHPNC I  IFF BC   DE   HL   A'F' B'C' D'E' H'L' IX   IY   SP\n");
}

// Output all CPU registers.
static void print_reg() {
  printf("%04x %02x ", (unsigned int)(PC - ram), A);
  printf("%c", F & S_FLAG ? '1' : '0');
  printf("%c", F & Z_FLAG ? '1' : '0');
  printf("%c", F & H_FLAG ? '1' : '0');
  printf("%c", F & P_FLAG ? '1' : '0');
  printf("%c", F & N_FLAG ? '1' : '0');
  printf("%c", F & C_FLAG ? '1' : '0');
  printf(" %02x ", I);
  printf("%c", IFF & 1 ? '1' : '0');
  printf("%c", IFF & 2 ? '1' : '0');
  printf("  %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %04x %04x %04x\n",
         B, C, D, E, H, L, A_, F_, B_, C_, D_, E_, H_, L_, IX, IY,
         (unsigned int)(STACK - ram));
}

// Error handler after CPU is stopped.
static void cpu_err_msg() {
  switch (cpu_error) {
    case NONE:
      break;

    case OPHALT:
      printf("HALT Op-Code reached at %04x\n",
             (unsigned int)(PC - ram - 1));
      break;

    case IOTRAP:
      printf("I/O Trap at %04x\n", (unsigned int)(PC - ram));
      break;

    case OPTRAP1:
      printf("Op-code trap at %04x %02x\n",
             (unsigned int)(PC - 1 - ram), *(PC-1));
      break;

    case OPTRAP2:
      printf("Op-code trap at %04x %02x %02x\n",
             (unsigned int)(PC - 2 - ram),
             *(PC-2), *(PC-1));
      break;

    case OPTRAP4:
      printf("Op-code trap at %04x %02x %02x %02x %02x\n",
             (unsigned int)(PC - 4 - ram), *(PC-4), *(PC-3),
             *(PC-2), *(PC-1));
      break;

    case USERINT:
      puts("User Interrupt");
      break;

    default:
      printf("Unknown error %d\n", cpu_error);
      break;
  }
}

// Execute a single step.
static void do_step() {
  BYTE *p;

  cpu_state = SINGLE_STEP;
  cpu_error = NONE;
  cpu();
  if (cpu_error == OPHALT) handle_break();
  cpu_err_msg();
  print_head();
  print_reg();
  p = PC;
  disasm(&p, p - ram);
}

// Execute several steps with trace output.
static void do_trace(char *s) {
  int count, i;

  while (isspace(*s)) s++;
  if (*s == '\0') {
    count = 20;
  } else {
    count = atoi(s);
  }

  cpu_state = SINGLE_STEP;
  cpu_error = NONE;
  print_head();
  print_reg();
  for (i = 0; i < count; i++) {
    cpu();
    print_reg();
    if (cpu_error) {
      if (cpu_error == OPHALT) {
        if (!handle_break()) break;
      } else {
        break;
      }
    }
  }
  cpu_err_msg();
}

// Start the CPU emulation.
static void do_go(char *s) {
  while (isspace(*s)) s++;
  if (isxdigit(*s)) PC = ram + exatoi(s);
cont:
  cpu_state = CONTIN_RUN;
  cpu_error = NONE;
  cpu();
  if (cpu_error == OPHALT) {
    if (handle_break()) {
      if (!cpu_error) goto cont;
    }
  }
  cpu_err_msg();
  print_head();
  print_reg();
}

// Memory dump.
static void do_dump(char *s) {
  int i, j;
  BYTE c;

  while (isspace(*s)) s++;
  if (isxdigit(*s)) {
    wrk_ram = ram + exatoi(s) - exatoi(s) % 16;
  }
  printf("Adr    ");
  for (i = 0; i < 16; i++) printf("%02x ", i);
  puts(" ASCII");
  for (i = 0; i < 16; i++) {
    printf("%04x - ", (unsigned int)(wrk_ram - ram));
    for (j = 0; j < 16; j++) {
      printf("%02x ", *wrk_ram);
      wrk_ram++;
      if (wrk_ram > ram + 65535) wrk_ram = ram;
    }
    putchar('\t');
    for (j = -16; j < 0; j++) {
      printf("%c",((c = *(wrk_ram+j)) >= ' ' && c <= 0x7f) ? c : '.');
    }
    putchar('\n');
  }
}

// Disassemble.
static void do_list(char *s) {
  int i;

  while (isspace(*s)) s++;
  if (isxdigit(*s)) wrk_ram = ram + exatoi(s);

  for (i = 0; i < 10; i++) {
    printf("%04x - ", (unsigned int)(wrk_ram - ram));
    disasm(&wrk_ram, wrk_ram - ram);
    if (wrk_ram > ram + 65535) wrk_ram = ram;
  }
}

// Memory modify.
static void do_modify(char *s) {
  char nv[CMDLEN];

  while (isspace(*s)) s++;
  if (isxdigit(*s)) wrk_ram = ram + exatoi(s);
  for (;;) {
    printf("%04x = %02x : ", (unsigned int)(wrk_ram - ram), *wrk_ram);
    fgets(nv, sizeof(nv), stdin);
    if (nv[0] == '\n') {
      wrk_ram++;
      if (wrk_ram > ram + 65535) wrk_ram = ram;
      continue;
    }
    if (!isxdigit(nv[0])) break;
    *wrk_ram++ = exatoi(nv);
    if (wrk_ram > ram + 65535) wrk_ram = ram;
  }
}

// Memory fill.
static void do_fill(char *s) {
  BYTE *p;
  int i;
  BYTE val;

  while (isspace(*s)) s++;
  p = ram + exatoi(s);
  while (*s != ',' && *s != '\0') s++;
  if (*s) {
    i = exatoi(++s);
  } else {
    puts("count missing");
    return;
  }
  while (*s != ',' && *s != '\0') s++;
  if (*s) {
    val = exatoi(++s);
  } else {
    puts("value missing");
    return;
  }
  while (i--) {
    *p++ = val;
    if (p > ram + 65535) p = ram;
  }
}

// Memory move.
static void do_move(char *s) {
  BYTE *p1, *p2;
  int count;
  
  while (isspace(*s)) s++;
  p1 = ram + exatoi(s);
  while (*s != ',' && *s != '\0') s++;
  if (*s) {
    p2 = ram + exatoi(++s);
  } else {
    puts("to missing");
    return;
  }
  while (*s != ',' && *s != '\0') s++;
  if (*s) {
    count = exatoi(++s);
  } else {
    puts("count missing");
    return;
  }
  while (count--) {
    *p2++ = *p1++;
    if (p1 > ram + 65535) p1 = ram;
    if (p2 > ram + 65535) p2 = ram;
  }
}

// Port modify.
static void do_port(char *s) {
  BYTE port;
  char nv[CMDLEN];

  while (isspace(*s)) s++;
  port = exatoi(s);
  printf("%02x = %02x : ", port, cpu_in(port));
  fgets(nv, sizeof(nv), stdin);
  if (isxdigit(*nv)) cpu_out(port, (BYTE) exatoi(nv));
}

// Register modify.
static void do_reg(char *s) {
  char nv[CMDLEN];

  while (isspace(*s)) s++;
  if (*s == '\0') {
    print_head();
    print_reg();
  } else {
    if (strncmp(s, "bc'", 3) == 0) {
      printf("BC' = %04x : ", B_ * 256 + C_);
      fgets(nv, sizeof(nv), stdin);
      B_ = (exatoi(nv) & 0xffff) / 256;
      C_ = (exatoi(nv) & 0xffff) % 256;
    } else if (strncmp(s, "de'", 3) == 0) {
      printf("DE' = %04x : ", D_ * 256 + E_);
      fgets(nv, sizeof(nv), stdin);
      D_ = (exatoi(nv) & 0xffff) / 256;
      E_ = (exatoi(nv) & 0xffff) % 256;
    } else if (strncmp(s, "hl'", 3) == 0) {
      printf("HL' = %04x : ", H_ * 256 + L_);
      fgets(nv, sizeof(nv), stdin);
      H_ = (exatoi(nv) & 0xffff) / 256;
      L_ = (exatoi(nv) & 0xffff) % 256;
    } else if (strncmp(s, "pc", 2) == 0) {
      printf("PC = %04x : ", (unsigned int)(PC - ram));
      fgets(nv, sizeof(nv), stdin);
      PC = ram + (exatoi(nv) & 0xffff);
    } else if (strncmp(s, "bc", 2) == 0) {
      printf("BC = %04x : ", B * 256 + C);
      fgets(nv, sizeof(nv), stdin);
      B = (exatoi(nv) & 0xffff) / 256;
      C = (exatoi(nv) & 0xffff) % 256;
    } else if (strncmp(s, "de", 2) == 0) {
      printf("DE = %04x : ", D * 256 + E);
      fgets(nv, sizeof(nv), stdin);
      D = (exatoi(nv) & 0xffff) / 256;
      E = (exatoi(nv) & 0xffff) % 256;
    } else if (strncmp(s, "hl", 2) == 0) {
      printf("HL = %04x : ", H * 256 + L);
      fgets(nv, sizeof(nv), stdin);
      H = (exatoi(nv) & 0xffff) / 256;
      L = (exatoi(nv) & 0xffff) % 256;
    } else if (strncmp(s, "ix", 2) == 0) {
      printf("IX = %04x : ", IX);
      fgets(nv, sizeof(nv), stdin);
      IX = exatoi(nv) & 0xffff;
    } else if (strncmp(s, "iy", 2) == 0) {
      printf("IY = %04x : ", IY);
      fgets(nv, sizeof(nv), stdin);
      IY = exatoi(nv) & 0xffff;
    } else if (strncmp(s, "sp", 2) == 0) {
      printf("SP = %04x : ", (unsigned int)(STACK - ram));
      fgets(nv, sizeof(nv), stdin);
      STACK = ram + (exatoi(nv) & 0xffff);
    } else if (strncmp(s, "fs", 2) == 0) {
      printf("S-FLAG = %c : ", (F & S_FLAG) ? '1' : '0');
      fgets(nv, sizeof(nv), stdin);
      F = (exatoi(nv)) ? (F | S_FLAG) : (F & ~S_FLAG);
    } else if (strncmp(s, "fz", 2) == 0) {
      printf("Z-FLAG = %c : ", (F & Z_FLAG) ? '1' : '0');
      fgets(nv, sizeof(nv), stdin);
      F = (exatoi(nv)) ? (F | Z_FLAG) : (F & ~Z_FLAG);
    } else if (strncmp(s, "fh", 2) == 0) {
      printf("H-FLAG = %c : ", (F & H_FLAG) ? '1' : '0');
      fgets(nv, sizeof(nv), stdin);
      F = (exatoi(nv)) ? (F | H_FLAG) : (F & ~H_FLAG);
    } else if (strncmp(s, "fp", 2) == 0) {
      printf("P-FLAG = %c : ", (F & P_FLAG) ? '1' : '0');
      fgets(nv, sizeof(nv), stdin);
      F = (exatoi(nv)) ? (F | P_FLAG) : (F & ~P_FLAG);
    } else if (strncmp(s, "fn", 2) == 0) {
      printf("N-FLAG = %c : ", (F & N_FLAG) ? '1' : '0');
      fgets(nv, sizeof(nv), stdin);
      F = (exatoi(nv)) ? (F | N_FLAG) : (F & ~N_FLAG);
    } else if (strncmp(s, "fc", 2) == 0) {
      printf("C-FLAG = %c : ", (F & C_FLAG) ? '1' : '0');
      fgets(nv, sizeof(nv), stdin);
      F = (exatoi(nv)) ? (F | C_FLAG) : (F & ~C_FLAG);
    } else if (strncmp(s, "a'", 2) == 0) {
      printf("A' = %02x : ", A_);
      fgets(nv, sizeof(nv), stdin);
      A_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "f'", 2) == 0) {
      printf("F' = %02x : ", F_);
      fgets(nv, sizeof(nv), stdin);
      F_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "b'", 2) == 0) {
      printf("B' = %02x : ", B_);
      fgets(nv, sizeof(nv), stdin);
      B_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "c'", 2) == 0) {
      printf("C' = %02x : ", C_);
      fgets(nv, sizeof(nv), stdin);
      C_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "d'", 2) == 0) {
      printf("D' = %02x : ", D_);
      fgets(nv, sizeof(nv), stdin);
      D_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "e'", 2) == 0) {
      printf("E' = %02x : ", E_);
      fgets(nv, sizeof(nv), stdin);
      E_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "h'", 2) == 0) {
      printf("H' = %02x : ", H_);
      fgets(nv, sizeof(nv), stdin);
      H_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "l'", 2) == 0) {
      printf("L' = %02x : ", L_);
      fgets(nv, sizeof(nv), stdin);
      L_ = exatoi(nv) & 0xff;
    } else if (strncmp(s, "i", 1) == 0) {
      printf("I = %02x : ", I);
      fgets(nv, sizeof(nv), stdin);
      I = exatoi(nv) & 0xff;
    } else if (strncmp(s, "a", 1) == 0) {
      printf("A = %02x : ", A);
      fgets(nv, sizeof(nv), stdin);
      A = exatoi(nv) & 0xff;
    } else if (strncmp(s, "f", 1) == 0) {
      printf("F = %02x : ", F);
      fgets(nv, sizeof(nv), stdin);
      F = exatoi(nv) & 0xff;
    } else if (strncmp(s, "b", 1) == 0) {
      printf("B = %02x : ", B);
      fgets(nv, sizeof(nv), stdin);
      B = exatoi(nv) & 0xff;
    } else if (strncmp(s, "c", 1) == 0) {
      printf("C = %02x : ", C);
      fgets(nv, sizeof(nv), stdin);
      C = exatoi(nv) & 0xff;
    } else if (strncmp(s, "d", 1) == 0) {
      printf("D = %02x : ", D);
      fgets(nv, sizeof(nv), stdin);
      D = exatoi(nv) & 0xff;
    } else if (strncmp(s, "e", 1) == 0) {
      printf("E = %02x : ", E);
      fgets(nv, sizeof(nv), stdin);
      E = exatoi(nv) & 0xff;
    } else if (strncmp(s, "h", 1) == 0) {
      printf("H = %02x : ", H);
      fgets(nv, sizeof(nv), stdin);
      H = exatoi(nv) & 0xff;
    } else if (strncmp(s, "l", 1) == 0) {
      printf("L = %02x : ", L);
      fgets(nv, sizeof(nv), stdin);
      L = exatoi(nv) & 0xff;
    } else {
      printf("can't change register %s\n", nv);
    }

    print_head();
    print_reg();
  }
}

// Software breakpoints.
static void do_break(char *s) {
#ifndef SBSIZE
  puts("Sorry, no breakpoints available");
  puts("Please recompile with SBSIZE defined");
#else
  int i;

  if (*s == '\n' || *s == '\0') {
    puts("No Addr Pass  Counter");
    for (i = 0; i < SBSIZE; i++) {
      if (soft[i].sb_pass) {
        printf("%02d %04x %05d %05d\n", i,
        soft[i].sb_adr,soft[i].sb_pass,soft[i].sb_passcount);
      }
    }
    return;
  }

  if (isxdigit(*s)) {
    i = atoi(s++);
    if (i >= SBSIZE) {
      printf("breakpoint %d not available\n", i);
      return;
    }
  } else {
    i = sb_next++;
    if (sb_next == SBSIZE) sb_next = 0;
  }

  while (isspace(*s)) s++;
  if (*s == 'c') {
    *(ram + soft[i].sb_adr) = soft[i].sb_oldopc;
    memset((char *) &soft[i], 0, sizeof(struct softbreak));
    return;
  }

  if (soft[i].sb_pass) {
    *(ram + soft[i].sb_adr) = soft[i].sb_oldopc;
  }
  soft[i].sb_adr = exatoi(s);
  soft[i].sb_oldopc = *(ram + soft[i].sb_adr);
  *(ram + soft[i].sb_adr) = 0x76;
  while (!iscntrl(*s) && !ispunct(*s)) s++;
  if (*s != ',') {
    soft[i].sb_pass = 1;
  } else {
    soft[i].sb_pass = exatoi(++s);
  }
  soft[i].sb_passcount = 0;
#endif
}

// History.
static void do_hist(char *s) {
#ifndef HISIZE
  puts("Sorry, no history available");
  puts("Please recompile with HISIZE defined in sim.h");
#else
  int i,  l, b, e, c, sa;

  while (isspace(*s)) s++;
  switch (*s) {
    case 'c':
      memset((char *) his, 0, sizeof(struct history) * HISIZE);
      h_next = 0;
      h_flag = 0;
      break;

    default:
      if ((h_next == 0) && (h_flag == 0)) {
        puts("History memory is empty");
        break;
      }
      e = h_next;
      b = (h_flag) ? h_next + 1 : 0;
      l = 0;
      while (isspace(*s)) s++;
      if (*s) {
        sa = exatoi(s);
      } else {
        sa = -1;
      }
      for (i = b; i != e; i++) {
        if (i == HISIZE) i = 0;
        if (sa != -1) {
          if (his[i].h_adr < sa) continue;
          sa = -1;
        }
        printf("%04x AF=%04x BC=%04x DE=%04x HL=%04x IX=%04x IY=%04x SP=%04x\n",
               his[i].h_adr, his[i].h_af, his[i].h_bc,
               his[i].h_de, his[i].h_hl, his[i].h_ix,
               his[i].h_iy, his[i].h_sp);
        l++;
        if (l == 20) {
          l = 0;
          printf("q = quit, else continue: ");
          c = getchar();
          putchar('\n');
          if (toupper(c) == 'Q') break;
        }
      }
  }
#endif
}

// Runtime measurement by counting the executed T states.
static void do_count(char *s) {
#ifndef ENABLE_TIM
  puts("Sorry, no t-state count available");
  puts("Please recompile with ENABLE_TIM defined in sim.h");
#else
  while (isspace(*s)) s++;
  if (*s == '\0') {
    puts("start  stop  status  T-states");
    printf("%04x   %04x    %s   %lu\n",
           (unsigned int)(t_start - ram),
           (unsigned int)(t_end - ram),
           t_flag ? "on ": "off", t_states);
  } else {
    t_start = ram + exatoi(s);
    while (*s != ',' && *s != '\0') s++;
    if (*s) t_end = ram + exatoi(++s);
    t_states = 0L;
    t_flag = 0;
  }
#endif
}

// Signal handler for the timer interrupt.
static void timeout(int sig) {
  cpu_state = STOPPED;
}

// Calculate the clock frequency of the emulated CPU:
// into memory locations 0000H to 0002H the following
// code will be stored:
//   LOOP: JP LOOP
// It uses 10 T states for each execution. A 3 secound
// timer is started and then the CPU. For every opcode
// fetch the R register is incremented by one and after
// the timer is down and stopps the emulation, the clock
// speed of the CPU is calculated with:
// f = R / 300000
static void do_clock() {
#ifndef WIN32
  BYTE save[3];

  // Save memory locations 0000H - 0002H.
  save[0] = *(ram + 0x0000);  
  save[1] = *(ram + 0x0001);
  save[2] = *(ram + 0x0002);

  // Store opcode JP 0000H at address 0000H.
  *(ram + 0x0000) = 0xc3;
  *(ram + 0x0001) = 0x00;
  *(ram + 0x0002) = 0x00;

  // set PC to this code.
  PC = ram + 0x0000;

  // Clear refresh register.
  R = 0L;

  // Initialize CPU.
  cpu_state = CONTIN_RUN;
  cpu_error = NONE;

  // Initialize timer interrupt handler.
  signal(SIGALRM, timeout);
  alarm(3);

  // Start CPU.
  cpu();        

  // Restore memory locations 0000H - 0002H.
  *(ram + 0x0000) = save[0];  
  *(ram + 0x0001) = save[1];
  *(ram + 0x0002) = save[2];

  // Compute clock frequency.
  if (cpu_error == NONE) {
    printf("clock frequency = %5.2f Mhz\n", ((float) R) / 300000.0);
  } else {
    puts("Interrupted by user");
  }
#endif
}

// Output informations about compiling options.
static void do_show() {
  int i;

#ifdef HISIZE
  i = HISIZE;
#else
  i = 0;
#endif
  printf("No. of entrys in history memory: %d\n", i);
#ifdef SBSIZE
  i = SBSIZE;
#else
  i = 0;
#endif
  printf("No. of software breakpoints: %d\n", i);
#ifdef ENABLE_SPC
  i = 1;
#else
  i = 0;
#endif
  printf("Stackpointer overflow %schecked\n", i ? "" : "not ");
#ifdef ENABLE_PCC
  i = 1;
#else
  i = 0;
#endif
  printf("Program counter overflow %schecked\n", i ? "" : "not ");
#ifdef ENABLE_TIM
  i = 1;
#else
  i = 0;
#endif
  printf("T-State counting %spossible\n", i ? "" : "im");
}

// Loader for binary images with Mostek header.
// Format of the first 3 bytes: 0xff ll lh
// ll = load address low
// lh = load address high
static int load_mos(int fd, char *fn) {
  BYTE fileb[3];
  unsigned count, readed;
  int rc = 0;

  // Read load address.
  read(fd, (char *) fileb, 3);
  
  // ...and set if not given.
  if (wrk_ram == NULL) {
    wrk_ram = ram + (fileb[2] * 256 + fileb[1]);
  }
  count = ram + 65535 - wrk_ram;
  if ((readed = read(fd, (char *) wrk_ram, count)) == count) {
    puts("Too much to load, stopped at 0xffff");
    rc = 1;
  }
  close(fd);
  printf("Loader statistics for file %s:\n", fn);
  printf("START : %04x\n", (unsigned int)(wrk_ram - ram));
  printf("END   : %04x\n", (unsigned int)(wrk_ram - ram + readed - 1));
  printf("LOADED: %04x\n", readed);
  PC = wrk_ram;
  return rc;
}
 
// Read a file into the memory of the emulated CPU.
// The following file formats are supported:
//
// binary images with Mostek header
static int do_getfile(char *s) {
  char fn[CMDLEN];
  BYTE fileb[5];
  char *pfn = fn;
  int fd;

  while (isspace(*s)) s++;
  while (*s != ',' && *s != '\n' && *s != '\0') *pfn++ = *s++;
  *pfn = '\0';
  if (strlen(fn) == 0) {
    puts("no input file given");
    return 1;
  }

  if ((fd = open(fn, O_RDONLY)) == -1) {
    printf("can't open file %s\n", fn);
    return 1;
  }

  if (*s == ',') {
    wrk_ram = ram + exatoi(++s);
  } else {
    wrk_ram = NULL;
  }

  // Read first 5 bytes of file.
  read(fd, (char *) fileb, 5);

  if (*fileb == (BYTE) 0xff) {
    // Mostek header.
    lseek(fd, 0l, 0);
    return load_mos(fd, fn);
  } else {
    printf("unkown format, can't load file %s\n", fn);
    close(fd);
    return 1;
  }
}

static int do_mount(char *s) {
  char fn[CMDLEN];
  char *pfn = fn;
  int drive;

  while (isspace(*s)) s++;
  while (*s != ',' && *s != '\n' && *s != '\0') *pfn++ = *s++;
  *pfn = '\0';
  if (strlen(fn) == 0) {
    puts("no disk image file name given");
    return 1;
  }

  if (*s == ',') {
    drive = exatoi(++s);
  } else {
    drive = 0;
  }

  fdc_mount_disk(drive & 3, fn);
}

// Output help text.
static void do_help() {
  puts("r filename[,address]      read object into memory");
  puts("d [address]               dump memory");
  puts("l [address]               list memory");
  puts("m [address]               modify memory");
  puts("f address,count,value     fill memory");
  puts("v from,to,count           move memory");
  puts("p address                 show/modify port");
  puts("g [address]               run program");
  puts("t [count]                 trace program");
  puts("return                    single step program");
  puts("x [register]              show/modify register");
  puts("x f<flag>                 modify flag");
  puts("b[no] address[,pass]      set soft breakpoint");
  puts("b                         show soft breakpoints");
  puts("b[no] c                   clear soft breakpoint");
  puts("h [address]               show history");
  puts("h c                       clear history");
  puts("z start,stop              set trigger address for t-state count");
  puts("z                         show t-state count");
  puts("c                         measure clock frequency");
  puts("s                         show settings");
  puts("n                         dump screen buffer");
  puts("M filename[,drive]        mount disk drive");
  puts("q                         quit");
}

// Monitor for z80 debugger.
void mon() {
  int eoj = 1;
  char cmd[CMDLEN];
  char *p;

  wrk_ram = ram;
  while (eoj) {
    printf(">>> ");
    fflush(stdout);
    if (fgets(cmd, CMDLEN, stdin) == NULL) {
      putchar('\n');
      continue;
    }

    for (p = cmd; *p; p++) {
      if (*p == '\n' || *p == '\r') *p = 0;
    }

    switch (*cmd) {
      case '\0':
      case '\n':
        do_step();
        break;

      case 't':
        do_trace(cmd + 1);
        break;

      case 'g':
        do_go(cmd + 1);
        break;

      case 'd':
        do_dump(cmd + 1);
        break;

      case 'l':
        do_list(cmd + 1);
        break;

      case 'm':
        do_modify(cmd + 1);
        break;

      case 'f':
        do_fill(cmd + 1);
        break;

      case 'v':
        do_move(cmd + 1);
        break;

      case 'x':
        do_reg(cmd + 1);
        break;

      case 'p':
        do_port(cmd + 1);
        break;

      case 'b':
        do_break(cmd + 1);
        break;

      case 'h':
        do_hist(cmd + 1);
        break;

      case 'z':
        do_count(cmd + 1);
        break;

      case 'c':
        do_clock();
        break;

      case 's':
        do_show();
        break;

      case 'n':
        dump_screen();
        break;

      case 'M':
        do_mount(cmd + 1);
        break;

      case '?':
        do_help();
        break;

      case 'r':
        do_getfile(cmd + 1);
        break;

      case 'q':
        eoj = 0;
        break;

      default:
        puts("???");
        break;
    }
  }
}

