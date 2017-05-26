# Heap Exploitation

The `glibc` library provides functions such as `free` and `malloc` to help developers manage the heap memory according to their use cases. It is the responsibility of the developer to:

* `free` any memory he/she has obtained using `malloc`.
* Do not `free` the same memory more than once.
* Ensure that memory usage does not go beyond the amount of memory requested, in other terms, prevent heap overflows.

Failing to do makes the software vulnerable to various kinds of attacks. [Shellphish](https://twitter.com/shellphish), a famous Capture the Flag team from UC Santa Barbara, has done a great job in listing a variety of heap exploitation techniques in [how2heap](https://github.com/shellphish/how2heap). Attacks described in "The Malloc Maleficarum" by "Phantasmal Phantasmagoria" in an [email](http://seclists.org/bugtraq/2005/Oct/118) to the "Bugtraq" mailing list are also described.

A summary of the attacks has been described below:

| Attack | Target | Technique |
| :---: | :---: | :---: |
| First Fit | This is not an attack, it just demonstrates the nature of glibc's allocator | --- |
| Double Free | Making `malloc` return an already allocated fastchunk | Disrupt the fastbin by freeing a chunk twice |
| Forging chunks | Making `malloc` return a nearly arbitrary pointer | Disrupting fastbin link structure |
| Unlink Exploit | Getting (nearly)arbitrary write access | Freeing a corrupted chunk and exploiting `unlink` |
| Shrinking Free Chunks | Making `malloc` return a chunk overlapping with an already allocated chunk | Corrupting a free chunk by decreasing its size |
| House of Spirit | Making `malloc` return a nearly arbitrary pointer | Forcing freeing of a crafted fake chunk |
| House of Lore | Making `malloc` return a nearly arbitrary pointer | Disrupting smallbin link structure |
| House of Force | Making `malloc` return a nearly arbitrary pointer | Overflowing into top chunk's header |
| House of Einherjar | Making `malloc` return a nearly arbitrary pointer | Overflowing a single byte into the next chunk |