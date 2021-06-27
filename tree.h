#ifndef __AWN_TREE__
#define __AWN_TREE__

#include "common.h"
#include "mem.h"

unsigned long reclamedRecNode;

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
	int fd;
//	unsigned int ref;	//对这个文件的引用数
//	bool userUnmapped;	//用户是否已经对这个文件unmmap了
};

void listFileMapNodeAction(const void *nodep, const VISIT which, const int depth);
void listFileMapTree();
void showRecMapNode(struct recTreeNode **nodep);
void listRecMapAction(const void *nodep, const VISIT which, const int depth);
void showRecMapNodeDetail(struct recTreeNode **nodep);
void listRecMapActionDetail(const void *nodep, const VISIT which, const int depth);
void listRecTree();
void listRecTreeDetail();
void checkEmptyRecTreeNode();
int recCompare(const void *pa, const void *pb);
int fileMapTreeInsDelCompare(const void *pa, const void *pb);
int fileMapNameTreeInsDelCompare(const void *pa, const void *pb);
int fileMapTreeSearchCompare(const void *pa, const void *pb);
//xzjin 遍历打印红黑树节点里的记录
void listRecTreeNode(void *pageNum);
struct recTreeNode* addTreeNode(void* pageNum);
struct recTreeNode** findAndAddRecTreeNode(struct recTreeNode *recp);

void reclaimSpecialRecTreeNode (struct recTreeNode** nodep);
void reclaimRecTreeNode(const void *nodep, const VISIT which, const int depth);
void recTreeNodeGrabageReclaim();

#endif //__AWN_TREE__