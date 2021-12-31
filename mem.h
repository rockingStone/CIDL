#ifndef __AWN_MEM__
#define __AWN_MEM__

#include "common.h"
#include "tree.h"

struct fMap{
	void* start;	//文件映射的开始
	void* tail;		//文件映射的结束
	int usedTime;
	int fd;
	char* fileName;
	unsigned long offset;	//文件映射部分相对于文件开头的偏移
};

#ifndef BASE_VERSION
//xzjin 对于跨页的记录，就记录在开始地址对应的记录里，反正最后会遍历对比
struct memRec{
	unsigned long fileOffset;
	char* fileName;
	short pageOffset;		//记录在页内的偏移，memRec是对应一个页的一条记录，页号是记录在树节点的
	short fd;
};
#else
struct memRec{
	unsigned long fileOffset;
	char* fileName;
	unsigned long long startMemory;
	unsigned long long tailMemory; 	//The tailMemory is not copied.
};

#define assertRec(rec) 	do{ assert(rec->startMemory<rec->tailMemory); \
	assert(rec->startMemory>343596402010 && rec->tailMemory>343596402010); \
	assert(rec->fileOffset<100000000000002); \
	assert(rec->fileName); }while(0)

#endif //BASE_VERSION

//xzjin single list head
struct slisthead{
    struct  recBlockEntry *slh_first;	/* first element */	
};

