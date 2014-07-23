//
// RC700  -  a Regnecentralen RC700 simulator
//
// Copyright (C) 2012 by Michael Ringgaard
//
// WebSocket server
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "websock.h"
#include "sha1.h"

#define WS_FIN      0x80
#define WS_OPCODE   0x7F

#define WS_MASK     0x80
#define WS_LEN      0x7F

void websock_init(struct websock *ws, int sock) {
  memset(ws, 0, sizeof(struct websock));
  ws->sock = sock;
}

void websock_free(struct websock *ws) {
  close(ws->sock);
  free(ws->buffer);
}

void websock_mode(struct websock *ws, int mode) {
  if (mode == WS_BLOCK) {
    if (ws->nbio) {
      fcntl(ws->sock, F_SETFL, fcntl(ws->sock, F_GETFL) & ~O_NONBLOCK);
      ws->nbio = 0;
    }
  } else {
    if (!ws->nbio) {
      fcntl(ws->sock, F_SETFL, fcntl(ws->sock, F_GETFL) | O_NONBLOCK);
      ws->nbio = 1;
    }
  }
}

int websock_fill(struct websock *ws) {
  int n;

  if (ws->end == ws->limit) {
    int size = ws->limit - ws->buffer;
    int used = ws->end - ws->buffer;
    size = size ? size * 2 : 512;
    ws->buffer = realloc(ws->buffer, size);
    if (!ws->buffer) return -1;
    ws->end = ws->buffer + used;
    ws->limit = ws->buffer + size;
  }

  n = read(ws->sock, ws->end, ws->limit - ws->end);
  if (n > 0) ws->end += n;
  return n;
}

int websock_read(struct websock *ws, int size) {
  int left;

  // Prepare receive buffer.
  if (ws->limit - ws->buffer < size) {
    ws->buffer = realloc(ws->buffer, size);
    if (!ws->buffer) return -1;
    ws->limit = ws->buffer + size;
  }
  ws->end = ws->buffer;

  // Receive requested number of bytes.
  left = size;
  while (ws->end - ws->buffer < size) {
    int n = read(ws->sock, ws->end, left);
    if (n <= 0) {
      //printf("read: errno=%d %d\n", errno, errno == EAGAIN);
      return errno == EAGAIN ? 0 : -1;
    }
    left -= n;
    ws->end += n;
  }
  return size;
}

