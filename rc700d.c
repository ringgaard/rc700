//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// RC700 emulator server
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "rc700.h"
#include "websock.h"

// Message types for emulator protocol.
#define MSG_RUN    0
#define MSG_HALT   1
#define MSG_MOUNT  2
#define MSG_CLRSCR 3
#define MSG_CURSOR 4
#define MSG_SCREEN 5
#define MSG_KEY    6

// Directory for disk images.
char *image_dir = ".";

// WebSocket for communication with client.
struct websock ws;

// I/O handlers for all I/O ports.
static struct port ports[256];

// Emulate a 4 Mhz Z80 CPU with 50 Hz screen refresh rate.
#define CPU_CLOCK_FREQUENCY   4000000
#define FRAMES_PER_SECOND     50
#define CYCLES_PER_FRAME      (CPU_CLOCK_FREQUENCY / FRAMES_PER_SECOND)
#define MILLISECS_PER_FRAME   (1000 / FRAMES_PER_SECOND)

// Number of CPU cycles executed in current frame.
int quantum = 0; 

// Milliseconds delay per frame taking emulation speed into account.
int ms_per_frame = MILLISECS_PER_FRAME;

// Delay in milliseconds.
void delay(int ms) {
  usleep(ms * 1000);
}

// CPU port input and output traps.
static BYTE in_trap(int dev) { return 0; }
static void out_trap(BYTE data, int dev) {}

// Register handlers for CPU I/O port.
void register_port(int adr, BYTE (*in)(int dev), void (*out)(BYTE data, int dev), int dev) {
  ports[adr].in = in ? in : in_trap;
  ports[adr].out = out ? out : out_trap;
  ports[adr].dev = dev;
}

// CPU port input handler.
BYTE cpu_in(BYTE adr) {
  return (*ports[adr].in)(ports[adr].dev);
}

// CPU port output handler.
void cpu_out(BYTE adr, BYTE data) {
  (*ports[adr].out)(data, ports[adr].dev);
}

// Set emulator speed in percent.
void set_emulation_speed(int percent) {
  ms_per_frame = MILLISECS_PER_FRAME * 100 / percent;
  printf("speed: %d%%, %d ms/frame\n", percent, ms_per_frame);
}

// CPU poll handler.
void cpu_poll(int cycles) {
  int overhead;
  clock_t t;

  quantum += cycles;
  if (quantum < CYCLES_PER_FRAME) return;

  t = clock();
  if (pio_poll() || crt_poll()) {
    overhead = (clock() - t) * 1000 / CLOCKS_PER_SEC;
  } else {
    overhead = 0;
  }

  if (overhead < ms_per_frame) delay(ms_per_frame - overhead);
  quantum -= CYCLES_PER_FRAME;
}

// CPU halt handler.
void cpu_halt() {
  delay(ms_per_frame);
};

// Log message to console.
void logmsg(const char *fmt, ...) {
  va_list args;
  char buffer[1024];
  time_t now;
  struct tm tm;

  va_start(args, fmt);
  vsprintf(buffer, fmt, args);
  va_end(args);

  now = time(NULL);
  localtime_r(&now, &tm);

  printf("%04d/%02d/%02d %02d:%02d:%02d [%d] %s\n",
         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
         tm.tm_hour, tm.tm_min, tm.tm_sec, getpid(), buffer);
  fflush(stdout);
}

// Initialize RC700 emulator.
static void init_rc700() {
  int i;

  // Clear RAM and start executing at 0x000.
  PC = STACK = ram;
  memset(ram, 0, 65536);

  // Setup trap handler for all unused I/O ports.
  for (i = 0; i <= 255; i++) {
    register_port(i, in_trap, out_trap, i);
  }

  // Initialize peripheral devices.
  init_rom();
  init_pio();
  init_sio();
  init_ctc();
  init_dma();
  init_crt();
  init_fdc();
  init_wdc();
}

void rcterm_clear_screen(int cols, int rows) {
  unsigned char msg[1];

  //logmsg("clear screen");
  msg[0] = MSG_CLRSCR;

  if (websock_send(&ws, WS_OP_BIN, msg, 1, NULL, 0) < 0) {
    cpu_error = USERINT;
    cpu_state = STOPPED;
  }
}

void rcterm_screen(BYTE *screen, BYTE *prev, int cols, int rows) {
  unsigned char msg[3];

  //logmsg("refresh screen");
  msg[0] = MSG_SCREEN;
  msg[1] = cols;
  msg[2] = rows;

  if (websock_send(&ws, WS_OP_BIN, msg, 3, screen, cols * rows) < 0) {
    cpu_error = USERINT;
    cpu_state = STOPPED;
  }
}

void rcterm_set_cursor(int type, int underline) {
  //logmsg("set cursor %d %d", type, underline);
}

int rcterm_gotoxy(int col, int row) {
  unsigned char msg[3];

  //logmsg("gotoxy %d %d", col, row);
  msg[0] = MSG_CURSOR;
  msg[1] = col;
  msg[2] = row;

  if (websock_send(&ws, WS_OP_BIN, msg, 3, NULL, 0) < 0) {
    cpu_error = USERINT;
    cpu_state = STOPPED;
  }
  
  return 0;
}

