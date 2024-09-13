#ifndef BF_JIT_X86_64_H
#define BF_JIT_X86_64_H

static inline void emit(unsigned char *buf, unsigned char byte) {
  *buf = byte;
  buf++;
}

#endif
