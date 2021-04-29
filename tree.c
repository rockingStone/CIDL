#include "addr.h"
#include "common.h"
#include "mem.h"
#include "tree.h"

void listFileMapNodeAction(const void *nodep, const VISIT which, const int depth){
    struct fileMapTreeNode *fmNode;

    switch (which) {
    case preorder:
        break;
    case postorder:
        fmNode = *(struct fileMapTreeNode **) nodep;
		MSG("map start:%p, map tail:%p, map offset:%ld, fileName: %s, fileName addr:%p, fmNodeaddr: %p\n",
			fmNode->start, fmNode->tail, fmNode->offset, fmNode->fileName, fmNode->fileName, fmNode);
        break;
    case endorder:
        break;
    case leaf:
        fmNode = *(struct fileMapTreeNode **) nodep;
		MSG("map start:%p, map tail:%p, map offset:%ld, fileName: %s, fileName addr:%p, fmNodeaddr: %p\n",
			fmNode->start, fmNode->tail, fmNode->offset, fmNode->fileName, fmNode->fileName, fmNode);
        break;
    }
}

void listFileMapTree() {
	twalk(fileMapTreeRoot,listFileMapNodeAction);
}

void showRecMapNode(struct recTreeNode **nodep){
    struct recTreeNode *fmNode;
    struct tailhead* head;
	fmNode = *(struct recTreeNode **) nodep;
	head = fmNode->listHead;

	MSG("pageNum: %p, list head:%p, memRecIdx:%2d, struct address: %p.\n",
		fmNode->pageNum, head, fmNode->memRecIdx, fmNode);
}

void listRecMapAction(const void *nodep, const VISIT which, const int depth){

    switch (which) {
    case preorder:
        break;
    case postorder:
		showRecMapNode((struct recTreeNode **)nodep);
        break;
    case endorder:
        break;
    case leaf:
		showRecMapNode((struct recTreeNode **)nodep);
        break;
    }
}

void showRecMapNodeDetail(struct recTreeNode **nodep){
    struct recTreeNode *fmNode;
    struct tailhead* head;
	fmNode = *(struct recTreeNode **) nodep;
	head = fmNode->listHead;

	fmNode = *(struct recTreeNode **) nodep;
	MSG("pageNum: %p, list head:%p, memRecIdx:%2d, struct address: %p.\n",
		fmNode->pageNum, head, fmNode->memRecIdx, fmNode);
	listRecTreeNode(fmNode->pageNum);
}

void listRecMapActionDetail(const void *nodep, const VISIT which, const int depth){

    switch (which) {
    case preorder:
        break;
    case postorder:
		showRecMapNodeDetail((struct recTreeNode **)nodep);
        break;
    case endorder:
        break;
    case leaf:
		showRecMapNodeDetail((struct recTreeNode **)nodep);
        break;
    }
}

void listRecTree(){
	twalk(recTreeRoot, listRecMapAction);
}

void listRecTreeDetail(){
	twalk(recTreeRoot, listRecMapActionDetail);
}

int recCompare(const void *pa, const void *pb) {
    const struct recTreeNode* a = pa;
    const struct recTreeNode* b = pb;
    if ((unsigned long)a->pageNum < (unsigned long)b->pageNum)
        return -1;
    if ((unsigned long)a->pageNum > (unsigned long)b->pageNum)
        return 1;
    return 0;
}

int fileMapTreeInsDelCompare(const void *pa, const void *pb) {
    const struct fileMapTreeNode* a = pa;
    const struct fileMapTreeNode* b = pb;
    if (a->start < b->start)
        return -1;
    if (a->start > b->start)
        return 1;
    return 0;
}

int fileMapNameTreeInsDelCompare(const void *pa, const void *pb) {
    const struct fileMapTreeNode* a = pa;
    const struct fileMapTreeNode* b = pb;
    if (a->fileName < b->fileName)
        return -1;
    if (a->fileName > b->fileName)
        return 1;
    return 0;
}

