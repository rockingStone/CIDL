#include "common.h"
#include "mem.h"
#include "tree.h"

void initMemory(){
	unsigned long long allocSize;
	totalAllocSize = 0;
	RECARRPOOLSIZE = 6000;
	TAILHEADPOOLSIZE = RECTREENODEPOOLSIZE;
	RECBLOCKENTRYPOOLSIZE = 6000;
	
	allocSize = sizeof(struct recTreeNode)*RECTREENODEPOOLSIZE;
	totalAllocSize += allocSize;
	RECTREENODEPOOL = malloc(allocSize);
	if(!RECTREENODEPOOL){
		ERROR("Could not allocate space for RECTREENODEPOOL.\n");
		assert(0);
	}
	allocSize = sizeof(struct recTreeNode*)*RECTREENODEPOOLSIZE;
	totalAllocSize += allocSize;
	RECTREENODEPOOLPTR = malloc(allocSize);
	if(!RECTREENODEPOOLPTR){
		ERROR("Could not allocate space for RECTREENODEPOOLPTR.\n");
		assert(0);
	}
	for(int i=0; i<RECTREENODEPOOLSIZE; i++){
		RECTREENODEPOOLPTR[i] = RECTREENODEPOOL+i;
	}
	RECTREENODEPOOLIDX = RECTREENODEPOOLSIZE;
	RECTREENODETHRESHOLD = RECTREENODEPOOLSIZE*RECTREENODETHRESHOLDRATIO;

	allocSize = sizeof(struct fileMapTreeNode)*FILEMAPTREENODEPOOLSIZE;
	totalAllocSize += allocSize;
	FILEMAPTREENODEPOOL = malloc(allocSize);
	if(!FILEMAPTREENODEPOOL){
		ERROR("Could not allocate space for FILEMAPTREENODEPOOL.\n");
		assert(0);
	}
	allocSize = sizeof(struct fileMapTreeNode*)*RECTREENODEPOOLSIZE;
	totalAllocSize += allocSize;
	FILEMAPTREENODEPOOLPTR = malloc(allocSize);
	if(!FILEMAPTREENODEPOOLPTR){
		ERROR("Could not allocate space for FILEMAPTREENODEPOOLPTR.\n");
		assert(0);
	}
	for(int i=0; i<FILEMAPTREENODEPOOLSIZE; i++){
		FILEMAPTREENODEPOOLPTR[i] = FILEMAPTREENODEPOOL+i;
	}
	FILEMAPTREENODEPOOLIDX = FILEMAPTREENODEPOOLSIZE;

//		SLISTHEADPOOL = malloc(sizeof(struct slisthead)*SLISTHEADPOOLSIZE);
//		if(!SLISTHEADPOOL){
//			ERROR("Could not allocate space for SLISTHEADPOOL.\n");
//			assert(0);
//		}
//		SLISTHEADPOOLPTR = malloc(sizeof(struct slisthead*)*SLISTHEADPOOLSIZE);
//		if(!SLISTHEADPOOLPTR){
//			ERROR("Could not allocate space for SLISTHEADPOOLPTR.\n");
//			assert(0);
//		}
//		for(int i=0; i<SLISTHEADPOOLSIZE; i++){
//			SLISTHEADPOOLPTR[i] = SLISTHEADPOOL+i;
//		}
//		SLISTHEADPOOLIDX = SLISTHEADPOOLSIZE;

	allocSize = sizeof(struct tailhead)*TAILHEADPOOLSIZE;
	totalAllocSize += allocSize;
	TAILHEADPOOL = malloc(allocSize);
	if(!TAILHEADPOOL){
		ERROR("Could not allocate space for TAILHEADPOOL.\n");
		assert(0);
	}
	allocSize = sizeof(struct tailhead*)*TAILHEADPOOLSIZE;
	totalAllocSize += allocSize;
	TAILHEADPOOLPTR = malloc(allocSize);
	if(!TAILHEADPOOLPTR){
		ERROR("Could not allocate space for TAILHEADPOOLPTR.\n");
		assert(0);
	}
	for(int i=0; i<TAILHEADPOOLSIZE; i++){
		TAILHEADPOOLPTR[i] = TAILHEADPOOL+i;
	}
	TAILHEADPOOLIDX = TAILHEADPOOLSIZE;

	allocSize = sizeof(struct recBlockEntry)*RECBLOCKENTRYPOOLSIZE;
	totalAllocSize += allocSize;
	RECBLOCKENTRYPOOL = malloc(allocSize);
	if(!RECBLOCKENTRYPOOL){
		ERROR("Could not allocate space for RECBLOCKENTRYPOOL.\n");
		assert(0);
	}
	allocSize = sizeof(struct recBlockEntry*)*RECBLOCKENTRYPOOLSIZE;
	totalAllocSize += allocSize;
	RECBLOCKENTRYPOOLPTR = malloc(allocSize);
	if(!RECBLOCKENTRYPOOLPTR){
		ERROR("Could not allocate space for RECBLOCKENTRYPOOLPTR.\n");
		assert(0);
	}
	for(int i=0; i<RECBLOCKENTRYPOOLSIZE; i++){
		RECBLOCKENTRYPOOLPTR[i] = RECBLOCKENTRYPOOL+i;
	}
	RECBLOCKENTRYPOOLIDX = RECBLOCKENTRYPOOLSIZE;

#ifndef BASE_VERSION
	//xzjin 注意这里RECARRPOOL的类型是memRec的指针，不是memRec[MEMRECPERENTRY],申请的时候应该是+-MEMRECPERENTRY
	allocSize = sizeof(struct memRec)*MEMRECPERENTRY*RECARRPOOLSIZE;
	totalAllocSize += allocSize;
	RECARRPOOL = malloc(allocSize);
	if(!RECARRPOOL){
		ERROR("Could not allocate space for RECARRPOOL.\n");
		assert(0);
	}
	allocSize = sizeof(struct memRec*)*RECARRPOOLSIZE;
	totalAllocSize += allocSize;
	RECARRPOOLPTR = malloc(allocSize);
	if(!RECARRPOOLPTR){
		ERROR("Could not allocate space for RECARRPOOLPTR.\n");
		assert(0);
	}
	for(int i=0; i<RECARRPOOLSIZE; i++){
		RECARRPOOLPTR[i] = RECARRPOOL+MEMRECPERENTRY*i;
		//RECARRPOOLTAIL = (unsigned long long)RECARRPOOLPTR[i];
	}
	RECARRPOOLIDX = RECARRPOOLSIZE;
#else
	allocSize = sizeof(struct memRec)*RECARRPOOLSIZE;
	totalAllocSize += allocSize;
	RECARRPOOL = malloc(allocSize);
	if(!RECARRPOOL){
		ERROR("Could not allocate space for RECARRPOOL.\n");
		assert(0);
	}
	allocSize = sizeof(struct memRec*)*RECARRPOOLSIZE;
	totalAllocSize += allocSize;
	RECARRPOOLPTR = malloc(allocSize);
	if(!RECARRPOOLPTR){
		ERROR("Could not allocate space for RECARRPOOLPTR.\n");
		assert(0);
	}
	for(int i=0; i<RECARRPOOLSIZE; i++){
		RECARRPOOLPTR[i] = RECARRPOOL+i;
		//RECARRPOOLTAIL = (unsigned long long)RECARRPOOLPTR[i];
	}
	RECARRPOOLIDX = RECARRPOOLSIZE;
#endif	//BASE_VERSION

	MSG("Allocate %llu bytes memory.\n", totalAllocSize);
}
//xzjin TODO 现在对于recTreeNode没有回收方式，同样tailHead也是
void withdrawRecTreeNode(struct recTreeNode * ptr){
	assert(RECTREENODEPOOLIDX<RECTREENODEPOOLSIZE);
	RECTREENODEPOOLPTR[RECTREENODEPOOLIDX] = ptr;
	RECTREENODEPOOLIDX++;
}

