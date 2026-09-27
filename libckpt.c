#include <signal.h>
#define _GNU_SOURCE /* Required for 'constructor' attribute (GNU extension) */
#include <stdio.h>

void my_constructor();
void signal_handler(int signal);

void __attribute__((constructor))
my_constructor() {
  signal(SIGUSR2, &signal_handler);
  fprintf(stderr, "*************************************\n"
                  "***  We are running, using libskpt.c ***\n"
                  "*************************************\n");
}

void signal_handler(int signal) {
  printf("SIGUSR2 triggered!\n");
}