/** xzjin compare relationship			
 * a.			|_________|			  a ? b
 * b.1 	|___|							>
 * b.2 	|____________|					=
 * b.3 	|__________________________|	=
 * b.4 				|___|				=
 * b.5 				|______________|	=
 * b.6 						|___|		<
*/
int fileMapTreeSearchCompare(const void *pa, const void *pb) {
    const struct fileMapTreeNode* a = pa;
    const struct fileMapTreeNode* b = pb;
    if ((unsigned long)(a->start) > (unsigned long)(b->start)&&
            (unsigned long)(a->start) > (unsigned long)(b->tail))
        return 1;
    if ((unsigned long)(a->start) < (unsigned long)(b->start)&&
            (unsigned long)(a->tail) < (unsigned long)(b->start))
        return -1;
    return 0;
}

//xzjin 遍历打印红黑树节点里的记录
void listRecTreeNode(void *pageNum){
	struct recTreeNode node,**pt;
	struct recBlockEntry *ent, *mostRecentModifiedEnry;
	node.pageNum = pageNum;
	pt = tfind(&node, &recTreeRoot, recCompare);
	if(!pt){
		MSG("page:%p not in tree.\n", pageNum);
		return;
	}
    struct tailhead* listHead = pt[0]->listHead;
	mostRecentModifiedEnry = pt[0]->recModEntry;

	if(TAILQ_EMPTY(listHead)){
		MSG("page:%p list NULL.\n", pageNum);
		return;
	}

	MSG("Page:%p:\n", pageNum);
	TAILQ_FOREACH(ent, listHead, entries){
		struct memRec* recArr = ent->recArr;
		int tail = MEMRECPERENTRY;
		if(ent == mostRecentModifiedEnry)
			tail = pt[0]->memRecIdx;

		MSG("\nnew Array, %d length, element:%p\n", tail, ent);
		for(int i=0; i<tail; i++,recArr++){
			void* bufAddr __attribute__((unused));
			bufAddr = getAddr(pageNum, recArr->pageOffset);
			DEBUG("page off:%4d, fileOffset:%ld, addr:%p, fileName:%s, bufAddr:%p.\n",
				recArr->pageOffset, recArr->fileOffset, recArr, 
				recArr->fileName, bufAddr);
		}
	}
}

struct recTreeNode* addTreeNode(void* pageNum){

	//xzjin 因为tdelete会在函数内部释放空间，所以在这里暂时用单独的malloc 
//	struct recTreeNode *treeNode = malloc(sizeof(struct recTreeNode));
	struct recTreeNode *treeNode = allocateRecTreeNode();
//	struct recTreeNode **insertResult;
	//DEBUG("Insert tree node, addr:%p, pageNum: %lu\n", treeNode, pageNum);
	assert(treeNode!=NULL);

//			xzjin 初始化链表结构，因为struct自己申请的内存会在程序结束被回收，用malloc代替
//			SLIST_HEAD(slisthead, entry) head = SLIST_HEAD_INITIALIZER(head);
//			SLIST_HEAD(slisthead,  recBlockEntry) head_obj = SLIST_HEAD_INITIALIZER(head_obj);
//			SLIST_INIT(&head_obj);                     /* Initialize the queue. */
//			SLIST_HEAD(slisthead,  recBlockEntry) *head = &head_obj;
//				#define	SLIST_HEAD(name, type)	
//				struct name {	
//					struct type *slh_first;	/* first element */	
//				}
//				struct recBlockEntry{
//					struct memRec* recArr;
//					SLIST_ENTRY(recBlockEntry) entries;     /* Singly-linked List. */
//				};
//			    struct slisthead{
//			        struct  recBlockEntry *slh_first;	/* first element */	
//			    };
//
//		    struct slisthead *head = malloc(sizeof(struct slisthead));
//			TAILQ_HEAD(tailhead, entry) head = TAILQ_HEAD_INITIALIZER(head);
//			TAILQ_HEAD(tailhead,  recBlockEntry) head = TAILQ_HEAD_INITIALIZER(head);
//			#define TAILQ_HEAD(name, type)	_TAILQ_HEAD(name, struct type,)
//			#define	_TAILQ_HEAD(name, type, qual)					
//			struct name {								
//				qual type *tqh_first;
//				qual type *qual *tqh_last;
//			}
//			#define TAILQ_HEAD(name, type)	_TAILQ_HEAD(name, struct type,)
//			#define	TAILQ_HEAD_INITIALIZER(head)					
//				{ NULL, &(head).tqh_first }