int rcterm_keypressed() {
  int rc;

  // Try to receive packet from client.
  rc = websock_recv(&ws, WS_NBLOCK);
  if (rc < 0) {
    cpu_error = USERINT;
    cpu_state = STOPPED;
  }
  if (rc <= 0) return -1;

  // Check for key press event.
  if (ws.end - ws.buffer == 2 && ws.buffer[0] == MSG_KEY) {
    int key = ws.buffer[1];
    //logmsg("key %d", key);
    return key;
  }

  // Check for halt command.
  if (ws.end - ws.buffer == 1 && ws.buffer[0] == MSG_HALT) {
    logmsg("halt");
    cpu_error = USERINT;
    cpu_state = STOPPED;
  }

  return -1;
}

int mount_disk(int drive, char *image) {
  char *p;
  char path[256];

  // Check parameters.
  if (drive != 0 && drive != 1) return -1;
  for (p = image; *p; ++p) {
    int ch = *p;
    if (!isalnum(ch) && ch != '-' && ch != '_') return -1;
  }
  if (strlen(image_dir) + strlen(image) > 250) return -1;

  // Mount disk image.
  strcpy(path, image_dir);
  strcat(path, "/");
  strcat(path, image);
  strcat(path, ".IMD");
  return fdc_mount_disk(drive, path, FDC_READONLY);
}

// Run emulator instance.
int run(int sock) {
  struct timeval timeout;
  struct sockaddr_in client;
  int addrlen;

  // Get client address.
  addrlen = sizeof(struct sockaddr_in);
  if (getpeername(sock, (struct sockaddr *) &client, &addrlen) < 0) {
    perror("getpeername");
    return -1;
  }
  logmsg("connect from %s port %d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

  // Set send and receive timeout on socket.
  timeout.tv_sec = 60;
  timeout.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout));

  // Initialize websocket connection to client.
  websock_init(&ws, sock);
  if (websock_handshake(&ws) < 0) {
    websock_free(&ws);
    return -1;
  }

  // Initialize emulator.
  init_rc700();

  // Run emulator.
  cpu_state = STOPPED;
  for (;;) {
    if (cpu_state == CONTIN_RUN) {
      logmsg("start");
      cpu_error = NONE;
      cpu();
      logmsg("stop");
    } else {
      logmsg("wait");
      if (websock_recv(&ws, WS_BLOCK) <= 0) break;
      if (ws.end - ws.buffer == 1 && ws.buffer[0] == MSG_RUN) {
        cpu_state = CONTIN_RUN;
      } else if (ws.end - ws.buffer > 1 && ws.buffer[0] == MSG_MOUNT) {
        int drive = ws.buffer[1];
        int len = ws.end - ws.buffer - 2;
        if  (len < 127) {
          char image[128];
          memcpy(image, ws.buffer + 2, len);
          image[len] = 0;
          logmsg("mount %s on drive %d", image, drive);
          if (mount_disk(drive, image) < 0) {
            logmsg("mount failed");
            break;
          }
        }
      }
    }
  }

  // Close client connection.
  websock_free(&ws);

  logmsg("done");
  return 0;
}

static int terminate = 0;
 
static void break_handler(int sig) {
  terminate = 1;
}

int main(int argc, char *argv[]) {
  int sock;
  int rc;
  int pid;
  int port = 702;
  struct sockaddr_in sin;
  struct sigaction sa;
  int i;
  int opt;
  char *s, *p;
  char *pn = argv[0];

  // Get command line parameters.
  for (i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i], "-speed") == 0 && i + 1 < argc) {
        set_emulation_speed(atoi(argv[i++ + 1]));
      } else if (strcmp(argv[i], "-port") == 0 && i + 1 < argc) {
        port = atoi(argv[i++ + 1]);
      } else if (strcmp(argv[i], "-imgdir") == 0 && i + 1 < argc) {
        image_dir = argv[i++ + 1];
      } else {
        exit(1);
      }
    }
  }

  // Set up signal handlers.
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = break_handler;
  sigaction(SIGINT, &sa, NULL);
  signal(SIGCHLD, SIG_IGN);

  // Initialize listen socket.
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    exit(1);
  }
  opt = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

  // Bind socket.
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  rc = bind(sock, (struct sockaddr *) &sin, sizeof(sin));
  if (rc < 0) {
    perror("bind");
    exit(1);
  }

  // Start listening on socket.
  rc = listen(sock, 5);
  if (rc < 0) {
    perror("listen");
    exit(1);
  }
  logmsg("rc700 deamon listening on port %d", port);

  while (!terminate) {
    // Wait until new connection on port.
    int s = accept(sock, NULL, NULL);
    if (terminate) break;
    if (s < 0) {
      perror("accept");
      break;
    }

    // Fork new emulator instance.
    pid = fork();
    if (pid < 0) {
      perror("fork");
      break;
    }

    if (pid == 0) {
      // Running new emulator instance in child process.
      signal(SIGINT, SIG_DFL);
      close(sock);
      run(s);
      exit(0);
    } else {
      // Close new socket in parent process.
      close(s);
    }
  }

  // Terminate deamon.
  logmsg("terminating rc700 deamon");
  close(sock);
  return 0;
}

