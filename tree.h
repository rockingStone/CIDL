#ifndef __AWN_TREE__
#define __AWN_TREE__

#include "common.h"

void listFileMapNodeAction(const void *nodep, const VISIT which, const int depth);
void listFileMapTree();
void showRecMapNode(struct recTreeNode **nodep);
void listRecMapAction(const void *nodep, const VISIT which, const int depth);
void showRecMapNodeDetail(struct recTreeNode **nodep);
void listRecMapActionDetail(const void *nodep, const VISIT which, const int depth);
void listRecTree();
void listRecTreeDetail();
int recCompare(const void *pa, const void *pb);
int fileMapTreeInsDelCompare(const void *pa, const void *pb);
int fileMapNameTreeInsDelCompare(const void *pa, const void *pb);
int fileMapTreeSearchCompare(const void *pa, const void *pb);
//xzjin 遍历打印红黑树节点里的记录
void listRecTreeNode(void *pageNum);
struct recTreeNode* addTreeNode(void* pageNum);
struct recTreeNode** findAndAddRecTreeNode(struct recTreeNode *recp);

#endif //__AWN_TREE__