	struct tailhead *head = allocateTailHead();
	TAILQ_INIT(head);

    struct recBlockEntry *ent = allocateRecBlockEntry();	/* Insert at the head. */
	ent->recArr = allocateMemRecArr();
	//xzjin 这里是insert head，可能是当时为了让新数据在前面
	TAILQ_INSERT_HEAD(head, ent, entries);

	//xzjin 添加到树里
	treeNode->pageNum = pageNum;
	treeNode->listHead = head;
	treeNode->recModEntry = ent;	//used when insert to tail
	treeNode->memRecIdx = 0;
	tsearch(treeNode, &recTreeRoot, recCompare);
//	DEBUG_FILE("add tree node:%p, pageNum:%p\n", treeNode, pageNum);
//	if(insertResult){
//		DEBUG("Inserted, address:%p, instered pageNum:%lu, origin pageNum:%lu.\n",
//			insertResult[0], insertResult[0]->pageNum, treeNode->pageNum);
//	}else{
//		DEUBG("insert NULL.\n");
//	}
//	MSG("Inserted\n");
	return treeNode;
}

struct recTreeNode** findAndAddRecTreeNode(struct recTreeNode *recp){
	struct recTreeNode **destRes = tfind(recp, &recTreeRoot, recCompare);
	if(!destRes){
		addTreeNode(recp->pageNum);
		destRes = tfind(recp, &recTreeRoot, recCompare);
//		DEBUG("destRes: %p, intend intert pageNum:%lu, actual inserted pageNum: %lu.\n",
//			 destRes, recp->pageNum, destRes[0]->pageNum);
	}
	return destRes;
}

void reclaimSpecialRecTreeNode (struct recTreeNode** nodep){
	struct recTreeNode delNode;
	struct recTreeNode* node;
	struct tailhead *head;
	struct recBlockEntry *ent;

	node = *nodep;
	head = node->listHead;
	ent = TAILQ_FIRST(head);
	//if(TAILQ_EMPTY(head))
	//xzjin list是空或者只有一个元素且这个列表是空的
	if(node->memRecIdx==0 && !(TAILQ_NEXT(ent, entries))){
		reclamedRecNode++;
		delNode.pageNum = node->pageNum;
		withdrawMemRecArr(ent->recArr);
		TAILQ_REMOVE(head, TAILQ_FIRST(head), entries);
		withdrawRecBlockEntry(ent);
		withdrawTailHead(head);
		memset(node, 0, sizeof(struct recTreeNode));
		withdrawRecTreeNode(node);
		tdelete(&delNode, &recTreeRoot, recCompare);
	}
}

void reclaimRecTreeNode(const void *nodep, const VISIT which, const int depth){

    switch (which) {
    case preorder:
        break;
    case postorder:
		reclaimSpecialRecTreeNode ((struct recTreeNode **)nodep);
        break;
    case endorder:
        break;
    case leaf:
		reclaimSpecialRecTreeNode((struct recTreeNode **)nodep);
        break;
    }
}


void recTreeNodeGrabageReclaim(){
	reclamedRecNode = 0;
	twalk(recTreeRoot, listRecMapActionDetail);
	MSG("Reclaimed recNode:%lu\n", reclamedRecNode);
}
