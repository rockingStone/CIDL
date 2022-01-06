#ifndef __AWN_COMMON__
#define __AWN_COMMON__

#ifndef __cplusplus
//typedef long long off64_t;
#endif

//#define BASE_VERSION
//#undef BASE_VERSION
#define _GNU_SOURCE

#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <search.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <linux/types.h>

#include "debug.h"
#include "statistics.h"

#define BUF_SIZE 40
#undef USE_STAIL

#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#define MAX(X,Y) (((X)>(Y))?(X):(Y))

// tell the compiler a branch is/is not likely to be followed
#define LIKELY(x)       __builtin_expect((long int)(x),1)
#define UNLIKELY(x)     __builtin_expect((long int)(x),0)

#ifndef __cplusplus
typedef int bool;
#define false 0
#define true 1
#endif

#ifndef MAX_FILE_NUM
#define  MAX_FILE_NUM 1024
#endif

struct fileMapTreeNode* openFileArray[MAX_FILE_NUM];
#define REALPKEYPROTECT 0

atomic_uint_least64_t Instrustats[INSTRUMENT_NUM];

typedef off_t off64_t;
typedef struct RBNode *RBNodePtr;
struct protRec{
	void* pageNum;
	int pro;
	struct protRec* next;
};

struct searchCache{
	void* pageNum;
	bool PageProtedted;
	RBNodePtr lastMapNode;
	RBNodePtr pageMapTree;
	int proKey;
};


#define MEMRECPERENTRY 38		// (4096/((13+100)/2))/2
void* recTreeRoot;
void* fileMapTreeRoot;
void* fileMapNameTreeRoot;

#define RECTREENODETHRESHOLDRATIO (0.6)

/**xzjin fd to path map, <fd>'s path is in 
 * fd%FILEMAPTREENODEPOOLSIZE; which means when
 * fd > FILEMAPTREENODEPOOLSIZE, then may by an
 * error.
*/
char** FD2PATH;
struct fileMapTreeNode* FILEMAPTREENODEPOOL;
#define FILEMAPTREENODEPOOLSIZE 2048
int FILEMAPTREENODEPOOLIDX;
struct fileMapTreeNode** FILEMAPTREENODEPOOLPTR;

//struct slisthead* SLISTHEADPOOL;
//#define SLISTHEADPOOLSIZE 800
//int SLISTHEADPOOLIDX;
//struct slisthead** SLISTHEADPOOLPTR;

extern unsigned long long totalAllocSize;
extern pthread_mutex_t recTreeMutex;

struct fileMapTreeNode *lastTs_memcpyFmNode;

struct {
	void* lastPageNum;
	struct memRec* lastMemRec;
	short* lastIdx;	//xzjin 这里指向recTreeNode的memRecIdx（指向可以直接写的entry）
} lastRec;		//这是上次写的缓存，因为写一般是有连续性的(一般是追加)

#define MMAPCACHESIZE 20
struct searchCache* mmapSrcCache;
struct searchCache* mmapDestCache;
//struct protRec* protRecPool;
//#define protRECPOOLSIZE 204

//RBNodePtr RBPool;
//#define RBPOOLSIZE 40960
//RBNodePtr *RBPoolPointer;
//int RBPPIdx;
//RBNodePtr allocateRBNode();
//void freeRBNode(RBNodePtr ptr);
//RBNodePtr lastDestInMap;
//RBNodePtr lastSrcInMap;
//int lastSrcInMapKey,lastDestInMapKey;

#define PROMAPPERSIZE 256
struct protRec** proMapper;

#define FILEMAPCACHESIZE 4
struct fileMapTreeNode* fileMapCache[FILEMAPCACHESIZE];

struct{
	int leastUsedTime;
	int idx;
} leastRecFCache;

//xzjin TODO 添加对多个锁的支持
//xzjin this Key is always allow all operation
int execv_done;
int PAGESIZE,PAGENUMSHIFT;
long long PAGEOFFMASK;
long long PAGENUMMASK;
RBNodePtr findmap2f(void* addr, RBNodePtr* pageMapTree, int* protected, int* proKey);
void segvhandler(int signum, siginfo_t* info, void* context);
struct sigaction action;
struct sigaction defaction;

#define STOREPATH(fd, path) do{ FD2PATH[fd%FILEMAPTREENODEPOOLSIZE] = path;} while(0)
#define GETPATH(fd) FD2PATH[fd%FILEMAPTREENODEPOOLSIZE]

// breaking the build
#define CALL_MMAP   addr, len, prot, flags, file, off
#define CALL_MEMCPY dest, src, n
#define CALL_MUNMAP addr, len
#define CALL_WRITE  file, buf, length
#define CALL_OPEN   path, oflag
#define CALL_READ   file, buf, length

#define RETT_MMAP   void*
#define RETT_MEMCPY void*
#define RETT_MUNMAP int
#define RETT_WRITE  ssize_t
#define RETT_OPEN   int
#define RETT_READ   ssize_t

#define INTF_MEMCPY void *dest, const void *src, size_t n
#define INTF_MMAP   void *addr, size_t len, int prot, int flags, int file, off_t off
#define INTF_MUNMAP void *addr, size_t len
#define INTF_WRITE  int file, const void* buf, size_t length
#define INTF_OPEN const char *path, int oflag, ...
#define INTF_READ   int file, void* buf, size_t length

#define PFFS_WRITE  "%i, %p, %i"
#define PFFS_OPEN   "%s, %i"
#define PFFS_READ   "%i, %p, %i"

#endif //__AWN_COMMON__