#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <unistd.h>

#define NAME_LEN 80
#define CKPT_IMAGE_FILE "myckpt"

typedef struct {
  void *start;
  void *end;
  char rwxp[4];
  int is_register_context; // Is this a context from getcontext?
                           // If so, ignore the start_address,
                           //   end_address, and read/write/execute.
  int data_size;           // Size of DATA after this HEADER.
  char name[NAME_LEN];     // used only for debugging
} ckpt_segment_t;

/*
 * Restore memory segments and then registers to switch to the restored process
 */
void restart(char *image_file);

/*
 * Reads exactly len from fd to start, unless EOF is reached
 * Return 0 if reached end of file, otherwise return the length read
 */
int read_helper(int fd, void *start, int len);

int main(int argc, char *argv[]) {
  // Allocate memory at a low address for the stack of the restart process so
  // it's unlikely to conflict with the stack of the to-be-restored process
  void *addr = mmap((void *)0x6000000, 0x1000000, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
  if (addr == MAP_FAILED) {
    perror("mmap");
  }
  void *stack_ptr = (void *)(0x6000000 + 0x1000000 - 16);
  asm volatile("mov %0,%%rsp;" : : "g"(stack_ptr) : "memory");

  if (argc == 2) {
    char *image_file = argv[1];
    restart(image_file);
  } else if (argc == 1) {
    restart(CKPT_IMAGE_FILE);
  } else {
    fprintf(stderr, "Restart works with at most 1 image file as argument\n");
  }
}

void restart(char *image_file) {
  int fd = open(image_file, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  // First, load context into buffer for setcontext() later
  // By project convention, the segment always stores the register context
  ckpt_segment_t seg;
  ucontext_t context;
  if (read_helper(fd, &seg, sizeof(seg)) == 0) {
    fprintf(stderr, "restart: %s is empty\n", image_file);
    exit(1);
  }
  assert(seg.is_register_context && seg.data_size == sizeof(ucontext_t));
  read_helper(fd, &context, seg.data_size);

  // Restore each memory segment at its original address
  while (read_helper(fd, &seg, sizeof(ckpt_segment_t)) != 0) {
    // MAP_FIXED discards anything that overlaps with the new mapping,
    // this is to prevent conflicts with the old stack of restart
    void *addr =
        mmap(seg.start, seg.data_size, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED) {
      perror("mmap segment");
      fprintf(stderr, "Failed on %p-%p  %s\n", seg.start, seg.end, seg.name);
      exit(1);
    }
    read_helper(fd, addr, seg.data_size);
  }
  close(fd);

  // Pull the trigger.
  setcontext(&context);
  perror("setcontext"); // NOOOOOOOOOOO
  exit(1);
}

int read_helper(int fd, void *start, int len) {
  int rc = read(fd, start, len);
  if (rc == -1) {
    perror("read");
    exit(1);
  }
  if (rc == 0) {
    return 0;
  }
  while (rc < len) {
    int rc2 = read(fd, (char *)start + rc, len - rc);
    if (rc2 == -1) {
      perror("read");
      exit(1);
    }
    rc += rc2;
  }
  assert(rc == len);
  return rc;
}
