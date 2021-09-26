# General Information

AWN is an I/OCausality-based In-line Deduplication Library designed for NVM devices

# Environment
Linux 64bit.

# INSTALL
make
sudo make install

# Running
1. Inclued header file (awn.h) in your source code
2. Replace the functions that the reduncy flows by with AWN's functions

# Function list

1. void ts_print_statistics();
2. void* ts_memcpy (void * __dest, void * __src, size_t __n) ;
3. void ts_free (void *__ptr, void *__tail);
4. void* ts_memcpy_traced(void * __dest, void * __src, size_t __n) ;
5. ssize_t ts_write(int __fd, void *__buf, size_t __n);
6. ssize_t ts_pwrite(int __fd, void *__buf, size_t __n, __off64_t __offset);
7. void* ts_mmap (void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset);
8. int ts_munmap (void *__addr, size_t __len);
9. int ts_open (const char *__path, int __oflag, ...);
10. ssize_t ts_read (int __fd, void *__buf, size_t __nbytes);
11. void* ts_realloc (void *ptr, size_t size, void* tail);
12. FILE* ts_fopen (const char *filename, const char *modes);
13. int ts_openat (char* dirName, int dirfd, const char *path, int oflag, ...);
14. size_t ts_fwrite (const void *ptr, size_t size, size_t n, FILE *s);
15. int ts_close (int fd);