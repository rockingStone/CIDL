#include "common.h"
#include "mem.h"
#include "tree.h"

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

void withdrawMemRecArr(struct memRec *ptr){
	assert(RECARRPOOLIDX<RECARRPOOLSIZE);
	RECARRPOOLPTR[RECARRPOOLIDX] = ptr;
	RECARRPOOLIDX++;
}
