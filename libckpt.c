#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ucontext.h>
#define _GNU_SOURCE /* Required for 'constructor' attribute (GNU extension) */
#define CKPT_IMAGE_FILE "myckpt"
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

void my_constructor();
void signal_handler(int signal);
int proc_self_maps(ckpt_segment_t proc_maps[]);
void save(const char *filename, ckpt_segment_t pm_line[]);
void write_helper(int fd, void *start, int len);

void __attribute__((constructor))
my_constructor() {
  signal(SIGUSR2, &signal_handler);
  fprintf(stderr, "*************************************\n"
                  "***  We are running, using libskpt.c ***\n"
                  "*************************************\n");
}


void signal_handler(int signal) {
  ckpt_segment_t segments[1000];
  ckpt_segment_t *segment_ptr = segments;

  ucontext_t context;  // A context includes the register values
  static int is_restart; // static local variables are stored in data segment
  is_restart = 0;
  getcontext(&context);
  if (is_restart == 1) {
    is_restart = 0;
    printf("*** RESTARTING ...\n");
    return;
  } else {
    printf("*** WRITING CHECKPOINT IMAGE FILE ...\n");
  }
  // When we set 'is_restart' is important:
  //   is_restart = 0; // before entering 'getcontext(&context)'
  //   is_restart = 1; // after returning from 'getcontext(&context)'
  //                   // and before the memory segments are written out
  // This guarantees if we enter getcontext(&context) directly (during ckpt),
  //   then when getcontext returns, 'is_restart == 0'.
  // If we enter getcontext(&context) via setcontext (during restart),
  //   then when getcontext returns, 'is_restart == 1'.
  is_restart = 1;

  *segment_ptr = (ckpt_segment_t){.start=&context,.is_register_context=1,.data_size=sizeof(ucontext_t),.name="Context"};

  segment_ptr++;
  assert(proc_self_maps(segment_ptr) == 0);

  save(CKPT_IMAGE_FILE, segments);
}


int match_one_line(int proc_maps_fd,
    ckpt_segment_t *segment, char *filename) {
  segment->is_register_context = 0;

  unsigned long int start, end;
  char rwxp[4];
  char tmp[10];
  int tmp_stdin = dup(0); // Make a copy of stdin
  dup2(proc_maps_fd, 0); // Duplicate proc_maps_fd to stdin
                         // scanf() reads stdin (fd:0). It's a dup of proc_maps_fd ('same struct file').
  int rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
      &start, &end, rwxp, filename);
  assert(fseek(stdin, 0, SEEK_CUR) == 0);
  // But fseek() didn't remove the EOF indicator on stdin.
  clearerr(stdin);  // remove EOF indicator on stdin for future scanf() calls.
  dup2(tmp_stdin, 0); // Restore original stdin; proc_maps_fd offset was advanced.
  close(tmp_stdin);
  if (rc == EOF || rc == 0) { 
    segment -> start = NULL;
    segment -> end = NULL;
    return EOF;
  }
  if (rc == 3) { // if no filename on /proc/self/maps line:
    strncpy(segment -> name,
        "ANONYMOUS_SEGMENT", strlen("ANONYMOUS_SEGMENT")+1);
  } else {
    assert( rc == 4 );
    strncpy(segment -> name, filename, NAME_LEN-1);
    segment ->name[NAME_LEN-1] = '\0';
  }
  segment -> start = (void *)start;
  segment -> end = (void *)end;
  memcpy(segment ->rwxp, rwxp, 4);
  segment->data_size = end - start;
  return 0;
}

// This might cause scanf to call mmap().  If this is a problem,
//   you could call it a second time, and scanf() can re-use the
//   previous memory, instead of mmap'ing more.
int proc_self_maps(ckpt_segment_t segments[]) {
  // NOTE:  fopen calls malloc() and potentially extends the heap segment.
  int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
  char filename[NAME_LEN]; // for debugging
  int i = 0;
  int rc = -2; // any value that will not terminate the 'for' loop.
  for (i = 0; rc != EOF; i++) {
    rc = match_one_line(proc_maps_fd, &segments[i], filename);
#ifdef DEBUG
    if (rc == 0) {
      printf("proc_self_maps: filename: %s\n", filename); // for debugging
    }
#endif
  } close(proc_maps_fd); return 0;
}

void save(const char *filename, ckpt_segment_t segments[]) {
  int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
  if (fd == -1) {perror("open");}

  for (; segments->start != NULL; segments++) {
    // Skip unreadable memory segments
    if (segments->is_register_context == 0) {
        if (segments->rwxp[0] != 'r') {
          continue;
        }
        if (strstr(segments->name, "[vvar") != NULL) {
          continue;
        }
        if (strstr(segments->name, "[vsyscall]") != NULL) {
          continue;
        }
    }

    write_helper(fd, segments, sizeof(ckpt_segment_t));     // Save header
    write_helper(fd, segments->start, segments->data_size); // Save data
}

  close(fd);
}

void write_helper(int fd, void *start, int len) {
  int rc = write(fd, start, len);
  if (rc == -1) {
    perror("write");
    exit(1);
  }
  while (rc < len) {
    int rc2 = write(fd, start+rc, len-rc);
    if (rc2 == -1) {
      perror("write");
      exit(1);
    }
    rc += rc2;
  }
  assert(rc == len);
}
