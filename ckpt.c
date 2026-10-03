#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  setenv("LD_PRELOAD", "./libckpt.so", 1);
  execvp(argv[1], &argv[1]);
}
