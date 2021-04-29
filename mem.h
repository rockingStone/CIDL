#ifndef __AWN_MEM__
#define __AWN_MEM__

#include "common.h"

inline struct recTreeNode *allocateRecTreeNode() __attribute__((always_inline));
//xzjin TODO 现在对于recTreeNode没有回收方式，同样tailHead也是
void withdrawRecTreeNode(struct recTreeNode * ptr);
inline struct fileMapTreeNode *allocateFileMapTreeNode() __attribute__((always_inline));
void withdrawFileMapTreeNode(struct fileMapTreeNode * ptr);
inline struct tailhead *allocateTailHead() __attribute__((always_inline));
void withdrawTailHead(struct tailhead *ptr);
inline struct recBlockEntry *allocateRecBlockEntry() __attribute__((always_inline));
void withdrawRecBlockEntry(struct recBlockEntry *ptr);
inline struct memRec *allocateMemRecArr() __attribute__((always_inline));
void withdrawMemRecArr(struct memRec *ptr);

//xzjin 因为IDX开始存放的是缓存大小，所以先减后返回
inline struct recTreeNode *allocateRecTreeNode(){
	assert(RECTREENODEPOOLIDX>0);
	RECTREENODEPOOLIDX--;
	return RECTREENODEPOOLPTR[RECTREENODEPOOLIDX];
}

inline struct fileMapTreeNode *allocateFileMapTreeNode(){
	assert(FILEMAPTREENODEPOOLIDX>0);
	FILEMAPTREENODEPOOLIDX--;
	return FILEMAPTREENODEPOOLPTR[FILEMAPTREENODEPOOLIDX];
}

inline struct tailhead *allocateTailHead(){
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

inline struct recBlockEntry *allocateRecBlockEntry(){
	assert(RECBLOCKENTRYPOOLIDX>0);
	RECBLOCKENTRYPOOLIDX--;
	return RECBLOCKENTRYPOOLPTR[RECBLOCKENTRYPOOLIDX];
}

inline struct memRec *allocateMemRecArr(){
	assert(RECARRPOOLIDX>0);
	RECARRPOOLIDX--;
	return RECARRPOOLPTR[RECARRPOOLIDX];
}

#endif //__AWN_MEM__