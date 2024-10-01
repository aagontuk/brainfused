#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>

#define TAP_SIZE 1048576
#define MAX_LOOPS 1024

#define HALT 0
#define BF_RIGHT 1
#define BF_LEFT 2
#define BF_INC 3
#define BF_DEC 4
#define BF_OUT 5
#define BF_IN 6
#define BF_OPEN 7
#define BF_CLOSE 8

struct pstats {
  uint64_t right;
  uint64_t left;
  uint64_t inc;
  uint64_t dec;
  uint64_t in;
  uint64_t out;
};

struct loop_info {
  int start;
  int end;
  int count;
};

static bool profile = false;
static struct pstats *stats;

int compare(const void *a, const void *b) {
  struct loop_info *l1 = (struct loop_info *)a;
  struct loop_info *l2 = (struct loop_info *)b;

  return l1->count - l2->count;
}

int get_info_index(struct loop_info linfo[], int total_loops, int start) {
  for (int i = 0; i < total_loops; i++) {
    if (linfo[i].start == start)
      return i;
  }

  return -1;
}

bool is_simple_loop(unsigned char *program, struct loop_info *linfo) {
  int start = linfo->start;
  int end = linfo->end;
  int pointer_movement = 0;
  int p0_change = 0;
  bool has_io = false;

  for (int i = start + 1; i < end; i++) {
    switch (program[i]) {
      case '>': pointer_movement++; break;
      case '<': pointer_movement--; break;
      case '+': if (pointer_movement == 0) p0_change++; break;
      case '-': if (pointer_movement == 0) p0_change--; break;
      case '.':
      case ',': has_io = true; break;
    }
  }

  // if simple print it
  if (!has_io && pointer_movement == 0 && (p0_change == 1 || p0_change == -1))
    return true;

  return false;
}

void print_loop(unsigned char *program, struct loop_info *linfo) {
  for (int i = linfo->start; i <= linfo->end; i++) {
    if (program[i] == '>' || program[i] == '<'
        || program[i] == '+' || program[i] == '-'
        || program[i] == '.' || program[i] == ','
        || program[i] == '[' || program[i] == ']') {
      
      printf("%c", program[i]);
    }
  }

  printf(" => %d\n", linfo->count);
}

int bf_interp(unsigned char *program) {
  char *tape = (char *)calloc(TAP_SIZE, 1);
  char *ptr = tape;
  unsigned char *code = program;
  struct loop_info loops[MAX_LOOPS];
  struct loop_info simple_loops[MAX_LOOPS];
  struct loop_info not_simple_loops[MAX_LOOPS];
  int loop_index;
  int loop_start = 0;
  int loop_end = 0;
  int loop_stack = 0;
  int total_loops = 0;
  int total_simple_loops = 0;
  int total_not_simple_loops = 0;
  bool is_inner = false;

  while(*code) {
    switch(*code) {
      case '>':
        if ((ptr + 1) > (tape + TAP_SIZE)) {
            fprintf(stderr, "error: tap overflow\n");
            return -1;
        }

        if (profile)
          stats->right++;

        ptr++;
        break;
      
      case '<':
        if ((ptr - 1) < tape) {
            fprintf(stderr, "error: tap underflow\n");
            return -1;
        }
        
        if (profile)
          stats->left++;
        
        ptr--;
        break;
      
      case '+':
        if (profile)
          stats->inc++;
        
        (*ptr)++;
        break;
      
      case '-':
        if (profile)
          stats->dec++;
        
        (*ptr)--;
        break;
      
      case '.':
        if (profile)
          stats->out++;

        putchar(*ptr);
        break;
      
      case ',':
        if (profile)
          stats->in++;
        
        *ptr = getchar();
        getchar();
        break;
      
      case '[':
        // skip the loop
        if(!*ptr) {
          int loop = 1;
          while(loop) {
            code++;
            if(*code == '[') loop++;
            if(*code == ']') loop--;
          }
        }
        else {
          if (profile) {
            loop_stack++;
            is_inner = false;
            loop_start = code - program;
          }
        }
        break;
      
      case ']':
        if (profile) {
          loop_stack--;

          if (loop_stack == 0) {
            loop_end = (int)(code - program);
            loop_index = get_info_index(loops, total_loops, loop_start);
            if (loop_index != -1) {
              loops[loop_index].count++;
            }
            else {
              loops[total_loops].start = loop_start;
              loops[total_loops].end = loop_end;
              loops[total_loops++].count = 1;
            }

            // printf("simple loop: [%d, %d]\n", loop_start, loop_end);
          }
          else {
            if (!is_inner) {
              // inner loop
              loop_end = (int)(code - program);
              loop_index = get_info_index(loops, total_loops, loop_start);
              if (loop_index != -1) {
                loops[loop_index].count++;
              }
              else {
                loops[total_loops].start = loop_start;
                loops[total_loops].end = loop_end;
                loops[total_loops++].count = 1;
              }
              // printf("simple loop: [%d, %d]\n", loop_start, loop_end);
              is_inner = true;
            }
          }
        }

        // jump to matching [
        if(*ptr) {
          int loop = 1;
          while(loop) {
            code--;
            if(*code == '[') loop--;
            if(*code == ']') loop++;
          }
        }
        break;

      default:
        break;
    }

    code++;
  }

  // print statistics
  if (profile) {
    printf("\n\n ====== PROFILE ======\n\n");
    printf("> => %lu\n", stats->right);
    printf("< => %lu\n", stats->left);
    printf("+ => %lu\n", stats->inc);
    printf("- => %lu\n", stats->dec);
    printf(", => %lu\n", stats->in);
    printf(". => %lu\n\n", stats->out);
    for (int i = 0; i < total_loops; i++) {
      if (is_simple_loop(program, &loops[i]))
        simple_loops[total_simple_loops++] = loops[i];
      else
        not_simple_loops[total_not_simple_loops++] = loops[i];
    }

    qsort(simple_loops, total_simple_loops, sizeof(struct loop_info), compare);
    qsort(not_simple_loops, total_not_simple_loops, sizeof(struct loop_info), compare);

    // print loops
    printf("Simple inner loops:\n");
    for (int i = 0; i < total_simple_loops; i++) {
      print_loop(program, &simple_loops[i]);
    }
    
    printf("\nOther inner loops:\n");
    for (int i = 0; i < total_not_simple_loops; i++) {
      print_loop(program, &not_simple_loops[i]);
    }
  }

  return 0;
}

