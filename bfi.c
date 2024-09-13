#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>

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

int bf_interp(unsigned char *code) {
  char *tape = (char *)calloc(TAP_SIZE, 1);
  char *ptr = tape;

  while(*code) {
    switch(*code) {
      case '>':
        if ((ptr + 1) > (tape + TAP_SIZE)) {
            fprintf(stderr, "error: tap overflow\n");
            return -1;
        }
        ptr++;
        break;
      
      case '<':
        if ((ptr - 1) < tape) {
            fprintf(stderr, "error: tap underflow\n");
            return -1;
        }
        ptr--;
        break;
      
      case '+':
        (*ptr)++;
        break;
      
      case '-':
        (*ptr)--;
        break;
      
      case '.':
        putchar(*ptr);
        break;
      
      case ',':
        *ptr = getchar();
        break;
      
      case '[':
        if(!*ptr) {
          int loop = 1;
          while(loop) {
            code++;
            if(*code == '[') loop++;
            if(*code == ']') loop--;
          }
        }
        break;
      
      case ']':
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
  };

  bool cgoto = false;
  bool interp = false;

  int opt;
  while ((opt = getopt_long(argc, argv, "ig", longopts, NULL)) != -1) {
    switch(opt) {
      case 'i':
        interp = true;
        break;
      case 'g':
        cgoto = true;
        break;
      default:
        printf("Unkown option\n");
        return 1;
    }
  }

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
