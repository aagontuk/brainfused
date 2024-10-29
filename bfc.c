#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "bf_jit_x86_64.h"

#define TAP_SIZE 1048576

#define HALT 0
#define BF_RIGHT 1
#define BF_LEFT 2
#define BF_INC 3
#define BF_DEC 4
#define BF_OUT 5
#define BF_IN 6
#define BF_OPEN 7
#define BF_CLOSE 8

void gen_prologue(FILE *ofile) {
  fprintf(ofile,
    "section .text\n"
    "\tglobal _start\n\n"
    "_start:\n"
  );

  fprintf(ofile,
    "\tmov rdi, %d\n", TAP_SIZE
  );
  
  fprintf(ofile, "\tmov rsi, 1\n");

  fprintf(ofile,
    "\textern calloc\n"
    "\tcall calloc\n"
    "\tmov rsi, rax\n"
  );
}

void gen_epilogue(FILE *ofile) {
  fprintf(ofile,
    "\tmov rax, 60\n"
    "\txor rdi, rdi\n"
    "\tsyscall\n"
  );
}

int bf_aot_comp(unsigned char *code, FILE *ofile) {
  int loop_stack[100];
  int loop_stack_pos = 0;
  int loop_count = 0;
  gen_prologue(ofile);
  
  while(*code) {
    switch(*code) {
      case '>':
        fprintf(ofile, "\tinc rsi\n");
        break;
      
      case '<':
        fprintf(ofile, "\tdec rsi\n");
        break;
      
      case '+':
        fprintf(ofile, "\tinc byte [rsi]\n");
        break;
      
      case '-':
        fprintf(ofile, "\tdec byte [rsi]\n");
        break;
      
      case '.':
        fprintf(ofile,
          "\tmov rax, 1\n"
          "\tmov rdi, 1\n"
          "\tmov rdx, 1\n"
          "\tsyscall\n"
        );
        break;
      
      case ',':
        break;
      
      case '[':
        loop_stack[loop_stack_pos++] = loop_count;

        fprintf(ofile, "loop_start_%d:\n", loop_count);
        fprintf(ofile, "\tcmp byte [rsi], 0\n");
        fprintf(ofile, "\tje loop_end_%d\n", loop_count);
        loop_count++;
        break;
      
      case ']':
        loop_stack_pos--;
        int cur_loop_count = loop_stack[loop_stack_pos];
        fprintf(ofile, "\tcmp byte [rsi], 0\n");
        fprintf(ofile, "\tjne loop_start_%d\n", cur_loop_count);
        fprintf(ofile, "loop_end_%d:\n", cur_loop_count);
        break;

      default:
        break;
    }

    code++;
  }

  gen_epilogue(ofile);

  return 0;
}

uint32_t compute_pc_rel32(uint32_t from, uint32_t to) {
  if (to >= from)
    return to - from;
  else
    return ~(from - to) + 1;
}

void replace_bytes(uint8_t *buf, uint32_t offset, uint32_t value, int size) {
  for (int i = 0; i < size; i++) {
    buf[offset + i] = (value >> (i * 8)) & 0xff;
  }
}

