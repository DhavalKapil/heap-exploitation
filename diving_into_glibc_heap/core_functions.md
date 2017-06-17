# Core functions

## void * _int_malloc (mstate av, size_t bytes)

1. Updates `bytes` to take care of alignments, etc.
2. Checks if `av` is NULL or not.
3. In the case of absence of usable arena (when `av` is NULL), calls `sysmalloc` to obtain chunk using mmap. If successful, calls `alloc_perturb`. Returns the pointer.  
4. * If size falls in the fastbin range:
     1. Get index into the fastbin array to access an appropriate bin according to the request size.
     2. Removes the first chunk in that bin and make `victim` point to it.
     3. If `victim` is NULL, move on to the next case (smallbin).
     4. If `victim` is not NULL, check the size of the chunk to ensure that it belongs to that particular bin. An error ("malloc(): memory corruption (fast)") is thrown otherwise.
     5. Calls `alloc_perturb` and then returns the pointer.

   * If size falls in the smallbin range:
     1. Get index into the smallbin array to access an appropriate bin according to the request size.
     2. If there are no chunks in this bin, move on to the next case. This is checked by comparing the pointers `bin` and `bin->bk`.
     3. `victim` is made equal to `bin->bk` (the last chunk in the bin). If it is NULL (happens during `initialization`), call `malloc_consolidate` and skip this complete step of checking into different bins. 
     4. Otherwise, when `victim` is non NULL, check if `victim->bk->fd` and `victim` are equal or not. If they are not equal, an error ("malloc(): smallbin double linked list corrupted") is thrown.
     5. Sets the PREV\_INSUSE bit for the next chunk (in memory, not in the doubly linked list) for `victim`.
     6. Remove this chunk from the bin list.
     7. Set the appropriate arena bit for this chunk depending on `av`.
     8. Calls `alloc_perturb` and then returns the pointer.

   * If size does not fall in the smallbin range:
     1. Get index into the largebin array to access an appropriate bin according to the request size.
     2. See if `av` has fastchunks or not. This is done by checking the `FASTCHUNKS_BIT` in `av->flags`. If so, call `malloc_consolidate` on `av`.

5. If no pointer has yet been returned, this signifies one or more of the following cases:
  1. Size falls into 'fastbin' range but no fastchunk is available.
  2. Size falls into 'smallbin' range but no smallchunk is available (calls `malloc_consolidate` during initialization).
  3. Size falls into 'largbin' range.

6. Next, unsorted chunks are checked and traversed chunks are placed into bins. This is the only place where chunks are placed into bins. Iterate the unsorted bin from the 'TAIL'.
  1. `victim` points to the current chunk being considered.
  2. Check if `victim`'s chunk size is within minimum (`2*SIZE_SZ`) and maximum (`av->system_mem`) range. Throw an error ("malloc(): memory corruption") otherwise.
  3. If (size of requested chunk falls in smallbin range) and (`victim` is the last remainder chunk) and (it is the only chunk in the unsorted bin) and (the chunks size >= the one requested):
    Break the chunk into 2 chunks:
     * The first chunk matches the size requested and is returned.
     * Left over chunk becomes the new last remainder chunk. It is inserted back into the unsorted bin.
       1. Set `chunk_size` and `chunk_prev_size` fields appropriately for both chunks.
       2. The first chunk is returned after calling `alloc_perturb`.
  4. If the above condition is false, control reaches here. Remove `victim` from the unsorted bin. If the size of `victim` matches the size requested exactly, return this chunk after calling `alloc_perturb`.
  5. If `victim`'s size falls in smallbin range, add the chunk in the appropriate smallbin at the `HEAD`.
  6. Else insert into appropriate largebin while maintaining sorted order:
    * First checks the last chunk (smallest). If `victim` is smaller than the last chunk, insert it at the last.
    * Otherwise, loop to find a chunk with size >= size of `victim`. If size is exactly same, always insert in the second position.
  7. Repeat this whole step a maximum of `MAX_ITERS` (10000) times or till all chunks in unsorted bin get exhausted.

7. After checking unsorted chunks, check if requested size does not fall in the smallbin range, if so then check largebins.
  1. Get index into largebin array to access an appropriate bin according to the request size.
  2. If the size of the largest chunk (the first chunk in the bin) is greater than the size requested:
    1. Iterate from 'TAIL' to find a chunk (`victim`) with the smallest size >= the requested size.
    2. Call `unlink` to remove the `victim` chunk from the bin.
    3. Calculate `remainder_size` for the `victim`'s chunk (this will be `victim`'s chunk size - requested size).
    4. If this `remainder_size` >= `MINSIZE` (the minimum chunk size including the headers), split the chunk into two chunks. Otherwise, the entire `victim` chunk will be returned. Insert the remainder chunk in the unsorted bin (at the 'TAIL' end). A check is made in unsorted bin whether `unsorted_chunks(av)->fd->bk == unsorted_chunks(av)`. An error is thrown otherwise ("malloc(): corrupted unsorted chunks").
    5. Return the `victim` chunk after calling `alloc_perturb`.

