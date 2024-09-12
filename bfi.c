#include <stdio.h>
#include <stdlib.h>

#define TAP_SIZE 1048576

int interp(char *code) {
  char *tape = (char *)calloc(TAP_SIZE, 1);
  char *ptr = tape;

  while(code) {
    switch(*code) {
      case '>':
        ptr++;
        break;
      
      case '<':
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
  fread(code, 1, length, file);
  code[length] = '\0';
  fclose(file);

  interp(code);
  return 0;
}
