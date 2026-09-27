#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  setenv("LD_PRELOAD", "./libckpt.so", 1);
  printf("arg count: %d\n", argc);
  printf("arg 0: %s\n", argv[0]);
  printf("arg 1: %s\n", argv[1]);
  printf("arg 2: %s\n", argv[2]);
  execvp(argv[1], &argv[1]);
}