8. Till now, we have checked unsorted bin and also the respective fast, small or large bin. Note that a single bin (fast or small) was checked using the **exact** size of the requested chunk. Repeat the following steps till all bins are exhausted:
  1. The index into bin array is incremented to check the next bin.
  2. Use `av->binmap` map to skip over bins that are empty.
  3. `victim` is pointed to the 'TAIL' of the current bin.
  4. Using the binmap ensures that if a bin is skipped (in the above 2nd step), it is definitely empty. However, it does not ensure that all empty bins will be skipped. Check if the victim is empty or not. If empty, again skip the bin and repeat the above process (or 'continue' this loop) till we arrive at a nonempty bin.
  5. Split the chunk (`victim` points to the last chunk of a nonempty bin) into two chunks. Insert the remainder chunk in unsorted bin (at the 'TAIL' end). A check is made in the unsorted bin whether `unsorted_chunks(av)->fd->bk == unsorted_chunks(av)`. An error is thrown otherwise ("malloc(): corrupted unsorted chunks 2").
  6. Return the `victim` chunk after calling `alloc_perturb`.

9. If still no empty bin is found, 'top' chunk will be used to service the request:
  1. `victim` points to `av->top`.
  2. If size of 'top' chunk >= 'requested size' + `MINSIZE`, split it into two chunks. In this case, the remainder chunk becomes the new 'top' chunk and the other chunk is returned to the user after calling `alloc_perturb`.
  3. See if `av` has fastchunks or not. This is done by checking the `FASTCHUNKS_BIT` in `av->flags`. If so, call `malloc_consolidate` on `av`. Return to step 6 (where we check unsorted bin).
  4. If `av` does not have fastchunks, call `sysmalloc` and return the pointer obtained after calling `alloc_perturb`.

## __libc_malloc (size_t bytes)

1. Calls `arena_get` to get an `mstate` pointer.
2. Calls `_int_malloc` with the arena pointer and the size.
3. Unlocks the arena.
4. Before returning the pointer to the chunk, one of the following should be true:
   * Returned pointer is NULL
   * Chunk is MMAPPED
   * Arena for chunk is the same as the one found in 1.

## _int_free (mstate av, mchunkptr p, int have_lock)

1. Check whether `p` is before `p + chunksize(p)` in the memory (to avoid wrapping). An error ("free(): invalid pointer") is thrown otherwise.
2. Check whether the chunk is at least of size `MINSIZE` or a multiple of `MALLOC_ALIGNMENT`. An error ("free(): invalid size") is thrown otherwise.
3. If the chunk's size falls in fastbin list:
   1. Check if next chunk's size is between minimum and maximum size (`av->system_mem`), throw an error ("free(): invalid next size (fast)") otherwise.
   2. Calls `free_perturb` on the chunk.
   3. Set `FASTCHUNKS_BIT` for `av`.
   4. Get index into fastbin array according to chunk size.
   5. Check if the top of the bin is not the chunk we are going to add. Otherwise, throw an error ("double free or corruption (fasttop)").
   6. Check if the size of the fastbin chunk at the top is the same as the chunk we are adding. Otherwise, throw an error ("invalid fastbin entry (free)").
   7. Insert the chunk at the top of the fastbin list and return.
4. If the chunk is not mmapped:
   1. Check if the chunk is the top chunk or not. If yes, an error ("double free or corruption (top)") is thrown.
   2. Check whether next chunk (by memory) is within the boundaries of the arena. If not, an error ("double free or corruption (out)") is thrown.
   3. Check whether next chunk's (by memory) previous in use bit is marked or not. If not, an error ("double free or corruption (!prev)") is thrown.
   4. Check whether the size of next chunk is between the minimum and maximum size (`av->system_mem`). If not, an error ("free(): invalid next size (normal)") is thrown.
   5. Call `free_perturb` on the chunk.
   6. If previous chunk (by memory) is not in use, call `unlink` on the previous chunk.
   7. If next chunk (by memory) is not top chunk:
      1. If next chunk is not in use, call `unlink` on the next chunk.
      2. Merge the chunk with previous, next (by memory), if any is free and add it to the head of unsorted bin. Before inserting, check whether `unsorted_chunks(av)->fd->bk == unsorted_chunks(av)` or not. If not, an error ("free(): corrupted unsorted chunks") is thrown.
   8. If next chunk (by memory) was a top chunk, merge the chunks appropriately into a single top chunk.
5. If the chunk was mmapped, call `munmap_chunk`.

## __libc_free (void *mem)

1. Return if `mem` is NULL.
2. If the corresponding chunk is mmapped, call `munmap_chunk` if the dynamic brk/mmap threshold needs adjusting.
3. Get arena pointer for that corresponding chunk.
4. Call `_int_free`.
