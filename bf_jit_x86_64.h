#ifndef BF_JIT_X86_64_H
#define BF_JIT_X86_64_H

#include <string.h>
#include <assert.h>
#include <inttypes.h>

#define MAX_OFFSET 1048576
#define MAX_NESTING 100

#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RIP 5
#define RSI 6
#define RDI 7
#define R8  8
#define R9  9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15

struct jit_state {
  uint8_t *buf;
  uint32_t offset;
};

static inline void
emit_bytes(struct jit_state *state, void *data, uint32_t len)
{
    memcpy(state->buf + state->offset, data, len);
    state->offset += len;
}

static inline void
emit1(struct jit_state *state, uint8_t x)
{
    emit_bytes(state, &x, sizeof(x));
}

static inline void
emit2(struct jit_state *state, uint16_t x)
{
    emit_bytes(state, &x, sizeof(x));
}

static inline void
emit4(struct jit_state *state, uint32_t x)
{
    emit_bytes(state, &x, sizeof(x));
}

  static inline void
emit_rex(struct jit_state *state, int w, int r, int x, int b)
{
    assert(!(w & ~1));
    assert(!(r & ~1));
    assert(!(x & ~1));
    assert(!(b & ~1));
    emit1(state, 0x40 | (w << 3) | (r << 2) | (x << 1) | b);
}

static inline void
emit_basic_rex(struct jit_state *state, int w, int src, int dst)
{
    if (w || (src & 8) || (dst & 8)) {
        emit_rex(state, w, !!(src & 8), 0, !!(dst & 8));
    }
}

static inline void
emit_push(struct jit_state *state, int r)
{
    emit_basic_rex(state, 0, 0, r);
    emit1(state, 0x50 | (r & 7));
}

static inline void
emit_pop(struct jit_state *state, int r)
{
    emit_basic_rex(state, 0, 0, r);
    emit1(state, 0x58 | (r & 7));
}

static inline void emit(unsigned char *buf, unsigned char byte) {
  *buf = byte;
  buf++;
}

#endif