unsigned char char_to_cmd(char c) {
  switch(c) {
    case '>':
      return BF_RIGHT;
    case '<':
      return BF_LEFT;
    case '+':
      return BF_INC;
    case '-':
      return BF_DEC;
    case '.':
      return BF_OUT;
    case ',':
      return BF_IN;
    case '[':
      return BF_OPEN;
    case ']':
      return BF_CLOSE;
    case '\0':
      return '\0';
    default:
      return 255;
  }
}

unsigned char *translate(unsigned char *code, int len) {
  unsigned char *buf = (unsigned char *)malloc(len);
  int buf_len = 0;
  while(*code) {
    unsigned char ch = char_to_cmd(*code);
    if (ch != 255) {
      buf[buf_len++] = ch;
    }
    code++;
  }
  
  return buf;
}

int interp_cgoto(unsigned char *code_org, int len) {
  char *tape = (char *)calloc(TAP_SIZE, 1);
  char *ptr = tape;
  unsigned char *code;

  code = translate(code_org, len);

  static void *cmds[] = {
    &&halt, &&right, &&left, &&inc, &&dec, &&out, &&in, &&open, &&close
  };
    
  goto *cmds[*code];

  while(1) {
    right:
      if ((ptr + 1) > (tape + TAP_SIZE)) {
          fprintf(stderr, "error: tap overflow\n");
          return -1;
      }
      ptr++;
      code++;
      goto *cmds[*code];

    left:
      if ((ptr - 1) < tape) {
          fprintf(stderr, "error: tap underflow\n");
          return -1;
      }
      ptr--;
      code++;
      goto *cmds[*code];

    inc:
      (*ptr)++;
      code++;
      goto *cmds[*code];

    dec:
      (*ptr)--;
      code++;
      goto *cmds[*code];

    out:
      putchar(*ptr);
      code++;
      goto *cmds[*code];

    in:
      *ptr = getchar();
      code++;
      goto *cmds[*code];

    open:
      if(!*ptr) {
        int loop = 1;
        while(loop) {
          code++;
          if(*code == BF_OPEN) loop++;
          if(*code == BF_CLOSE) loop--;
        }
      }
      code++;
      goto *cmds[*code];

    close:
      if(*ptr) {
        int loop = 1;
        while(loop) {
          code--;
          if(*code == BF_OPEN) loop--;
          if(*code == BF_CLOSE) loop++;
        }
      }
      code++;
      goto *cmds[*code];
    
    halt:
      break;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  struct option longopts[] = {
    { .name = "interp", .val = 'i', },
    { .name = "cgoto", .val = 'g', },
    { .name = "profile", .val = 'p', },
  };

  bool cgoto = false;
  bool interp = false;

  int opt;
  while ((opt = getopt_long(argc, argv, "igp", longopts, NULL)) != -1) {
    switch(opt) {
      case 'i':
        interp = true;
        break;
      case 'g':
        cgoto = true;
        break;
      case 'p':
        profile = true;
        break;
      default:
        printf("Unkown option\n");
        return 1;
    }
  }

  stats = (struct pstats *)calloc(1, sizeof(struct pstats));

  FILE *file = fopen(argv[optind], "r");
  if(!file) {
    printf("Error: Could not open file\n");
    return 1;
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  unsigned char *code = (unsigned char *)malloc(length + 1);
  int rv = fread(code, 1, length, file);
  if (rv != length)
    return 1;
  code[length] = '\0';
  fclose(file);

  if (interp)
    bf_interp(code);
  else if (cgoto)
    interp_cgoto(code, length);
  
  return 0;
}
