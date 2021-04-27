
// Header file shared by nvmfileops.c, fileops_compareharness.c

#ifndef __NV_COMMON_H_
#define __NV_COMMON_H_

#ifndef __cplusplus
//typedef long long off64_t;
#endif

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

//#include "rbtree.h"
#include "debug.h"
#include "timers.h"

#include "boost/preprocessor/seq/for_each.hpp"
//#include "boost/preprocessor/cat.hpp"

#define BUF_SIZE 40
#undef USE_STAIL

#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#define MAX(X,Y) (((X)>(Y))?(X):(Y))

// tell the compiler a branch is/is not likely to be followed
#define LIKELY(x)       __builtin_expect((long int)(x),1)
#define UNLIKELY(x)     __builtin_expect((long int)(x),0)

#define assert(x) if(UNLIKELY(!(x))) { printf("ASSERT FAILED\n"); fflush(NULL); ERROR("ASSERT("#x") failed!\n"); exit(100); }

// places quotation marks around arg (eg, MK_STR(stuff) becomes "stuff")
#define MK_STR(arg) #arg

#define MACRO_WRAP(a) a
#define MACRO_CAT(a, b) MACRO_WRAP(a##b)

#ifndef __cplusplus
typedef int bool;
#define false 0
#define true 1
#endif
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

struct fMap{
	void* start;	//文件映射的开始
	void* tail;		//文件映射的结束
	int usedTime;
	int fd;
	char* fileName;
	unsigned long offset;	//文件映射部分相对于文件开头的偏移
};

//xzjin 对于跨页的记录，就记录在开始地址对应的记录里，反正最后会遍历对比
struct memRec{
	unsigned long fileOffset;
	char* fileName;
	short pageOffset;		//记录在页内的偏移，memRec是对应一个页的一条记录，页号是记录在树节点的
//	short fd;
};

//xzjin single list element
#ifndef  USE_STAIL
	struct recBlockEntry{
		struct memRec* recArr;		//这里是一块连续struct memRec地址的开始
		TAILQ_ENTRY(recBlockEntry) entries;     /* Singly-linked List. */
	};
#else
	struct recBlockEntry{
		struct memRec* recArr;		//这里是一块连续struct memRec地址的开始
		SLIST_ENTRY(recBlockEntry) entries;     /* Singly-linked List. */
	};
#endif	// USE_STAIL

//xzjin single list head
struct slisthead{
    struct  recBlockEntry *slh_first;	/* first element */	
};

//xzjin tail queue list
struct tailhead {
	struct recBlockEntry *tqh_first;		/* first element */
	struct recBlockEntry * *tqh_last;	/* addr of last next element */
};

#define MEMRECPERENTRY 38		// (4096/((13+100)/2))/2
void* recTreeRoot;
void* fileMapTreeRoot;
void* fileMapNameTreeRoot;

#ifndef  USE_STAIL
struct recTreeNode{
	void* pageNum;
    struct tailhead* listHead;
	struct recBlockEntry *recModEntry;	//used when insert to tail, most recent modified entry.
	short memRecIdx;		//It seems write first and then increase idx
};
#else
struct recTreeNode{
	void* pageNum;
    struct slisthead* listHead;
	struct recBlockEntry *recModEntry;	//used when insert to tail, most recent modified entry.
	short memRecIdx;		//It seems write first and then increase idx
};
#endif	//USE_STAIL

struct fileMapTreeNode{
	void* start;	//文件映射的开始
	void* tail;		//文件映射的结束
	int usedTime;
	char* fileName;
	unsigned long offset;	//文件映射部分相对于文件开头的偏移
//	unsigned int ref;	//对这个文件的引用数
//	bool userUnmapped;	//用户是否已经对这个文件unmmap了
};

struct recTreeNode* RECTREENODEPOOL;
#define RECTREENODEPOOLSIZE 2500
int RECTREENODEPOOLIDX;
struct recTreeNode** RECTREENODEPOOLPTR;

struct fileMapTreeNode* FILEMAPTREENODEPOOL;
//#define FILEMAPTREENODEPOOLSIZE 1024
#define FILEMAPTREENODEPOOLSIZE 2048
int FILEMAPTREENODEPOOLIDX;
struct fileMapTreeNode** FILEMAPTREENODEPOOLPTR;

//struct slisthead* SLISTHEADPOOL;
//#define SLISTHEADPOOLSIZE 800
//int SLISTHEADPOOLIDX;
//struct slisthead** SLISTHEADPOOLPTR;

//xzjin TAILHEAD和RECTREENODE的数目一定是相同的
struct tailhead* TAILHEADPOOL;
#define TAILHEADPOOLSIZE RECTREENODEPOOLSIZE
int TAILHEADPOOLIDX;
struct tailhead** TAILHEADPOOLPTR;

struct recBlockEntry* RECBLOCKENTRYPOOL;
#define RECBLOCKENTRYPOOLSIZE 60000
int RECBLOCKENTRYPOOLIDX;
struct recBlockEntry** RECBLOCKENTRYPOOLPTR;

struct memRec* RECARRPOOL;
unsigned long long RECARRPOOLTAIL;
#define RECARRPOOLSIZE 60000
int RECARRPOOLIDX;
struct memRec** RECARRPOOLPTR;

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
#define FMAPCACHESIZE 10
struct fMap fMapCache[FMAPCACHESIZE];

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


/**xzjin fd to path map, <fd>'s path is in fd%100;
 * which means when fd > 100, then may by an error.
*/
char** FD2PATH;
//xzjin TODO 添加对多个锁的支持
//xzjin this Key is always allow all operation
int nonProKey;
int execv_done,proKey;
int PAGESIZE,PAGENUMSHIFT;
long long PAGEOFFMASK;
long long PAGENUMMASK;
RBNodePtr pageNum2Tree;
// TNULL is process shared
extern RBNodePtr TNULL;
extern void nvp_print_io_stats(void);
void p2fAppend(void *, long long , long long , char*, short);
RBNodePtr findmap2f(void* addr, RBNodePtr* pageMapTree, int* protected, int* proKey);
extern void *intel_memcpy(void* __restrict__ b, const void* __restrict__ a, size_t n);
void delMap(RBNodePtr* root, RBNodePtr addr);
void segvhandler(int signum, siginfo_t* info, void* context);
extern void* addr2PageNum(void* addr);
extern void* addr2PageTail(void* addr);
extern void* addr2PageBegin(void* addr);
extern short addr2PageOffset(void* addr);
struct sigaction action;
struct sigaction defaction;
// maximum number of file operations to support simultaneously
#define MAX_FILEOPS 32

#define RESOLVE_TWO_FILEOPS(MODULENAME, OP1, OP2) \
	DEBUG("Resolving module "MODULENAME": wants two fileops.\n"); \
	struct Fileops_p** result = resolve_n_fileops(tree, MODULENAME, 2); \
	OP1 = result[0]; \
	OP2 = result[1]; \
	if(OP1 == NULL) { \
		ERROR("Failed to resolve "#OP1"\n"); \
		assert(0); \
	} else { \
		DEBUG(MODULENAME"("#OP1") resolved to %s\n", OP1->name); \
	} \
	if(OP2 == NULL) { \
		ERROR("Failed to resolve "#OP2"\n"); \
		assert(0); \
	} else { \
		DEBUG(MODULENAME"("#OP2") resolved to %s\n", OP2->name); \
	}

#define STOREPATH(fd, path) do{ FD2PATH[fd%FILEMAPTREENODEPOOLSIZE] = path;} while(0)
#define GETPATH(fd) FD2PATH[fd%FILEMAPTREENODEPOOLSIZE]
#endif

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