#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ucontext.h>
#include <unistd.h>

#define NAME_LEN 80
typedef struct {
  void *start;
  void *end;
  char rwxp[4];
  int is_register_context; // Is this a context from getcontext?
                           // If so, ignore the start_address,
                           //   end_address, and read/write/execute.
  int data_size;  // Size of DATA after this HEADER.
  char name[NAME_LEN];  // used only for debugging
} ckpt_segment_t;

int read_helper(int fd, void *start, int len);

int main(int argc, char *argv[]) {
  assert(argc == 2);

  char *filename = argv[1];
  int fd = open(filename, O_RDONLY);
  if (fd == -1) {perror("open");}

  // Read stored context first
  ckpt_segment_t segment;
  ucontext_t context;
  read_helper(fd, &segment, sizeof(ckpt_segment_t)); // Read header
  read_helper(fd, &context, segment.data_size);      // Read data
  printf("%p - %p         %s\n", segment.start, segment.end, segment.name);

  // Read stored memory segments
  while (read_helper(fd, &segment, sizeof(ckpt_segment_t)) == 0) {
    printf("%p - %p         %s\n", segment.start, segment.end, segment.name);
    lseek(fd, segment.data_size, SEEK_CUR);
  }
  close(fd);
}

int read_helper(int fd, void *start, int len) {
  int rc = read(fd, start, len);
  if (rc == -1) {
    perror("read");
    exit(1);
  }
  if (rc == 0) {
    return -1;
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
  return 0;
}