void bf_jit_com_x86_64(unsigned char *code, int len) {
  struct jit_state state;
  
  state.buf = (uint8_t *)malloc(MAX_OFFSET);
  state.offset = 0;

  uint32_t open_bracket_off[MAX_NESTING];
  uint32_t open_br_off;
  uint32_t open_bracket_index = 0;

  // push callee saved registers
  emit_push(&state, RBX);
  emit_push(&state, RBP);
  emit_push(&state, R12);
  emit_push(&state, R13);
  emit_push(&state, R14);
  emit_push(&state, R15);

  for (int i = 0; i < len; i++) {
    switch(code[i]) {
      case '>':
        /*
         * Tape is supplied as a pointer by the called in rdi
         * 
         * inc rdi
         */
        emit1(&state, 0x48);
        emit1(&state, 0xff);
        emit1(&state, 0xc7);
        break;

      case '<':
        // dec rdi
        emit1(&state, 0x48);
        emit1(&state, 0xff);
        emit1(&state, 0xcf);
        break;

      case '+':
        // inc byte [rdi]
        emit1(&state, 0xfe);
        emit1(&state, 0x07);
        break;

      case '-':
        // dec byte [rdi]
        emit1(&state, 0xfe);
        emit1(&state, 0x0f);
        break;
      
      case '.':
        // mov rbx, rdi ;save rdi for write syscall
        emit1(&state, 0x48);
        emit1(&state, 0x89);
        emit1(&state, 0xfb);

        // mov al, byte [rbx] ;arg2 char to write
        emit1(&state, 0x8a);
        emit1(&state, 0x03);

        // mov rax, 1 ;syscall number
        emit1(&state, 0xb8);
        emit4(&state, 0x01000000);

        // mov rdi, 1 ; arg1 stdout
        emit1(&state, 0xbf);
        emit4(&state, 0x01000000);

        // mov rdx, 1; arg3 size
        emit1(&state, 0xba);
        emit4(&state, 0x01000000);

        // syscall
        emit1(&state, 0x0f);
        emit1(&state, 0x05);

        // mov rdi, rbx
        emit1(&state, 0x48);
        emit1(&state, 0x89);
        emit1(&state, 0xdf);
        break;
        
      case '[':
        // cmp byte [rdi], 0
        emit1(&state, 0x80);
        emit1(&state, 0x3f);
        emit1(&state, 0x00);
        open_bracket_off[open_bracket_index++] = state.offset;

        // jz 0
        emit1(&state, 0x0f);
        emit1(&state, 0x84);
        emit4(&state, 0x00000000);
        break;

      case ']':
        open_br_off = open_bracket_off[--open_bracket_index];
        
        // cmp byte [rdi], 0
        emit1(&state, 0x80);
        emit1(&state, 0x3f);
        emit1(&state, 0x00);

        uint32_t jmp_open_from = state.offset + 6;
        uint32_t jmp_open_to = open_br_off + 6;
        uint32_t jmp_open_off = compute_pc_rel32(jmp_open_from, jmp_open_to);

        // jnz jmp_open_off
        emit1(&state, 0x0f);
        emit1(&state, 0x85);
        emit4(&state, jmp_open_off);

        uint32_t jmp_close_from = open_br_off + 6;
        uint32_t jmp_close_to = state.offset;
        uint32_t jmp_close_off = compute_pc_rel32(jmp_close_from, jmp_close_to);

        // replace off
        replace_bytes(state.buf, open_br_off + 2, jmp_close_off, 4);
        break;
    }
  }

  emit_pop(&state, R15);
  emit_pop(&state, R14);
  emit_pop(&state, R13);
  emit_pop(&state, R12);
  emit_pop(&state, RBP);
  emit_pop(&state, RBX);

  void *jitted_code = mmap(NULL, state.offset, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  memcpy(jitted_code, state.buf, state.offset);
  typedef void (*jit_fn)(uint64_t *);
  jit_fn fn = (jit_fn)jitted_code;
  uint64_t *tape = (uint64_t *)calloc(TAP_SIZE, sizeof(uint64_t));
  fn(tape);
}

int main(int argc, char *argv[]) {
  FILE *ofile;

  struct option longopts[] = {
    {.name = "aot", .val = 'a', },
    {.name = "jit", .val = 'j', },
  };

  bool aot = false;

  int opt;
  while ((opt = getopt_long(argc, argv, "aj", longopts, NULL)) != -1) {
    switch(opt) {
      case 'a':
        aot = true;
        break;
      case 'j':
        break;
      default:
        printf("Unkown option\n");
        return 1;
    }
  }

  if (optind >= argc) {
    printf("Error: No input file\n");
    return 1;
  }

  FILE *ifile = fopen(argv[optind++], "r");
  if(!ifile) {
    printf("Error: Could not open file\n");
    return 1;
  }

  if (aot) {
    if (optind < argc) {
      ofile = fopen(argv[optind], "w");
      if(!ofile) {
        printf("Error: Could not open file\n");
        return 1;
      }
    }
    else {
      ofile = stdout;
    }
  }

  fseek(ifile, 0, SEEK_END);
  long length = ftell(ifile);
  fseek(ifile, 0, SEEK_SET);

  unsigned char *code = (unsigned char *)malloc(length + 1);
  int rv = fread(code, 1, length, ifile);
  if (rv != length)
    return 1;
  code[length] = '\0';
  fclose(ifile);

  (void)ofile;
  // bf_aot_comp(code, ofile);
  bf_jit_com_x86_64(code, length);
  
  return 0;
}