void withdrawFileMapTreeNode(struct fileMapTreeNode * ptr){
	assert(FILEMAPTREENODEPOOLIDX<FILEMAPTREENODEPOOLSIZE);
	FILEMAPTREENODEPOOLPTR[FILEMAPTREENODEPOOLIDX] = ptr;
	FILEMAPTREENODEPOOLIDX++;
}

void withdrawTailHead(struct tailhead *ptr){
	assert(TAILHEADPOOLIDX<TAILHEADPOOLSIZE);
	TAILHEADPOOLPTR[TAILHEADPOOLIDX] = ptr;
	TAILHEADPOOLIDX++;
}

//void withdrawSlistHead(struct slisthead *ptr){
//	assert(SLISTHEADPOOLIDX<SLISTHEADPOOLSIZE);
//	SLISTHEADPOOLPTR[SLISTHEADPOOLIDX] = ptr;
//	SLISTHEADPOOLIDX++;
//}

void withdrawRecBlockEntry(struct recBlockEntry *ptr){
	assert(RECBLOCKENTRYPOOLIDX<RECBLOCKENTRYPOOLSIZE);
	RECBLOCKENTRYPOOLPTR[RECBLOCKENTRYPOOLIDX] = ptr;
	RECBLOCKENTRYPOOLIDX++;
}

#ifndef BASE_VERSION
void withdrawMemRecArr(struct memRec *ptr){
	assert(RECARRPOOLIDX<RECARRPOOLSIZE);
	RECARRPOOLPTR[RECARRPOOLIDX] = ptr;
	RECARRPOOLIDX++;
}
#else
void withdrawMemRecArr(struct memRec *ptr){
	free(ptr);
}
#endif	//BASE_VERSION