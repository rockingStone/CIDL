# General Information

CIDL is an I/O Causality-based In-line Deduplication Library designed for NVM devices. It memory map the files into DRAM and find the duplicate by tracking I/O operations such as read/copy/write. CIDL maintains several data structures to manage the record of I/O operations and use a location-dependent method to identify and delete outdated data. CIDL achieves up to 10× higher deduplication ratio than chunk-based methods and reduce the execution time by 47% on average. 

# How to Use？
## Environment
Linux 64bit.

## Build and Install
git clone https://github.com/rockingStone/CIDL.git  
cd CIDL; make  
sudo make install

## Running
1. Inclued header file (awn.h) in your source code
2. Replace the functions with AWN's functions where the redundancy flows

# List of Main Functions
1. int CIDL_open (const char *__path, int __oflag, ...);
2. FILE* CIDL_fopen (const char *filename, const char *modes);
3. int CIDL_openat (char* dirName, int dirfd, const char *path, int oflag, ...);
4. void* CIDL_memcpy (void * __dest, void * __src, size_t __n) ;
5. void CIDL_free (void *__ptr, void *__tail);
6. void* CIDL_memcpy_traced(void * __dest, void * __src, size_t __n) ;
7. ssize_t CIDL_write(int __fd, void *__buf, size_t __n);
8. ssize_t CIDL_pwrite(int __fd, void *__buf, size_t __n, __off64_t __offset);
9. size_t CIDL_fwrite (const void *ptr, size_t size, size_t n, FILE *s);
10. void* CIDL_mmap (void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset);
11. int CIDL_munmap (void *__addr, size_t __len);
12. ssize_t CIDL_read (int __fd, void *__buf, size_t __nbytes);
13. void* CIDL_realloc (void *ptr, size_t size, void* tail);
14. int CIDL_close (int fd);
