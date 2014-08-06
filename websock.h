//
// RC700  -  a Regnecentralen RC700 emulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// WebSocket server
//

#define WS_OP_CONT  0x00
#define WS_OP_TEXT  0x01
#define WS_OP_BIN   0x02
#define WS_OP_CLOSE 0x08
#define WS_OP_PING  0x09
#define WS_OP_PONG  0x0A

#define WS_BLOCK    0
#define WS_NBLOCK   1

struct websock {
  int sock;
  unsigned char *buffer;
  unsigned char *end;
  unsigned char *limit;
  int opcode;
  int nbio;
};

void websock_init(struct websock *ws, int sock);
void websock_free(struct websock *ws);
int websock_handshake(struct websock *ws);

int websock_recv(struct websock *ws, int mode);

int websock_send(struct websock *ws, int type,
                 void *data1, int size1, 
                 void *data2, int size2);