int websock_handshake(struct websock *ws) {
  unsigned char *p;
  unsigned char *body;
  unsigned char *key;
  unsigned char *keyend;
  sha1_context ctx;
  char response[SHA1_BASE64_LENGTH];
  char reply[SHA1_BASE64_LENGTH + 512];

  // Receive HTTP upgrade request.
  websock_mode(ws, WS_BLOCK);
  for (;;) {
    if (websock_fill(ws) < 0) return -1;
    body = NULL;
    for (p = ws->buffer; p < ws->end - 3; ++p) {
      if (*p == '\r' && memcmp(p, "\r\n\r\n", 4) == 0) {
        body = p + 4;
        break;
      }
    }
    if (body) break;
  }

  // Get websocket key.
  for (p = ws->buffer; p < body; ++p) {
    if (*p != '\n') continue;
    if (p + 18 < body && memcmp(p + 1, "Sec-WebSocket-Key:", 18) == 0) {
      key = p + 19;
      while (key < body && *key == ' ') key++;
      break;
    }
  }
  if (!key) return -1;
  keyend = key;
  while (keyend < body && *keyend > ' ') keyend++;

  // Compute response key.
  sha1_start(&ctx);
  sha1_update(&ctx, key, keyend - key);
  sha1_update(&ctx, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
  sha1_finish_base64(&ctx, response);

  // Send upgrade reply.
  sprintf(reply, 
          "HTTP/1.1 101 Switching Protocols\r\n"
          "Upgrade: websocket\r\n"
          "Connection: Upgrade\r\n"
          "Sec-WebSocket-Accept: %s\r\n\r\n", response);
  return write(ws->sock, reply, strlen(reply));
}

int websock_recv_fragment(struct websock *ws) {
  unsigned char masklen;
  int len;
  int rc;
  unsigned char mask[4];

  // Receive message header.
  rc = websock_read(ws, 2);
  if (rc <= 0) return rc;
  websock_mode(ws, WS_BLOCK);

  ws->opcode = ws->buffer[0];
  masklen = ws->buffer[1];
  if ((ws->opcode & WS_FIN) == 0) {
    fprintf(stderr, "websock: message fragments not supported\n");
    return -1;
  }

  // Receive extended length field.
  if ((masklen & WS_LEN) == 0x7E) {
    if (websock_read(ws, 2) < 0) return -1;
    len = (ws->buffer[0] << 8) | (ws->buffer[1] << 0);
  } else if ((masklen & WS_LEN) == 0x7F) {
    if (websock_read(ws, 8) < 0) return -1;
    len = (ws->buffer[4] << 24) | (ws->buffer[5] << 16) |
          (ws->buffer[6] << 8) | (ws->buffer[7] << 0);
  } else {
    len = masklen & WS_LEN;
  }

  // Receive masking key.
  if (masklen & 0x80) {
    if (websock_read(ws, 4) < 0) return -1;
    memcpy(mask, ws->buffer, 4);
  } else {
    memset(mask, 0, 4);
  }

  // Receive payload.
  if (len != 0) {
    int i;

    if (websock_read(ws, len) < 0) return -1;
    for (i = 0; i < len; ++i) ws->buffer[i] ^= mask[i % 4];
  }

  return 1;
}

int websock_send(struct websock *ws, int type,
                 void *data1, int size1, 
                 void *data2, int size2) {
  struct iovec iov[3];
  int n;
  int rc;
  unsigned char hdr[10];
  int size = size1 + size2;

  // Setup fragment header.
  iov[0].iov_base = &hdr;
  hdr[0] = type | WS_FIN;
  if (size < 0x7E) {
    hdr[1] = size;
    iov[0].iov_len = 2;
  } else if (size < 0x10000) {
    hdr[1] = 0x7E;
    hdr[2] = (size >> 8) & 0xFF;
    hdr[3] = (size >> 0) & 0xFF;
    iov[0].iov_len = 4;
  } else {
    hdr[1] = 0x7F;
    hdr[2] = 0;
    hdr[3] = 0;
    hdr[4] = 0;
    hdr[5] = 0;
    hdr[6] = (size >> 24) & 0xFF;
    hdr[7] = (size >> 16) & 0xFF;
    hdr[8] = (size >> 8) & 0xFF;
    hdr[9] = (size >> 0) & 0xFF;
    iov[0].iov_len = 10;
  }

  // Setup payload.
  n = 1;
  if (size1 > 0) {
    iov[n].iov_base = data1;
    iov[n].iov_len = size1;
    n++;
  }
  if (size2 > 0) {
    iov[n].iov_base = data2;
    iov[n].iov_len = size2;
    n++;
  }

  // Send fragment.
  websock_mode(ws, WS_BLOCK);
  rc = writev(ws->sock, iov, n);
  if (rc != size + iov[0].iov_len) {
    printf("writev %d returned %d\n", (int) (size + iov[0].iov_len), rc);
  }
  return rc;
}

int websock_recv(struct websock *ws, int mode) {
  int rc;

  for (;;) {
    websock_mode(ws, mode);

    rc = websock_recv_fragment(ws);
    if (rc <= 0) return rc;

    switch (ws->opcode & WS_OPCODE) {
      case WS_OP_TEXT:
      case WS_OP_BIN:
        return rc;

      case WS_OP_CLOSE:
        websock_send(ws, WS_OP_CLOSE, NULL, 0, NULL, 0);
        return -1;

      case WS_OP_PING:
        websock_send(ws, WS_OP_PONG, ws->buffer, ws->end - ws->buffer, NULL, 0);
        break;

      default:
        printf("unknown opcode %d\n", ws->opcode & WS_OPCODE);
        return -1;
    }
  }
}