//xzjin tail queue list
struct tailhead {
	struct recBlockEntry *tqh_first;		/* first element */
	struct recBlockEntry * *tqh_last;	/* addr of last next element */
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

void initMemory();
inline struct recTreeNode *allocateRecTreeNode() __attribute__((always_inline));
//xzjin TODO 现在对于recTreeNode没有回收方式，同样tailHead也是
void withdrawRecTreeNode(struct recTreeNode * ptr);
inline struct fileMapTreeNode *allocateFileMapTreeNode() __attribute__((always_inline));
void withdrawFileMapTreeNode(struct fileMapTreeNode * ptr);
inline struct tailhead *allocateTailHead() __attribute__((always_inline));
void withdrawTailHead(struct tailhead* ptr);
inline struct recBlockEntry *allocateRecBlockEntry() __attribute__((always_inline));
void withdrawRecBlockEntry(struct recBlockEntry *ptr);
inline struct memRec *allocateMemRecArr() __attribute__((always_inline));
void withdrawMemRecArr(struct memRec *ptr);

struct recTreeNode* RECTREENODEPOOL;
int RECTREENODEPOOLSIZE;
int RECTREENODEPOOLIDX;
int RECTREENODETHRESHOLD;
struct recTreeNode** RECTREENODEPOOLPTR;
//xzjin 因为IDX开始存放的是缓存大小，所以先减后返回
inline struct recTreeNode *allocateRecTreeNode(){
//	if(UNLIKELY(RECTREENODEPOOLIDX<=0)){
//		unsigned long long allocSize;
//        struct recTreeNode** recTreeNodeDepot;
//
//		allocSize = sizeof(struct tailhead)*RECTREENODEPOOLSIZE;
//		totalAllocSize += allocSize;
//		RECTREENODEPOOL = malloc(allocSize);
//		if(!RECTREENODEPOOL){
//			ERROR("Could not allocate space for RECTREENODEPOOL.\n");
//			assert(0);
//		}
//
//        recTreeNodeDepot = malloc(sizeof(struct tailhead*)*RECTREENODEPOOLSIZE*2);
//		totalAllocSize += sizeof(struct tailhead*)*RECTREENODEPOOLSIZE;
//		if(!totalAllocSize){
//			ERROR("Could not allocate space for RECTREENODEPOOLPTR.\n");
//			assert(0);
//		}
//        memcpy(recTreeNodeDepot, RECTREENODEPOOLPTR, RECTREENODEPOOLIDX*sizeof(struct tailhead*));
//        free(RECTREENODEPOOLPTR);
//        RECTREENODEPOOLPTR = recTreeNodeDepot;
//		RECTREENODEPOOLPTR[RECTREENODEPOOLIDX] = RECTREENODEPOOL;
//		for(int i=RECTREENODEPOOLIDX+1, end = RECTREENODEPOOLIDX+RECTREENODEPOOLSIZE; i< end; i++){
//			RECTREENODEPOOLPTR[i] = RECTREENODEPOOLPTR[i-1] + 1;
//		}
//		RECTREENODEPOOLIDX += RECTREENODEPOOLSIZE;
//        RECTREENODEPOOLSIZE += RECTREENODEPOOLSIZE;
//
//	}

	assert(RECTREENODEPOOLIDX>0);
	RECTREENODEPOOLIDX--;
	return RECTREENODEPOOLPTR[RECTREENODEPOOLIDX];
}

inline struct fileMapTreeNode *allocateFileMapTreeNode(){
	assert(FILEMAPTREENODEPOOLIDX>0);
	FILEMAPTREENODEPOOLIDX--;
	return FILEMAPTREENODEPOOLPTR[FILEMAPTREENODEPOOLIDX];
}

struct tailhead* TAILHEADPOOL;
unsigned long TAILHEADPOOLSIZE;
int TAILHEADPOOLIDX;
struct tailhead** TAILHEADPOOLPTR;

//xzjin TAILHEAD和RECTREENODE的数目一定是相同的
inline struct tailhead *allocateTailHead(){
	if(UNLIKELY(TAILHEADPOOLIDX<=0)){
		unsigned long long allocSize;
        struct tailhead** tailheadDepot;

		allocSize = sizeof(struct tailhead)*TAILHEADPOOLSIZE;
		totalAllocSize += allocSize;
		TAILHEADPOOL = malloc(allocSize);
		if(!TAILHEADPOOL){
			ERROR("Could not allocate space for TAILHEADPOOL.\n");
			assert(0);
		}

        tailheadDepot = malloc(sizeof(struct tailhead*)*TAILHEADPOOLSIZE*2);
		totalAllocSize += sizeof(struct tailhead*)*TAILHEADPOOLSIZE;
		if(!totalAllocSize){
			ERROR("Could not allocate space for TAILHEADPOOLPTR.\n");
			assert(0);
		}
        memcpy(tailheadDepot, TAILHEADPOOLPTR, TAILHEADPOOLIDX*sizeof(struct tailhead*));
        free(TAILHEADPOOLPTR);
        TAILHEADPOOLPTR = tailheadDepot;
		TAILHEADPOOLPTR[TAILHEADPOOLIDX] = TAILHEADPOOL;
		for(int i=TAILHEADPOOLIDX+1, end = TAILHEADPOOLIDX+TAILHEADPOOLSIZE; i< end; i++){
			TAILHEADPOOLPTR[i] =TAILHEADPOOLPTR[i-1] + 1;
		}
		TAILHEADPOOLIDX += TAILHEADPOOLSIZE;
        TAILHEADPOOLSIZE += TAILHEADPOOLSIZE;

	}
	assert(TAILHEADPOOLIDX>0);
	TAILHEADPOOLIDX--;
	return TAILHEADPOOLPTR[TAILHEADPOOLIDX];
}

//inline struct slisthead *allocateSlistHead() __attribute__((always_inline));
//inline struct slisthead *allocateSlistHead(){
//	assert(SLISTHEADPOOLIDX>0);
//	SLISTHEADPOOLIDX--;
//	return SLISTHEADPOOLPTR[SLISTHEADPOOLIDX];
//}

struct recBlockEntry* RECBLOCKENTRYPOOL;
unsigned long RECBLOCKENTRYPOOLSIZE;
int RECBLOCKENTRYPOOLIDX;
struct recBlockEntry** RECBLOCKENTRYPOOLPTR;

inline struct recBlockEntry *allocateRecBlockEntry(){
	if(UNLIKELY(RECBLOCKENTRYPOOLIDX<=0)){
        struct recBlockEntry** poolDepot;
		unsigned long long allocSize;

        allocSize = sizeof(struct recBlockEntry)*RECBLOCKENTRYPOOLSIZE;
		totalAllocSize += allocSize;

		RECBLOCKENTRYPOOL = malloc(allocSize);
		if(!RECBLOCKENTRYPOOL){
			ERROR("Could not allocate space for RECBLOCKENTRYPOOL.\n");
			assert(0);
		}
        //poolDepot = realloc(RECBLOCKENTRYPOOLPTR, sizeof(struct recBlockEntry*)*RECBLOCKENTRYPOOLSIZE*2);
        poolDepot = malloc(sizeof(struct recBlockEntry*)*RECBLOCKENTRYPOOLSIZE*2);
		totalAllocSize += sizeof(struct recBlockEntry*)*RECBLOCKENTRYPOOLSIZE;
		if(!poolDepot){
			ERROR("Could not allocate space for RECBLOCKENTRYPOOLPTR.\n");
			assert(0);
		}
        memcpy(poolDepot, RECBLOCKENTRYPOOLPTR, RECBLOCKENTRYPOOLIDX * sizeof(struct recBlockEntry*));
        free(RECBLOCKENTRYPOOLPTR);
        RECBLOCKENTRYPOOLPTR = poolDepot;
		RECBLOCKENTRYPOOLPTR[RECBLOCKENTRYPOOLIDX] = RECBLOCKENTRYPOOL;
		for(int i=RECBLOCKENTRYPOOLIDX+1, end = RECBLOCKENTRYPOOLIDX+RECBLOCKENTRYPOOLSIZE; i< end; i++){
			RECBLOCKENTRYPOOLPTR[i] = RECBLOCKENTRYPOOLPTR[i-1] + 1;
		}
		RECBLOCKENTRYPOOLIDX += RECBLOCKENTRYPOOLSIZE;
        RECBLOCKENTRYPOOLSIZE += RECBLOCKENTRYPOOLSIZE;
	}
	assert(RECBLOCKENTRYPOOLIDX>0);
	RECBLOCKENTRYPOOLIDX--;
	return RECBLOCKENTRYPOOLPTR[RECBLOCKENTRYPOOLIDX];
}

struct memRec* RECARRPOOL;
unsigned long RECARRPOOLSIZE;
int RECARRPOOLIDX;
struct memRec** RECARRPOOLPTR;

#ifndef BASE_VERSION
inline struct memRec *allocateMemRecArr(){
	if(UNLIKELY(RECARRPOOLIDX<=0)){
        struct memRec** poolDepot;
		unsigned long long allocSize;

        allocSize = sizeof(struct memRec)*MEMRECPERENTRY*RECARRPOOLSIZE;
		totalAllocSize += allocSize;

		RECARRPOOL = malloc(allocSize);
		if(!RECARRPOOL){
			ERROR("Could not allocate space for RECARRPOOL.\n");
			assert(0);
		}
        MSG("%llu, %llu, %llu\n\n", (long long unsigned int)RECARRPOOLPTR,
                 (long long unsigned int)sizeof(struct memRec*)*RECARRPOOLSIZE*2,
                 (long long unsigned int)totalAllocSize);
        //poolDepot = realloc(RECARRPOOLPTR, sizeof(struct memRec*)*RECARRPOOLSIZE*2);
        poolDepot = malloc(sizeof(struct memRec*)*RECARRPOOLSIZE*2);
		totalAllocSize += sizeof(struct memRec*)*RECARRPOOLSIZE;
		if(!poolDepot){
			ERROR("Could not reallocate space for RECARRPOOLPTR.\n");
			assert(0);
		}
        memcpy(poolDepot, RECARRPOOLPTR, RECARRPOOLIDX * sizeof(struct memRec*));
        free(RECARRPOOLPTR);
        RECARRPOOLPTR = poolDepot;
		RECARRPOOLPTR[RECARRPOOLIDX] = RECARRPOOL;
		for(int i=RECARRPOOLIDX+1, end = RECARRPOOLIDX+RECARRPOOLSIZE; i< end; i++){
			RECARRPOOLPTR[i] = RECARRPOOLPTR[i-1] + MEMRECPERENTRY;
		}
		RECARRPOOLIDX += RECARRPOOLSIZE;
        RECARRPOOLSIZE += RECARRPOOLSIZE;
	}
	assert(RECARRPOOLIDX>0);
	RECARRPOOLIDX--;
	return RECARRPOOLPTR[RECARRPOOLIDX];
}
#else 
inline struct memRec *allocateMemRecArr(){
	//MSG("malloc rec\n");
	struct memRec* ret = malloc(sizeof(struct memRec));
	if(!ret){
		ERROR("alloc struct memRec ERROR.\n");
		exit(-1);
	}

	totalAllocSize += sizeof(struct memRec);
	return ret;
}
#endif	//BASE_VERSION

#endif //__AWN_MEM__