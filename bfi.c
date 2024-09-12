#include <stdio.h>
#include <stdlib.h>

#define TAP_SIZE 1048576

int interp(char *code) {
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

int main(int argc, char *argv[]) {
  if(argc < 2) {
    printf("Usage: %s <file>\n", argv[0]);
    return 1;
  }

  FILE *file = fopen(argv[1], "r");
  if(!file) {
    printf("Error: Could not open file\n");
    return 1;
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *code = (char *)malloc(length + 1);
  int rv = fread(code, 1, length, file);
  if (rv != length)
    return 1;
  code[length] = '\0';
  fclose(file);

  interp(code);
  return 0;
}
