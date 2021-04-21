#ifndef _AWN_H
#define _AWN_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
/* Include guard goes here */
#ifdef __cplusplus
extern "C" {
#endif

/* Copy N bytes of SRC to DEST.  */
extern void *ts_memcpy (void * __dest, void * __src,
		     size_t __n) ;

/* Free a block allocated by `malloc', `realloc' or `calloc'.  */
extern void ts_free (void *__ptr, void *__tail);

//extern int ts_memTraced(void* ptr);

/* Copy N bytes of SRC to DEST, SRC is traced.  */
extern void *ts_memcpy_traced(void * __dest, void * __src,
		     size_t __n) ;

/* Write N bytes of BUF to FD.  Return the number written, or -1.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
extern ssize_t  ts_write(int __fd, void *__buf, size_t __n);

/* Map addresses starting near ADDR and extending for LEN bytes.  from
   OFFSET into the file FD describes according to PROT and FLAGS.  If ADDR
   is nonzero, it is the desired mapping address.  If the MAP_FIXED bit is
   set in FLAGS, the mapping will be at ADDR exactly (which must be
   page-aligned); otherwise the system chooses a convenient nearby address.
   The return value is the actual mapping address chosen or MAP_FAILED
   for errors (in which case `errno' is set).  A successful `mmap' call
   deallocates any previous mapping for the affected region.  */
extern void *ts_mmap (void *__addr, size_t __len, int __prot,
		   int __flags, int __fd, __off_t __offset);

/* Deallocate any mapping for the region starting at ADDR and extending LEN
   bytes.  Returns 0 if successful, -1 for errors (and sets errno).  */
extern int ts_munmap (void *__addr, size_t __len);

/* Check that calls to open and openat with O_CREAT or O_TMPFILE set have an
   appropriate third/fourth parameter.  */
extern int ts_open (const char *__path, int __oflag, ...);
 
extern ssize_t ts_read (int __fd, void *__buf, size_t __nbytes);

/* Re-allocate the previously allocated block
   in PTR, making the new block SIZE bytes long.  */
extern void *ts_realloc (void *ptr, size_t size, void* tail);
//extern void *realloc (void *__ptr, size_t __size);

/* Open a file and create a new stream for it.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern FILE *ts_fopen (const char *filename,
		    const char *modes);

extern int ts_openat (char* dirName, int dirfd, const char *path, int oflag, ...);

/* Write chunks of generic data to STREAM.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
extern size_t ts_fwrite (const void *ptr, size_t size,
		      size_t n, FILE *s);

/* Close the file descriptor FD. */
extern int ts_close (int fd);
#ifdef __cplusplus
}
#endif

/* #endif of the include guard here */
#endif   //_AWN_H