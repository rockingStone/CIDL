#include "awn.h"
#include "common.h"
#include <sys/mman.h>

#define PATCH 0
#define DEL_REALLOC_ADDR 0
#define USE_TS_FUNC 1
#define USE_FMAP_CACHE 0
#define REC_INSERT_TEST 0
#define CMPWRITE
#undef TS_MEMCPY_CMPWRITE

/**xzjin 大概是一个全局的操作库，每个元素包含了一个模块的操作
 */
struct Fileops_p* _hub_fileops_lookup[MAX_FILEOPS];

//Debug help func
void init(void);
void listRecTreeNode(void *pageNum);
void listFileMapTree();
void listRecTree();
void listRecTreeDetail();

//exported function
int ts_open(const char *path, int oflag, ...);
int ts_openat(char* dirName, int dirfd, const char *path, int oflag, ...);
FILE *ts_fopen(const char *filename, const char *modes);
int ts_close (int fd);
ssize_t ts_write(int file, void *buf, size_t length);
size_t ts_fwrite (const void *buf, size_t size, size_t n, FILE *s);
void* ts_memcpy_traced(void *dest, void *src, size_t n);
void* ts_memcpy(void *dest, void *src, size_t n);
ssize_t ts_read(int fd, void *buf, size_t nbytes);
void* ts_realloc(void *ptr, size_t size, void* tail);

//Address process func
inline void *getAddr(void* pageNum, int offset) __attribute__((always_inline));
inline void *addr2PageNum(void* addr) __attribute__((always_inline));
inline void *addr2PageBegin(void* addr) __attribute__((always_inline));
inline void *addr2PageTail(void* addr) __attribute__((always_inline));
inline short addr2PageOffset(void* addr) __attribute__((always_inline));

instrumentation_type compare_mem_time, remap_mem_rec_time, ts_write_time;
instrumentation_type mem_from_file_trace_time, mem_from_mem_trace_time;
instrumentation_type memcmp_asm_time, tfind_time, cmp_write_time;
instrumentation_type fileMap_time, fileUnmap_time, insert_rec_time;
instrumentation_type ts_memcpy_tfind_file_time;

static void listFileMapNodeAction(const void *nodep, const VISIT which, const int depth){
    struct fileMapTreeNode *fmNode;

    switch (which) {
    case preorder:
        break;
    case postorder:
        fmNode = *(struct fileMapTreeNode **) nodep;
		MSG("map start:%lu, map tail:%lu, map offset:%ld, fileName: %s, fileName addr:%lu, fmNodeaddr: %lu\n",
			fmNode->start, fmNode->tail, fmNode->offset, fmNode->fileName, fmNode->fileName, fmNode);
        break;
    case endorder:
        break;
    case leaf:
        fmNode = *(struct fileMapTreeNode **) nodep;
		MSG("map start:%lu, map tail:%lu, map offset:%ld, fileName: %s, fileName addr:%lu, fmNodeaddr: %lu\n",
			fmNode->start, fmNode->tail, fmNode->offset, fmNode->fileName, fmNode->fileName, fmNode);
        break;
    }
}

void listFileMapTree() {
	twalk(fileMapTreeRoot,listFileMapNodeAction);
}

static void showRecMapNode(struct recTreeNode **nodep){
    struct recTreeNode *fmNode;
    struct tailhead* head;
	fmNode = *(struct recTreeNode **) nodep;
	head = fmNode->listHead;

	MSG("pageNum: %lu, list head:%lu, memRecIdx:%2d, struct address: %lu.\n",
		fmNode->pageNum, head, fmNode->memRecIdx, fmNode);
}

static void listRecMapAction(const void *nodep, const VISIT which, const int depth){

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

static void showRecMapNodeDetail(struct recTreeNode **nodep){
    struct recTreeNode *fmNode;
    struct tailhead* head;
	fmNode = *(struct recTreeNode **) nodep;
	head = fmNode->listHead;

	fmNode = *(struct recTreeNode **) nodep;
	MSG("pageNum: %lu, list head:%lu, memRecIdx:%2d, struct address: %lu.\n",
		fmNode->pageNum, head, fmNode->memRecIdx, fmNode);
	listRecTreeNode(fmNode->pageNum);
}

static void listRecMapActionDetail(const void *nodep, const VISIT which, const int depth){

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
	twalk(recTreeRoot,  listRecMapAction);
}

void listRecTreeDetail(){
	twalk(recTreeRoot,  listRecMapActionDetail);
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

extern int memcmp_avx2_asm(const void *s1, const void *s2, size_t n, void* firstDiffPos);	
//xzjin 因为IDX开始存放的是缓存大小，所以先减后返回
inline struct recTreeNode *allocateRecTreeNode() __attribute__((always_inline));
inline struct recTreeNode *allocateRecTreeNode(){
	assert(RECTREENODEPOOLIDX>0);
	RECTREENODEPOOLIDX--;
	return RECTREENODEPOOLPTR[RECTREENODEPOOLIDX];
}

//xzjin TODO 现在对于recTreeNode没有回收方式，同样tailHead也是
void withdrawRecTreeNode(struct recTreeNode * ptr){
	assert(RECTREENODEPOOLIDX<RECTREENODEPOOLSIZE);
	RECTREENODEPOOLPTR[RECTREENODEPOOLIDX] = ptr;
	RECTREENODEPOOLIDX++;
}

inline struct fileMapTreeNode *allocateFileMapTreeNode() __attribute__((always_inline));
inline struct fileMapTreeNode *allocateFileMapTreeNode(){
	assert(FILEMAPTREENODEPOOLIDX>0);
	FILEMAPTREENODEPOOLIDX--;
	return FILEMAPTREENODEPOOLPTR[FILEMAPTREENODEPOOLIDX];
}

void withdrawFileMapTreeNode(struct fileMapTreeNode * ptr){
	assert(FILEMAPTREENODEPOOLIDX<FILEMAPTREENODEPOOLSIZE);
	FILEMAPTREENODEPOOLPTR[FILEMAPTREENODEPOOLIDX] = ptr;
	FILEMAPTREENODEPOOLIDX++;
}

inline struct tailhead *allocateTailHead() __attribute__((always_inline));
inline struct tailhead *allocateTailHead(){
	assert(TAILHEADPOOLIDX>0);
	TAILHEADPOOLIDX--;
	return TAILHEADPOOLPTR[TAILHEADPOOLIDX];
}

void withdrawTailHead(struct tailhead *ptr){
	assert(TAILHEADPOOLIDX<TAILHEADPOOLSIZE);
	TAILHEADPOOLPTR[TAILHEADPOOLIDX] = ptr;
	TAILHEADPOOLIDX++;
}

//inline struct slisthead *allocateSlistHead() __attribute__((always_inline));
//inline struct slisthead *allocateSlistHead(){
//	assert(SLISTHEADPOOLIDX>0);
//	SLISTHEADPOOLIDX--;
//	return SLISTHEADPOOLPTR[SLISTHEADPOOLIDX];
//}
//
//void withdrawSlistHead(struct slisthead *ptr){
//	assert(SLISTHEADPOOLIDX<SLISTHEADPOOLSIZE);
//	SLISTHEADPOOLPTR[SLISTHEADPOOLIDX] = ptr;
//	SLISTHEADPOOLIDX++;
//}

inline struct recBlockEntry *allocateRecBlockEntry() __attribute__((always_inline));
inline struct recBlockEntry *allocateRecBlockEntry(){
	assert(RECBLOCKENTRYPOOLIDX>0);
	RECBLOCKENTRYPOOLIDX--;
	return RECBLOCKENTRYPOOLPTR[RECBLOCKENTRYPOOLIDX];
}

void withdrawRecBlockEntry(struct recBlockEntry *ptr){
	assert(RECBLOCKENTRYPOOLIDX<RECBLOCKENTRYPOOLSIZE);
	RECBLOCKENTRYPOOLPTR[RECBLOCKENTRYPOOLIDX] = ptr;
	RECBLOCKENTRYPOOLIDX++;
}

inline struct memRec *allocateMemRecArr() __attribute__((always_inline));
inline struct memRec *allocateMemRecArr(){
	assert(RECARRPOOLIDX>0);
	RECARRPOOLIDX--;
	return RECARRPOOLPTR[RECARRPOOLIDX];
}

void withdrawMemRecArr(struct memRec *ptr){
	assert(RECARRPOOLIDX<RECARRPOOLSIZE);
	RECARRPOOLPTR[RECARRPOOLIDX] = ptr;
	RECARRPOOLIDX++;
}

inline void *getAddr(void* pageNum, int offset){
	return (void *)(((unsigned long long)pageNum<<PAGENUMSHIFT)+offset);
}

inline void *addr2PageNum(void* addr){
	return (void *)((unsigned long long)addr>>PAGENUMSHIFT);
}

inline void *addr2PageBegin(void* addr){
	return (void *)((unsigned long long)addr & PAGENUMMASK);
}

inline void *addr2PageTail(void* addr){
	return (void *)((unsigned long long)(addr+PAGESIZE) & PAGENUMMASK);
}

inline short addr2PageOffset(void* addr){
	return (short)((unsigned long long)(addr) & PAGEOFFMASK);
}

//xzjin 遍历打印红黑树节点里的记录
void listRecTreeNode(void *pageNum){
	struct recTreeNode node,**pt;
	struct recBlockEntry *ent, *mostRecentModifiedEnry;
	node.pageNum = pageNum;
	pt = tfind(&node, &recTreeRoot, recCompare);
	if(!pt){
		MSG("page:%lu not in tree.\n", pageNum);
		return;
	}
    struct tailhead* listHead = pt[0]->listHead;
	mostRecentModifiedEnry = pt[0]->recModEntry;

	if(TAILQ_EMPTY(listHead)){
		MSG("page:%lu list NULL.\n", pageNum);
		return;
	}

	MSG("Page:%lu:\n", pageNum);
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

inline void writeRec(unsigned long fileOffset, void* src, void* dest,
	 void* pageNum, char* fileName) __attribute__((always_inline));
inline void writeRec(unsigned long fileOffset, void* src, void* dest,
	 void* pageNum, char* fileName){
	int idx = *(lastRec.lastIdx);
	struct memRec* pt = lastRec.lastMemRec;
	unsigned long pageOffset = (unsigned long)dest-((unsigned long)pageNum<<PAGENUMSHIFT);
	//xzjin 注意开始的情况，如果lastRec是第一次用，还没有设置怎么办
	if(idx<MEMRECPERENTRY){		//索引小于边界
		pt[idx].fileName= fileName;
		pt[idx].fileOffset = fileOffset;
		//因为在ts_memcpy_traced有时候会把pageOffset>page size的地址放到这里，所以这么做
		pt[idx].pageOffset = pageOffset;
		//xzjin update lastRec
//			lastRec.lastPageNum = ***;
//			lastRec.lastMemRec = ***;
		*(lastRec.lastIdx) = idx+1;
	}else{	//索引大于边界，追加list
//		DEBUG("Append list node\n");
		struct recTreeNode tmpRecTreeNode,**rec;
		//xzjin 这个是实际包含memRec
		struct memRec *recArr;
		//xzjin 这个是链表的结构，里面只包含一个指向memRec的指针
		struct recBlockEntry *rbe;
		tmpRecTreeNode.pageNum = pageNum;
		rec = tfind(&tmpRecTreeNode, &recTreeRoot,recCompare);
		assert(rec!=NULL);
//		recArr = malloc(sizeof(struct memRec)*MEMRECPERENTRY);
//		rbe = malloc(sizeof(struct recBlockEntry));
		recArr = allocateMemRecArr();
		rbe = allocateRecBlockEntry();
		rbe->recArr = recArr;
		//xzjin 插入链表插在最前面，这样比较的时候是从最新拷贝的记录开始比较的
		//xzjin 也就是说越大的内存地址在越前面
		TAILQ_INSERT_HEAD(rec[0]->listHead, rbe, entries);
		rec[0]->recModEntry = rbe;
		
		//xzjin update Rec
		recArr[0].fileName= fileName;
		recArr[0].fileOffset = fileOffset;
		//因为在ts_memcpy_traced有时候会把pageOffset>page size的地址放到这里，所以这么做
		recArr[0].pageOffset = pageOffset;

		//xzjin update lastRec
		lastRec.lastMemRec = recArr;
		*(lastRec.lastIdx) = 1;
	}
	//MSG("PageNum: %lu, idx:%d\n", lastRec.lastPageNum, *(lastRec.lastIdx));
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

//xzjin src is UNUSED and UNCHECKED
inline void insertRec(unsigned long fileOffset, void* src, void* dest, char* fileName) __attribute__((always_inline));
inline void insertRec(unsigned long fileOffset, void* src, void* dest, char* fileName){
	void* pageNum = addr2PageNum(dest);

	//DEBUG("insterRec.\n");
	//xzjin 这里暂时没有问题，对多输出的情况可以也做一个缓存
	//xzjin 这里是同一页内容就追加，对重复写的内容怎么办？
	//xzjin 这里主要的作用是检查树里面有没有对应这个页的节点，如果没有要新建节点
//	if(pageNum==lastRec.lastPageNum){	//xzjin 是和上一次修改同一个页
//		writeRec(fileOffset, src, dest, pageNum, fileName);
//	}else
	{		//xzjin 和上次更新的不在同一个页
		struct recTreeNode node,**pt;
		node.pageNum = pageNum;
		pt = tfind(&node, &recTreeRoot, recCompare);
		if(UNLIKELY(pt)){	//xzjin 在树里找到了
			//MSG("Found in tree\n");
			//xzjin update lastRec, it is one of parameters that writeRec needs, 
			//must update lastRec before callwriteRec
			lastRec.lastPageNum = pageNum;
			lastRec.lastMemRec = pt[0]->recModEntry->recArr;
			//xzjin 注意这里是指向数字的指针
			lastRec.lastIdx = &(pt[0]->memRecIdx);
			//Check whether new address is smaller than older
			//If true, delete node content
			{
				struct memRec* lastMenRec = lastRec.lastMemRec;
				int idx = *lastRec.lastIdx;
				struct recBlockEntry *ent;
			    struct tailhead *head = pt[0]->listHead;
				assert(idx>=0);
				//TODO idx=0发生在
				//	1. memcpy treced 刚删除了所有内容
				//	2. memcpy treced 的dest的树节点之前是空的，现在刚插入内容
				//NOTE: bigger or equal, equal is important in some cases
				if(idx>0 && lastMenRec[idx-1].pageOffset>=addr2PageOffset(dest)){
					DEBUG("\nDeleting memory record list, pageNum:%lu.\n", pageNum);
//					listRecTreeNode(pageNum);
					TAILQ_FOREACH(ent, head, entries){
//						DEBUG("delete list head:%p, element:%p\n", head, ent);
						if(TAILQ_NEXT(ent, entries)){
							withdrawMemRecArr(ent->recArr);
							TAILQ_REMOVE(head, TAILQ_FIRST(head), entries);
							withdrawRecBlockEntry(ent);
						}
					}
					pt[0]->recModEntry =  TAILQ_FIRST(head);
					lastRec.lastMemRec = pt[0]->recModEntry->recArr;
					*(lastRec.lastIdx) = 0;
				}
			}
			//DEBUG("insterRec if\n");
			writeRec(fileOffset, src, dest, pageNum, fileName);
//		    struct tailhead *head = pt[0]->listHead;

		}else{	//xzjin 在树里没找到
			//MSG("Not found in tree\n");
			struct recTreeNode *treeNode = addTreeNode(pageNum);
			//xzjin update lastRec
			lastRec.lastPageNum = pageNum;
			lastRec.lastMemRec = treeNode->recModEntry->recArr;
			lastRec.lastIdx = &(treeNode->memRecIdx);

			if(UNLIKELY(*lastRec.lastIdx<0 ||*lastRec.lastIdx>MEMRECPERENTRY)){
				MSG("lastRec.lastIdx ERROR\n");
				exit(-1);
			}
			writeRec(fileOffset, src, dest, pageNum, fileName);
		}
	}
	if(UNLIKELY(*lastRec.lastIdx<0 ||*lastRec.lastIdx>MEMRECPERENTRY)){
		MSG("lastRec.lastIdx ERROR\n");
	}
}

void insertPro(void *addr,int pro){
//	void * pageNum = addr2PageNum(addr);
//	int index =((unsigned long)pageNum) %PROMAPPERSIZE;
//	struct protRec p;
//	p.pageNum = pageNum;
//	p.pro = pro;
//	p.next = proMapper[index];
//	MSG("p.next=%p\n",p.next);
//	proMapper[index] = &p;
}

int getPro(void *addr){
//	void * pageNum = addr2PageNum(addr);
//	int index =((unsigned long)pageNum) %PROMAPPERSIZE;
//	struct protRec *p = proMapper[index];
//	while(p){
//		if(p->pageNum == pageNum) return p->pro;
//		p = p->next;
//	}
	return PROT_READ|PROT_WRITE;
}

int segvCounter=0;
void segvhandler(int signum, siginfo_t *info, void *context) {
//	struct p2fmap *p2f;
	DEBUG("Fault address: %p\n", info->si_addr);
	switch (info->si_code) {

	case SEGV_PKUERR:
//		segvCounter++;
	/** xzjin For PKU protected mamory , we change current page protect 
	 * to non and delete entire current page map.*/
		DEBUG("Address %p PKU protected.\n",info->si_addr);
//		printTwoLevel();
		/** 这里设置完pkey不管用,下次执行依然是pkey=1,是不是因为handler的
		 * 处理是在内核态或者不是同一个执行线程*/
		DEBUG("set Unprotected key for mem addr:%p\n",info->si_addr);
#if USE_PKEY
		void *pageStart = (void*)(((unsigned long long)(info->si_addr))&PAGENUMMASK);
		if(pkey_mprotect(pageStart, PAGESIZE, getPro(pageStart),
				 nonProKey)){
			ERROR("Fault in mprotect.\n");
		   perror("Couldn’t mprotect\n");
		   exit(errno);
		}
//		deleteMapExtent(pageStart, PAGESIZE);
		DEBUG("PKU segv err processed\n",info->si_addr);
//		if(segvCounter>9) sigaction(SIGSEGV, &defaction, NULL);
#endif //USE_PKEY
		break;

	case SEGV_BNDERR:
		MSG("Address %p failed bound checks.\n",info->si_addr);
  		sigaction(SIGSEGV, &defaction, NULL);
		break;

	case SEGV_MAPERR:
		MSG("Address %p not mapped.\n",info->si_addr);
  		sigaction(SIGSEGV, &defaction, NULL);
		break;

	case SEGV_ACCERR:
		MSG("Access to this address is not allowed.\n");
  		sigaction(SIGSEGV, &defaction, NULL);
		break;

	default:
		MSG("Unknown reason.\n");
  		sigaction(SIGSEGV, &defaction, NULL);
		break;
  	}
//	p2f = findmap2f(info->si_addr);
//	if(p2f){
//		if (mprotect(p2f->start, (p2f->end)-(p2f->start), 
//					PROT_READ|PROT_WRITE)) {
//			ERROR("Fault in mprotect.\n");
//		    perror("Couldn’t mprotect\n");
//		    exit(errno);
//		}else{
//			MSG("Change addr %p~%p to read|write.\n",
//					p2f->start,p2f->end);
//			delMap(p2f);
//		}
//	}
 	/* unregister and let the default action occur */
//  	sigaction(SIGSEGV, &defaction, NULL);
}

//void p2fAppend(void *processAddr, long long fileoffset, long long length,
//				 char* absolutePath, short keyId){
//	DEBUG("Entering p2fAppend, processAddr:%p.\n", processAddr);
//	insertMapExtent( processAddr, processAddr + length, fileoffset,
//					 absolutePath, keyId);
//	void *pageNum = addr2PageNum(processAddr);
//	//xzjin 不同的页有不同的子树，这里是以页为搜索单位的
//	RBNodePtr search = searchTree(pageNum2Tree, addr2PageNum(processAddr));
//	if(! search){
//		DEBUG("process Addr is not in tree\n");
//		RBNodePtr root = creatRBTree();
//		insertMapExtent(&root, processAddr, processAddr + length, 
//		       fileoffset, absolutePath, keyId);
//
//		insertMapTree(&pageNum2Tree, addr2PageNum(processAddr), root);
//	}else{
//		//TODO xzjin 增加重复地址检测的内容
//		insertMapExtent(&(search->innerMapTree), processAddr, processAddr + length, 
//		       fileoffset, absolutePath, keyId);
//	}
//	DEBUG("process Addr:%p is added to tree\n", processAddr);
//	return;
//}
//xzjin 这个函数没有写完
//xzjin TODO add lock
//xzjin 和下面函数不一样的地方在于这里是搜索在一个范围里的第一个映射
//RBNodePtr findmapExte2f(void *addr,unsigned long long len,
//		int *protected, int *proKey){
//	DEBUG("Entering find findmap2 function.\n",addr);
//	void *end = addr + len;
//	struct p2fmap *pos = NULL;
//	*protected =0;
//
//	RBNodePtr search = NULL;
//	//xzjin Find mapTree and update start to first page where overlap may exists.
//	for(;(!search || !search->innerMapTree || search->innerMapTree == TNULL)&& addr<end;
//			addr = (void *)(((unsigned long long )addr & PAGENUMMASK)+4096)){
//		search = searchTree(pageNum2Tree, addr2PageNum(addr));
//	}
//
//	if(search){
//		if(!isEmpty(search->innerMapTree)){
//			*protected = 1;
//			*proKey = search->innerMapTree->keyid;
//		}
////		DEBUG("Found page tree\n");
//		search = searchContainer(search->innerMapTree, addr);
//		if(search && search->end > addr){
//			DEBUG("Found in tree, start:%p,end:%p,keyid:%d\n",
//				search->start,search->end,search->keyid);
//			return search;
//		}
//	}
////	DEBUG("process Addr:%p is not in map\n",addr);
//	return NULL;
//}

//xzjin TODO add lock
/** Here pageMapTree is first level node, when delete it's 
 * second level node ,use &(*pageMapTree->innerMap) */
//RBNodePtr findmap2f(void *addr, RBNodePtr *pageMapTree, int *protected, int *proKey){
////	DEBUG("Entering find findmap2f function.\n",addr);
//	struct p2fmap *pos = NULL;
//	*protected =0;
//
//	RBNodePtr search = searchTree(pageNum2Tree, addr2PageNum(addr));
//	*pageMapTree = search;
//	if(search){
//		if(!isEmpty(search->innerMapTree)){
//			*protected = 1;
//			*proKey = search->innerMapTree->keyid;
//		}
////		DEBUG("Found page tree\n");
//		search = searchContainer(search->innerMapTree, addr);
//		if(search && search->end > addr){
//			DEBUG("Found in tree, start:%p,end:%p,keyid:%d\n",
//				search->start, search->end, search->keyid);
//			return search;
//		}
//	}
////	DEBUG("process Addr:%p is not in map\n",addr);
//	return NULL;
//}
//xzjin TODO Add lock
//xzjin TODO 这个delMap是不是要先释放所有子树的内容，然后再删除和释放本身
void delMap(RBNodePtr *root, RBNodePtr addr){
//	DEBUG("Delete map start:%p,end:%p,file:%s,keyId:%d\n",
//				node->start,node->end,node->abp,node->keyid);
//	deleteNodeFromAddr(root, addr);
//	RBNodePtr search = searchTree(pageNum2Tree, addr2PageNum(node->start));
//	if(!search){
//		ERROR("Here is no tree for addr:%p\n",node->start);
//		exit(-1);
//	}
//
//	deleteNode(&(search->innerMapTree), node->start);
	return;
}

// Creates the set of standard posix functions as a module.
__attribute__((constructor))void init(void) {
	execv_done = 0;
	int tmp;
	MSG("Initializing the libawn.so.\n");

	//xzjin Put dlopen before call to memcpy and mmap call.
	ts_write_size = 0;
 	ts_write_same_size = 0;
 	ts_write_not_found_size = 0;
	ts_metadataItem = 0;
	nvpWriteSize = 0;
	mmapSrcCache = calloc(MMAPCACHESIZE, sizeof(struct searchCache));
	mmapDestCache = calloc(MMAPCACHESIZE, sizeof(struct searchCache));
	FD2PATH = calloc(FILEMAPTREENODEPOOLSIZE, sizeof(char*));

//	lastDestInMap = NULL;
//	lastSrcInMap = NULL;
//	lastSrcInMapKey = 0;
//	lastDestInMapKey = 0;
	proKey = nonProKey = -1;
	lastTs_memcpyFmNode = NULL;
	leastRecFCache.idx=-1;
	leastRecFCache.leastUsedTime = INT32_MAX;

//	RBPool = (RBNodePtr)malloc(sizeof(struct RBNode)*RBPOOLSIZE);
//	if(!RBPool){
//		ERROR("Could not allocate space for RBPool.\n");
//		assert(0);
//	}
//	RBPoolPointer = (RBNodePtr* )malloc(sizeof(RBNodePtr)*RBPOOLSIZE);
//	if(!RBPoolPointer){
//		ERROR("Could not allocate space for RBPoolPointer.\n");
//		assert(0);
//	}
//	for(RBPPIdx=0; RBPPIdx<RBPOOLSIZE; RBPPIdx++){
//		RBPoolPointer[RBPPIdx] = RBPool+RBPPIdx;
//	}
//	RBPPIdx = RBPOOLSIZE-1;

	//xzjin Allocate pool for node
	{
		RECTREENODEPOOL = malloc(sizeof(struct recTreeNode)*RECTREENODEPOOLSIZE);
		if(!RECTREENODEPOOL){
			ERROR("Could not allocate space for RECTREENODEPOOL.\n");
			assert(0);
		}
		RECTREENODEPOOLPTR = malloc(sizeof(struct recTreeNode*)*RECTREENODEPOOLSIZE);
		if(!RECTREENODEPOOLPTR){
			ERROR("Could not allocate space for RECTREENODEPOOLPTR.\n");
			assert(0);
		}
		for(int i=0; i<RECTREENODEPOOLSIZE; i++){
			RECTREENODEPOOLPTR[i] = RECTREENODEPOOL+i;
		}
		RECTREENODEPOOLIDX = RECTREENODEPOOLSIZE;

		FILEMAPTREENODEPOOL = malloc(sizeof(struct fileMapTreeNode)*FILEMAPTREENODEPOOLSIZE);
		if(!FILEMAPTREENODEPOOL){
			ERROR("Could not allocate space for FILEMAPTREENODEPOOL.\n");
			assert(0);
		}
		FILEMAPTREENODEPOOLPTR = malloc(sizeof(struct fileMapTreeNode*)*RECTREENODEPOOLSIZE);
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

		TAILHEADPOOL = malloc(sizeof(struct tailhead)*TAILHEADPOOLSIZE);
		if(!TAILHEADPOOL){
			ERROR("Could not allocate space for TAILHEADPOOL.\n");
			assert(0);
		}
		TAILHEADPOOLPTR = malloc(sizeof(struct tailhead*)*TAILHEADPOOLSIZE);
		if(!TAILHEADPOOLPTR){
			ERROR("Could not allocate space for TAILHEADPOOLPTR.\n");
			assert(0);
		}
		for(int i=0; i<TAILHEADPOOLSIZE; i++){
			TAILHEADPOOLPTR[i] = TAILHEADPOOL+i;
		}
		TAILHEADPOOLIDX = TAILHEADPOOLSIZE;

		RECBLOCKENTRYPOOL = malloc(sizeof(struct recBlockEntry)*RECBLOCKENTRYPOOLSIZE);
		if(!RECBLOCKENTRYPOOL){
			ERROR("Could not allocate space for RECBLOCKENTRYPOOL.\n");
			assert(0);
		}
		RECBLOCKENTRYPOOLPTR = malloc(sizeof(struct recBlockEntry*)*RECBLOCKENTRYPOOLSIZE);
		if(!RECBLOCKENTRYPOOLPTR){
			ERROR("Could not allocate space for RECBLOCKENTRYPOOLPTR.\n");
			assert(0);
		}
		for(int i=0; i<RECBLOCKENTRYPOOLSIZE; i++){
			RECBLOCKENTRYPOOLPTR[i] = RECBLOCKENTRYPOOL+i;
		}
		RECBLOCKENTRYPOOLIDX = RECBLOCKENTRYPOOLSIZE;

		//xzjin 注意这里RECARRPOOL的类型是memRec的指针，不是memRec[MEMRECPERENTRY],申请的时候应该是+-MEMRECPERENTRY
		RECARRPOOL = malloc(sizeof(struct memRec)*MEMRECPERENTRY*RECARRPOOLSIZE);
		if(!RECARRPOOL){
			ERROR("Could not allocate space for RECARRPOOL.\n");
			assert(0);
		}
		RECARRPOOLPTR = malloc(sizeof(struct memRec*)*RECARRPOOLSIZE);
		if(!RECARRPOOLPTR){
			ERROR("Could not allocate space for RECARRPOOLPTR.\n");
			assert(0);
		}
		for(int i=0; i<RECARRPOOLSIZE; i++){
			RECARRPOOLPTR[i] = RECARRPOOL+MEMRECPERENTRY*i;
			RECARRPOOLTAIL = (unsigned long long)RECARRPOOLPTR[i];
		}
		RECARRPOOLIDX = RECARRPOOLSIZE;
	}

#if USE_PKEY
    nonProKey = pkey_alloc(0, 0);
    if(nonProKey<0){
        ERROR("nonProkey alloc Error.\n");
    }

#if REALPKEYPROTECT 
    proKey = pkey_alloc(0, PKEY_DISABLE_WRITE);
#else
	proKey = pkey_alloc(0, 0);
#endif
    if(proKey <0){
        ERROR("Prokey alloc Error.\n");
    }
#endif //USE_PKEY
	PAGESIZE = getpagesize();
	DEBUG("Page size is %d\n",PAGESIZE);

	tmp = PAGESIZE - 1;
	PAGENUMSHIFT = 0;
	while(tmp){
		PAGENUMSHIFT++;
		tmp >>=1;
	}
	DEBUG("PAGENUMSHIFT is %d\n",PAGENUMSHIFT);

	PAGEOFFMASK = (1<<PAGENUMSHIFT)-1;
	PAGENUMMASK = ~PAGEOFFMASK;
	DEBUG("PAGENUMMASK is %X\n",PAGENUMMASK);

	proMapper = NULL;
	/** xzjin 这里用pvalloc会有问题，不清楚原因, need to be set before 
	 * call to mmap,because mmap my call proInsert, which depends on this.*/
	proMapper = aligned_alloc(PAGESIZE, PROMAPPERSIZE*sizeof(struct protRec*));
	if(!proMapper){
		ERROR("Could not allocate space for proMapper.\n");
		assert(0);
	}
	memset(proMapper, 0, PROMAPPERSIZE*sizeof(struct protRec*));
	//memset(fMapCache, 0, FMAPCACHESIZE*sizeof(struct fMap));
	memset(fileMapCache, 0, FILEMAPCACHESIZE*sizeof(struct fileMapTreeNode*));

	//initTNULL();
	//pageNum2Tree = creatRBTree();
 	recTreeRoot = NULL;
 	fileMapTreeRoot = NULL;
 	fileMapNameTreeRoot = NULL;
	memset(&lastRec, 0, sizeof(lastRec));

 //out:
	execv_done = 0;
	
	MSG("%s: END\n", __func__);
}

//Add by xzjin
RETT_MMAP ts_mmap(INTF_MMAP) {
	void *ret;
	int err;
//	int idx;

	ret = mmap(CALL_MMAP);
	err = errno;
	if(UNLIKELY((long)ret==-1)){
		ERROR("mmap ERROR, %s, errno:%d\n", strerror(err), err);
	}
	DEBUG("_hub_mmap returned:%p fileName:%s, fd:%d\n",ret, fd2path[file%100], file);

	START_TIMING(fileMap_t, fileMap_time);
	//xzjin Here we need to skip pmem dir file
	if(!(prot & MAP_ANONYMOUS) && file>-1 ){
		char* path = GETPATH(file);
		//xzjin Here we skip mktmp file
		if(UNLIKELY(!strncmp("./pmem/DR-", path, 10))) goto out;
		//xzjin Recored page protection here.
//		int pro = prot & (PROT_READ | PROT_WRITE | PROT_EXEC );
		//struct fileMapTreeNode *treeNode = malloc(sizeof(struct fileMapTreeNode));
		struct fileMapTreeNode *treeNode = allocateFileMapTreeNode();
		treeNode->start = ret;
		treeNode->tail = ret+len;
		//xzjin since mmap operation need file to be 
		//open(through able to close file after operation 
		//complete), fd2path is still holding the origion file name, 
		// except another file descriptor has same mod value.
		//treeNode->fileName = fd2path[file%100];
		treeNode->fileName = GETPATH(file);
		treeNode->usedTime = 0;
		treeNode->offset = off;
		tsearch(treeNode, &fileMapTreeRoot, fileMapTreeInsDelCompare);
		tsearch(treeNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
//		MSG("_hub_mmap fileName:%s, start:%p, tail:%p, offset:%llu\n",
//			treeNode->fileName, treeNode->start, treeNode->tail,
//			treeNode->offset);
		//listFileMapTree();
		//xzjin TODO add to search tree
		/*
		insertPro(ret, pro);
		MSG("set:%p,len:%lld unwritable pkeys\n",ret,len);
		//	Mark the buffer read-only use proKey, but does not change pro,
		//  PROT_NONE is 0x0, so we do need to treat it as a special case. 
		if(pkey_mprotect((void*)((unsigned long long)ret&PAGENUMMASK),
				 len, pro, proKey)){
			ERROR("Fault in mprotect.\n");
		    perror("Couldn’t mprotect\n");
		    exit(errno);
		}
		p2fAppend(ret, off, len, fd2path[file], proKey);
		DEBUG_FILE("xzjin add mmap protect from file %s, offset %lu, length %lu, "
			"map pos: %lu, added to linked list and is mprotected.\n", 
			fd2path[file], off, len, ret);
		*/
	}
	END_TIMING(fileMap_t, fileMap_time);
out:
	return ret;
}

//Add by xzjin
//可以在unmap的时候删除映射吗
RETT_MUNMAP ts_munmap(INTF_MUNMAP) {
	
	int ret;
//	MSG("_hub_unmmap addr:%p,len:%llu\n", addr, len);

//	ret = _hub_fileops->MUNMAP(CALL_MUNMAP);
	ret = munmap(CALL_MUNMAP);
	DEBUG("_hub_munmap returned:%d.\n",ret);
	START_TIMING(fileUnmap_t, fileUnmap_time);
	struct fileMapTreeNode treeNode, **delNodep;
	treeNode.start = addr;
	//TODO 这里需要的到被删除的节点，释放节点和包含文件名的字符串
	delNodep = tfind(&treeNode, &fileMapTreeRoot, fileMapTreeInsDelCompare);
	if(delNodep && delNodep[0]){
		struct fileMapTreeNode *freep = *delNodep;
		//xzjin 同样，tdelete会改变delNodep指向位置的内容
		tdelete(&treeNode, &fileMapTreeRoot, fileMapTreeInsDelCompare);
		//这里用的比较算法还是fileMapTree的插入/删除算法
		tdelete(&treeNode, &fileMapNameTreeRoot, fileMapTreeInsDelCompare);
//		MSG("deleted node is: %p, fileName:%s.\n", freep, freep->fileName);
		free(freep->fileName);
		//free(freep);
		withdrawFileMapTreeNode(freep);
	}else{
		MSG("Nothing deleted.\n");
	}
	END_TIMING(fileUnmap_t, fileUnmap_time);

	/** xzjin TODO Add support to check whether page contians
	 *  other map, if not, set page protect to 0. */
	/** xzjin TODO If we add page lock in mmap, rember to
	 *  unlock it*/
	return ret;
}

//xzjin check whether ptr page is traced by recTree
int ts_isMemTraced(void* ptr){
	struct recTreeNode node,**pt;
	node.pageNum = addr2PageNum(ptr);
	pt = tfind(&node, &recTreeRoot, recCompare);
	return pt != NULL;
}

/** xzjin 16进制打印内存数据,每行16 byte
 * @mem 内存开始地址
 * @lineNr 打印的行数
*/
void printMem(void* mem, int lineNr){
		int i=0;
		for(;i<lineNr;i++){
			__u8* id = (__u8*)(mem+16*i);
			MSG("%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
				 *(__u8*)((unsigned long long)id+0), *(__u8*)((unsigned long long)id+1),
				 *(__u8*)((unsigned long long)id+2), *(__u8*)((unsigned long long)id+3),
				 *(__u8*)((unsigned long long)id+4), *(__u8*)((unsigned long long)id+5),
				 *(__u8*)((unsigned long long)id+6), *(__u8*)((unsigned long long)id+7),
				 *(__u8*)((unsigned long long)id+8), *(__u8*)((unsigned long long)id+9),
				 *(__u8*)((unsigned long long)id+10), *(__u8*)((unsigned long long)id+11),
				 *(__u8*)((unsigned long long)id+12), *(__u8*)((unsigned long long)id+13),
				 *(__u8*)((unsigned long long)id+14), *(__u8*)((unsigned long long)id+15));
		}
}

//compare and call write to file metedata
inline unsigned long cmpWrite(unsigned long bufCmpStart, struct memRec *mrp, void* buf,
		  unsigned long tail, void** diffPos,
		  int fd) __attribute__((always_inline));
inline unsigned long cmpWrite(unsigned long bufCmpStart, struct memRec *mrp, void* buf,
		  unsigned long tail, void** diffPos, int fd){
	START_TIMING(cmp_write_t, cmp_write_time);
	
	unsigned long fileCmpStart;
	unsigned long cmpLen;
	unsigned long addLen = 0;	//same content length	
	struct fileMapTreeNode fmNode, **fmSearNodep, *fmTarNode;
//	MSG("CMP write, fileOffset:%lu, pageOffset:%d, fileName:%s.\n",
//		mrp[i].fileOffset, mrp[i].pageOffset, mrp[i].fileName);

#ifdef CMPWRITE
	//TODO xzjin 这里文件应该有很强的局部性，可以用一个文件节点缓存
	fmNode.fileName = mrp->fileName;
	fmSearNodep = tfind(&fmNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
//	assert(fmSearNodep);
	//zxjin TODO maybe add UNLIKELY?
	if(UNLIKELY(!fmSearNodep)) return (~0);
	fmTarNode = *fmSearNodep;

	//xzjin 检查文件偏移对不对
	if(LIKELY(fmTarNode->offset <= mrp->fileOffset)){
		//xzjin 文件映射的开始+内存追踪的文件偏移-文件映射的偏移
		fileCmpStart = (unsigned long)(fmTarNode->start+mrp->fileOffset-fmTarNode->offset);
#if PATCH
		//xzjin 对于patch命令，fileCmpStart把对比位置设置到实际写的位置，而不是记录位置
		fileCmpStart += (unsigned long)buf-bufCmpStart;
		bufCmpStart = (unsigned long)buf;
#endif //PATCH
		cmpLen = tail-bufCmpStart;
		START_TIMING(memcmp_asm_t, memcmp_asm_time);
		//xzjin TODO 可以改成无论比较结果，都设置diffPos，减少比较后面的if语句
		int cmpRet = memcmp_avx2_asm((void*)bufCmpStart, (void*)fileCmpStart, cmpLen, diffPos);
		END_TIMING(memcmp_asm_t, memcmp_asm_time);
		if(cmpRet){		//different
			addLen = (tail<(unsigned long)*diffPos?tail:(unsigned long)*diffPos) - bufCmpStart;
		}else{		//same
			addLen = cmpLen;
			*diffPos = (void*)tail;
		}
		ts_metadataItem++;
//		MSG("Cmp write: same len:%lu, buf:%p, file buf:%p, diffPos:%lu, cmpRet:%d, fd:%d, fileName:%s\n\n",
//			addLen, bufCmpStart, fileCmpStart, *diffPos, cmpRet, fd, fmTarNode->fileName);
		ts_write_same_size += addLen;
	}else{
		MSG("ts_write fMapCache not hit\n");
	}
//	END_TIMING(cmp_write_t, cmp_write_time);
#endif //CMPWRITE
	return addLen;
}

ssize_t ts_write(int file, void *buf, size_t length){
	void *t = buf;
	unsigned long tail = (unsigned long)buf+length;
	unsigned long long start = (unsigned long long)addr2PageNum(t);
	unsigned long long end = (unsigned long long)addr2PageNum((void*)((unsigned long long)buf+length));
	struct recTreeNode searchNode;
	struct memRec *mrp;
	void* diffPos = 0;
	void* bufCmpStart;
	unsigned long sameLen;
	int sameContentTimes = 0;
	int curWriteSameLen = 0;
	int toTailLen;
	int idx; 

	//TODO xzjin 这个是不是要在用户空间维护一下，太耗时了
	//off_t fpos = lseek(file, 0, SEEK_CUR);
	ts_write_size += length;
	START_TIMING(ts_write_t, ts_write_time);
//	unsigned long ret =  _hub_fileops->WRITE(CALL_WRITE);
	unsigned long ret = write(CALL_WRITE);
	END_TIMING(ts_write_t, ts_write_time);
//	MSG("ts_write: buf:%p, file:%d, len: %llu, fileName:%s\n", 
//		buf, file, length, GETPATH(file));
//	listRecTreeDetail();
#if USE_TS_FUNC 
	START_TIMING(compare_mem_t, compare_mem_time);
	//xzjin 因为是比较连续的地址，所以不用在开始重新初始化diffPos变量
	for(int i=0; start<=end; start++,i++){
//		int writeLenCurPage = 4096;
		int sameLenCurPage = 0;
		int notFoundLen;
		toTailLen = length-((unsigned long)getAddr((void*)start,0)-(unsigned long)buf);
//		float samePercent;
//		DEBUG("compare, Page num:%lu.\n", start);
		if((unsigned long)getAddr((void*)start, 0)<(unsigned long)diffPos) continue;
//		if(startPageNum == start){
//			writeLenCurPage = 4096-addr2PageOffset(buf);
//		}else if(end == start){
//			writeLenCurPage = addr2PageOffset((void*)tail);
//		}
		searchNode.pageNum = (void*)start;
//		START_TIMING(tfind_t, tfind_time);
		struct recTreeNode **res = tfind(&searchNode, &recTreeRoot, recCompare);
//		END_TIMING(tfind_t, tfind_time);
		if(res){
//			listRecTreeNode((void*)start);
			struct recBlockEntry *blockEntry = TAILQ_LAST(res[0]->listHead, tailhead);
			//xzjin 注意这里有个foreach，这个有效吗？正确吗?
			while(blockEntry){
				int i=0;
				mrp = blockEntry->recArr;
				if(res[0]->recModEntry == blockEntry ){
					idx = res[0]->memRecIdx;
				}else{
					idx = MEMRECPERENTRY;
				}					
				//xzjin 过滤掉比buf小的地址
				while (((unsigned long)getAddr((void*)start, mrp->pageOffset) < (unsigned long) buf) &&
					i<idx ) {
					mrp++; i++;
				}
				
				for(; i<idx; mrp++,i++){
					DEBUG("i=%d, mrp:%lu.\n", i, mrp);
					bufCmpStart = getAddr((void*)start, mrp->pageOffset);
					if((unsigned long)bufCmpStart < (unsigned long) diffPos) continue;
					if((unsigned long)bufCmpStart > (unsigned long) tail) break;
					sameLen = cmpWrite((unsigned long)bufCmpStart, mrp, buf, tail, &diffPos, file);
					sameContentTimes += sameLen?1:0;
					sameLenCurPage += sameLen;
					curWriteSameLen +=sameLen;
				}
				blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
			}

//delTail:
//			samePercent = ((double)sameLenCurPage/writeLenCurPage)*100;
//			printf("same percentage this page: %.2f%%, pageLen:%d, samelen:%d, writePageNum:%lu, curPageNum:%lu.\n",
//				samePercent, writeLenCurPage, sameLenCurPage,
//				addr2PageNum(buf), start);
			//xzjin 写完一个页就删除对应的映射结构
			//xzjin TODO这里是不是可以等到写对比不同的时候再删除，不是写完就删除
			//对一个地址多次写入的情况会有好处
			/*
			while (!TAILQ_EMPTY(headp)) {           // List Deletion.
				rbp = TAILQ_FIRST(headp);
				withdrawMemRecArr(rbp->recArr);
				TAILQ_REMOVE(headp, TAILQ_FIRST(headp), entries);
				withdrawRecBlockEntry(rbp);
			}
			withdrawTailHead(headp);
			delNodep = tfind(&searchNode, &recTreeRoot, recCompare);
			if(delNodep && *delNodep){
				DEBUG("Delete tree node:%lu, withdraw addr: %lu, %lu, delNodep: %lu\n", 
					delNodep[0]->pageNum, *delNodep, delNodep[0], delNodep);
				//xzjin tdelete之后会修改delNodep指向的内容，所以要在tdelete之前保存
				struct recTreeNode *freep = *delNodep;
				tdelete(&searchNode, &recTreeRoot, recCompare);
				//free(freep);
				withdrawRecTreeNode(freep);
			}
			*/
		}else{
			notFoundLen = MIN(PAGESIZE, toTailLen);
			MSG("buf: %lu not found, size:%d.\n", start, notFoundLen);
			ts_write_not_found_size += notFoundLen;
		}
	}
//	MSG("ts_write buf:%p, tail:%p, length:%lu, same time:%d, same len:%d, same occupy:%d%%\n\n\n",
//		 buf, tail, length, sameContentTimes, curWriteSameLen, (int)(curWriteSameLen*100/length));
	END_TIMING(compare_mem_t, compare_mem_time);
#endif //USE_TS_FUNC 
	return ret;
}

size_t ts_fwrite (const void *buf, size_t size, size_t n, FILE *s){
	void *t = (void*)buf;
	unsigned long tail;
	unsigned long long start, end;
	struct recTreeNode searchNode;
	size_t length;
	struct memRec *mrp;
	void* diffPos = 0;
	void* bufCmpStart;
	unsigned long sameLen;
	int sameContentTimes = 0;
	int curWriteSameLen = 0;
	int toTailLen;
	int idx; 
	int file;

	length = size*n;
	tail = (unsigned long)buf+length;
	start = (unsigned long long)addr2PageNum(t);
	end = (unsigned long long)addr2PageNum((void*)((unsigned long long)buf+length));
	file = fileno(s);
	//TODO xzjin 这个是不是要在用户空间维护一下，太耗时了
	//off_t fpos = lseek(file, 0, SEEK_CUR);
	ts_write_size += length;
	START_TIMING(ts_write_t, ts_write_time);
	unsigned long ret = fwrite(buf, size, n, s);
	END_TIMING(ts_write_t, ts_write_time);
//	MSG("ts_fwrite: buf:%p, file:%d, len: %llu, fileName:%s\n", 
//		buf, file, length, fd2path[file%100]);
	//listRecTreeDetail();
#if USE_TS_FUNC 
	START_TIMING(compare_mem_t, compare_mem_time);
#if PATCH
	//xzjin 因为是比较连续的地址，所以不用在开始重新初始化diffPos变量
	for(int i=0; start<=end; start++,i++){
		int sameLenCurPage = 0;
		int notFoundLen;
		toTailLen = length-((unsigned long)getAddr((void*)start,0)-(unsigned long)buf);
//		DEBUG("compare, Page num:%lu.\n", start);
		if((unsigned long)getAddr((void*)start, 0)<(unsigned long)diffPos) continue;
		searchNode.pageNum = (void*)start;
//		START_TIMING(tfind_t, tfind_time);
		struct recTreeNode **res = tfind(&searchNode, &recTreeRoot, recCompare);
//		END_TIMING(tfind_t, tfind_time);
		if(res){
//			listRecTreeNode((void*)start);
			struct recBlockEntry *blockEntry = TAILQ_LAST(res[0]->listHead, tailhead);
			//xzjin 注意这里有个foreach，这个有效吗？正确吗?
			while(blockEntry){
				int i=0;
				mrp = blockEntry->recArr;
				if(res[0]->recModEntry == blockEntry ){
					idx = res[0]->memRecIdx;
				}else{
					idx = MEMRECPERENTRY;
				}					
				//xzjin 过滤掉比buf小的地址
				//xzjin 这里是不是不太对，对于patch这种一次读进来然后分开写的都过滤了
//				while (((unsigned long)getAddr((void*)start, mrp->pageOffset) < (unsigned long) buf) &&
//					i<idx ) {
//					mrp++; i++;
//				}
				
				for(; i<idx; mrp++,i++){
					DEBUG("i=%d, mrp:%lu.\n", i, mrp);
					bufCmpStart = getAddr((void*)start, mrp->pageOffset);
					if((unsigned long)bufCmpStart < (unsigned long) diffPos) continue;
					if((unsigned long)bufCmpStart > (unsigned long) tail) break;
					sameLen = cmpWrite((unsigned long)bufCmpStart, mrp, (void*)buf, tail, &diffPos, file);
					sameContentTimes += sameLen?1:0;
					sameLenCurPage += sameLen;
					curWriteSameLen +=sameLen;
				}
				blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
			}

//delTail:
//			samePercent = ((double)sameLenCurPage/writeLenCurPage)*100;
//			printf("same percentage this page: %.2f%%, pageLen:%d, samelen:%d, writePageNum:%lu, curPageNum:%lu.\n",
//				samePercent, writeLenCurPage, sameLenCurPage,
//				addr2PageNum(buf), start);
			//xzjin 写完一个页就删除对应的映射结构
			//xzjin TODO这里是不是可以等到写对比不同的时候再删除，不是写完就删除
			//对一个地址多次写入的情况会有好处
			/*
			while (!TAILQ_EMPTY(headp)) {           // List Deletion.
				rbp = TAILQ_FIRST(headp);
				withdrawMemRecArr(rbp->recArr);
				TAILQ_REMOVE(headp, TAILQ_FIRST(headp), entries);
				withdrawRecBlockEntry(rbp);
			}
			withdrawTailHead(headp);
			delNodep = tfind(&searchNode, &recTreeRoot, recCompare);
			if(delNodep && *delNodep){
				DEBUG("Delete tree node:%lu, withdraw addr: %lu, %lu, delNodep: %lu\n", 
					delNodep[0]->pageNum, *delNodep, delNodep[0], delNodep);
				//xzjin tdelete之后会修改delNodep指向的内容，所以要在tdelete之前保存
				struct recTreeNode *freep = *delNodep;
				tdelete(&searchNode, &recTreeRoot, recCompare);
				//free(freep);
				withdrawRecTreeNode(freep);
			}
			*/
		}else{
			notFoundLen = MIN(PAGESIZE, toTailLen);
			MSG("buf page: %X not found, not found size:%d.\n", start, notFoundLen);
			ts_write_not_found_size += notFoundLen;
		}
	}
#else
	//xzjin 因为是比较连续的地址，所以不用在开始重新初始化diffPos变量
	for(int i=0; start<=end; start++,i++){
//		int writeLenCurPage = 4096;
		int sameLenCurPage = 0;
		int notFoundLen;
		toTailLen = length-((unsigned long)getAddr((void*)start,0)-(unsigned long)buf);
//		float samePercent;
//		DEBUG("compare, Page num:%lu.\n", start);
		if((unsigned long)getAddr((void*)start, 0)<(unsigned long)diffPos) continue;
//		if(startPageNum == start){
//			writeLenCurPage = 4096-addr2PageOffset(buf);
//		}else if(end == start){
//			writeLenCurPage = addr2PageOffset((void*)tail);
//		}
		searchNode.pageNum = (void*)start;
//		START_TIMING(tfind_t, tfind_time);
		struct recTreeNode **res = tfind(&searchNode, &recTreeRoot, recCompare);
//		END_TIMING(tfind_t, tfind_time);
		if(res){
//			struct tailhead *headp = res[0]->listHead;
//			listRecTreeNode((void*)start);
			struct recBlockEntry *blockEntry = TAILQ_LAST(res[0]->listHead, tailhead);
			//xzjin 注意这里有个foreach，这个有效吗？正确吗?
			while(blockEntry){
				int i=0;
				mrp = blockEntry->recArr;
				if(res[0]->recModEntry == blockEntry ){
					idx = res[0]->memRecIdx;
				}else{
					idx = MEMRECPERENTRY;
				}					
				//xzjin 过滤掉比buf小的地址
				//xzjin 这里是不是不太对，对于patch这种一次读进来然后分开写的都过滤了
				while (((unsigned long)getAddr((void*)start, mrp->pageOffset) < (unsigned long) buf) &&
					i<idx ) {
					mrp++; i++;
				}
				
				for(; i<idx; mrp++,i++){
					DEBUG("i=%d, mrp:%lu.\n", i, mrp);
					bufCmpStart = getAddr((void*)start, mrp->pageOffset);
					if((unsigned long)bufCmpStart < (unsigned long) diffPos) continue;
					if((unsigned long)bufCmpStart > (unsigned long) tail) break;
					sameLen = cmpWrite((unsigned long)bufCmpStart, mrp, (void*)buf, tail, &diffPos, file);
					sameContentTimes += sameLen?1:0;
					sameLenCurPage += sameLen;
					curWriteSameLen +=sameLen;
				}
				blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
			}

//delTail:
//			samePercent = ((double)sameLenCurPage/writeLenCurPage)*100;
//			printf("same percentage this page: %.2f%%, pageLen:%d, samelen:%d, writePageNum:%lu, curPageNum:%lu.\n",
//				samePercent, writeLenCurPage, sameLenCurPage,
//				addr2PageNum(buf), start);
			//xzjin 写完一个页就删除对应的映射结构
			//xzjin TODO这里是不是可以等到写对比不同的时候再删除，不是写完就删除
			//对一个地址多次写入的情况会有好处
			/*
			while (!TAILQ_EMPTY(headp)) {           // List Deletion.
				rbp = TAILQ_FIRST(headp);
				withdrawMemRecArr(rbp->recArr);
				TAILQ_REMOVE(headp, TAILQ_FIRST(headp), entries);
				withdrawRecBlockEntry(rbp);
			}
			withdrawTailHead(headp);
			delNodep = tfind(&searchNode, &recTreeRoot, recCompare);
			if(delNodep && *delNodep){
				DEBUG("Delete tree node:%lu, withdraw addr: %lu, %lu, delNodep: %lu\n", 
					delNodep[0]->pageNum, *delNodep, delNodep[0], delNodep);
				//xzjin tdelete之后会修改delNodep指向的内容，所以要在tdelete之前保存
				struct recTreeNode *freep = *delNodep;
				tdelete(&searchNode, &recTreeRoot, recCompare);
				//free(freep);
				withdrawRecTreeNode(freep);
			}
			*/
		}else{
			notFoundLen = MIN(PAGESIZE, toTailLen);
			MSG("buf page: %X not found, not found size:%d.\n", start, notFoundLen);
			ts_write_not_found_size += notFoundLen;
		}
	}
#endif //NOT_PATCH
//	MSG("ts_write buf:%p, tail:%p, length:%lu, same time:%d, same len:%d, same occupy:%d%%\n\n\n",
//		 buf, tail, length, sameContentTimes, curWriteSameLen, (int)(curWriteSameLen*100/length));
	END_TIMING(compare_mem_t, compare_mem_time);
#endif //USE_TS_FUNC 
	return ret;
}

//xzjin 拷贝内存到新地址然后删除旧映射
void* ts_memcpy_traced(void *dest, void *src, size_t n){

	unsigned long long start = (unsigned long long)addr2PageNum(src);
	unsigned long long srcPageNum = start;
	unsigned long long destStart = (unsigned long long)addr2PageNum(dest);
	unsigned long long end = (unsigned long long)addr2PageNum((void*)((unsigned long long)src+n));
	struct recTreeNode searchNode, destSearchNode;

	void *ret;

	ret = memcpy(CALL_MEMCPY);

#if USE_TS_FUNC
//	DEBUG("ts_memcpy_traced: From: %lu, from tail: %lu, to： %lu, to tail: %lu, length: %lu.\n",
//		src, src+n, dest, dest+n, n);
	START_TIMING(mem_from_mem_trace_t, mem_from_mem_trace_time);
	//xzjin TODO,对不是从页开始的内容做memRec的拼接
	for(int i=0; start<=end; start++,destStart++,i++){
//		DEBUG("start:%lu.\n", start);
		searchNode.pageNum = (void*)start;
		destSearchNode.pageNum = (void*)destStart;
		struct recTreeNode **srcRes = tfind(&searchNode, &recTreeRoot, recCompare);
		struct recTreeNode **destRes = findAndAddRecTreeNode(&destSearchNode);
		//src address is in tree and dest address is not
		if(srcRes){
			struct recBlockEntry *srcLastEntry = srcRes[0]->recModEntry;
			struct recBlockEntry *blockEntry = TAILQ_LAST(srcRes[0]->listHead, tailhead);
//			struct recBlockEntry *delBlockEntry;
			struct memRec* rec;
			int idx = 0, uplimit = MEMRECPERENTRY;
			//xzjin 这里应该是略过拷贝src同页面里比src小的记录
			if(blockEntry == srcLastEntry){
				uplimit = srcRes[0]->memRecIdx;
			}
			//int destOffset = addr2PageOffset(dest);
			if(start == srcPageNum){
				int destOffset = addr2PageOffset(src);
				//xzjin 先找到recArr
				do{
				//	if(srcRes[0]->recModEntry == srcLastEntry){
				//		uplimit = srcRes[0]->memRecIdx;
				//	}
					rec = blockEntry->recArr;
					if(rec[uplimit-1].pageOffset>= destOffset){
						break;
					}
					blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
				}while(blockEntry);
				if(!blockEntry){
//					MSG("No offset bigger than copy offset, continue.\n");
					continue;
				}

				//xzjin 再从recArr里面找具体的条目
				for(idx=0; idx < uplimit; idx++){
					if(rec[idx].pageOffset >= destOffset){
						srcRes[0]->memRecIdx = idx;
						srcRes[0]->recModEntry = blockEntry;
						break;
					}
				}
			}

//			int destOffset = addr2PageOffset(dest);
//			do{
//				if(srcRes[0]->recModEntry == srcLastEntry){
//					uplimit = srcRes[0]->memRecIdx;
//				}
//				rec = blockEntry->recArr;
//				if(rec[uplimit-1].pageOffset>= destOffset){
//					break;
//				}
//				blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
//			}while(blockEntry);
//copyRec:
//			for(idx=0; idx < uplimit; idx++){
//				if(rec[idx].pageOffset >= destOffset){
//					srcRes[0]->memRecIdx = idx;
//					srcRes[0]->recModEntry = blockEntry;
//					break;
//				}
//			}
//			MSG("blockEntry:%lu.\n", blockEntry);
			//Copy record item to dest
			for(; idx < uplimit; idx++){
//				void *fileCmpStart;
//				size_t cmpLen = 999;
				struct memRec *curMemRec = rec+idx;
//				struct fileMapTreeNode **fmSearNodep;
//				struct fileMapTreeNode *fmTarNode = NULL;
				void* copySrc = getAddr((void*)start, curMemRec->pageOffset);
				void* copyDest;
//				struct memRec *modifyTarget; 
 				copyDest = dest+(copySrc-src);
				//xzjin 确保拷贝到了正确的页结构里面
				if(UNLIKELY((unsigned long long)addr2PageNum(copyDest) != destStart)){
//					DEBUG("searchNode:%lu, pageNum:%lu.\n", &destSearchNode, destSearchNode.pageNum);
					destStart = (unsigned long long)addr2PageNum(copyDest);
					destSearchNode.pageNum = (void*)destStart;
					destRes = findAndAddRecTreeNode(&destSearchNode);
				}
				//xzjin update lastRec, it's one of paramters that writeRec needs，
				//so update lastRec before call writeRec
				lastRec.lastPageNum = (void*)destStart;
				lastRec.lastMemRec = destRes[0]->recModEntry->recArr;
				//xzjin 注意这里是指向数字的指针
				lastRec.lastIdx = &(destRes[0]->memRecIdx);

				//Test Compare whether content are the same
#ifdef  TS_MEMCPY_CMPWRITE
				{
					void *diffPos;
				 	void *srcCmpStart = curMemRec->pageOffset+(start<<PAGENUMSHIFT);
					struct fileMapTreeNode fmNode;
					modifyTarget = lastRec.lastMemRec + (*lastRec.lastIdx);
					//struct fMap *fmp = &fMapCache[curMemRec->fd%FMAPCACHESIZE];
					fmNode.start = srcCmpStart;
					fmNode.tail = (unsigned long)(srcCmpStart)+1;
					MSG("Search start:%lu, search tail:%lu.\n", fmNode.start, fmNode.tail);
					fmSearNodep = tfind(&fmNode, &fileMapTreeRoot, fileMapTreeSearchCompare);
					//xzjin 检查文件描述符和文件偏移对不对，这个是不是应该放到调用函数里面检查
					//assert(fmSearNodep);
					if(fmSearNodep){
						fmTarNode = *fmSearNodep;
						fileCmpStart = (unsigned long)(fmTarNode->start+curMemRec->fileOffset-fmTarNode->offset);
						MSG("src cmp start:%lu, file cmp start:%lu.\n", srcCmpStart, fileCmpStart);
						START_TIMING(memcmp_asm_t, memcmp_asm_time);
						int cmpRet = memcmp_avx2_asm((void*)srcCmpStart, (void*)fileCmpStart, cmpLen, &diffPos);
						END_TIMING(memcmp_asm_t, memcmp_asm_time);
						if(cmpRet){		//different
							int sameLen = diffPos - srcCmpStart;
							MSG("Copy item: same len:%lu, buf:%lu, file buf:%lu, diffPos:%lu, cmpRet:%d, fd:%d, fileName: %s, pageNum:%lu, pageOffset:%lu\n",
								sameLen, srcCmpStart, fileCmpStart, diffPos, cmpRet, fd, fmTarNode->fileName, start, curMemRec->pageOffset);
						}else{		//same
							MSG("Copy item: same len:%lu, buf:%lu, file buf:%lu, diffPos:%lu, cmpRet:%d, fd:%d, fileName: %s, pageNum:%lu, pageOffset:%lu\n",
								cmpLen, srcCmpStart, fileCmpStart, diffPos, cmpRet, fd, fmTarNode->fileName, start, curMemRec->pageOffset);
						}
					}
				}
#endif	//TS_MEMCPY_CMPWRITE
				//insertRec(unsigned long fileOffset, void* src, void* dest, char* fileName);
//				DEBUG("ts_memcpy_traced UP idx:%d, uplimit:%d\n", idx, uplimit);
				insertRec(curMemRec->fileOffset, copySrc, copyDest, curMemRec->fileName);
//				MSG("copy src: %lu, copy dest: %lu\n", copySrc, copyDest);
#ifdef TS_MEMCPY_CMPWRITE
				{
					if(fmSearNodep){
						curMemRec = modifyTarget;
						void *diffPos;
					 	void *srcCmpStart = curMemRec->pageOffset+(destStart<<PAGENUMSHIFT);
						START_TIMING(memcmp_asm_t, memcmp_asm_time);
						int cmpRet = memcmp_avx2_asm((void*)srcCmpStart, (void*)fileCmpStart, cmpLen, &diffPos);
						END_TIMING(memcmp_asm_t, memcmp_asm_time);
						if(cmpRet){		//different
							int sameLen = diffPos - srcCmpStart;
							MSG("After copy item: same len:%lu, buf:%lu, file buf:%lu, diffPos:%lu, cmpRet:%d, fd:%d, fileName: %s, pageNum:%lu, pageOffset:%lu\n",
								sameLen, srcCmpStart, fileCmpStart, diffPos, cmpRet, fd, fmTarNode->fileName, destStart,curMemRec->pageOffset);
						}else{		//same
							MSG("After copy item: same len:%lu, buf:%lu, file buf:%lu, diffPos:%lu, cmpRet:%d, fd:%d, fileName: %s, pageNum:%lu, pageOffset:%lu\n",
								cmpLen, srcCmpStart, fileCmpStart, diffPos, cmpRet, fd, fmTarNode->fileName, destStart,curMemRec->pageOffset);
						}
					}
				}
#endif	//TS_MEMCPY_CMPWRITE
			}

			blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
			//xzjin TODO 这里是不是可以和上面合并,函数太长了
			while(blockEntry){
				if(blockEntry == srcLastEntry){
					uplimit = srcRes[0]->memRecIdx;
				}
//				if(srcRes[0]->recModEntry == srcLastEntry){
//					uplimit = srcRes[0]->memRecIdx;
//				}
				rec = blockEntry->recArr;
				for(idx = 0; idx < uplimit; idx++){
					struct memRec *curMemRec = rec+idx;
					void* copySrc = getAddr((void*)start, curMemRec->pageOffset);
					void* copyDest = dest+(copySrc-src);
					//xzjin 确保拷贝到了正确的页结构里面
					if(UNLIKELY((unsigned long long)addr2PageNum(copyDest) != destStart)){
						destStart = (unsigned long long)addr2PageNum(copyDest);
						destSearchNode.pageNum = (void*)destStart;
						MSG("searchNode:%lu, pageNum:%lu, copyDest: %lu, dest: %lu, copySrc:%lu, src:%lu.\n", 
							&destSearchNode, destSearchNode.pageNum, copyDest, 
							dest, copySrc, src);
						destRes = findAndAddRecTreeNode(&destSearchNode);
					}
					//xzjin update lastRec,it's one of the paremeters that writeRec needs，
					// so update lastRec before call writeRec
					lastRec.lastPageNum = (void*)destStart;
					lastRec.lastMemRec = destRes[0]->recModEntry->recArr;
					//xzjin 注意这里是指向数字的指针
					lastRec.lastIdx = &(destRes[0]->memRecIdx);
//					writeRec(curMemRec->fileOffset, copySrc, copyDest,
//						 (void*)destStart, curMemRec->fileName);
//					DEBUG("ts_memcpy_traced DOWN idx:%d, uplimit:%d\n", idx, uplimit);
					insertRec(curMemRec->fileOffset, copySrc, copyDest, curMemRec->fileName);
				}
				//xzjin TODO 这里是不是也是不用删除，直接等写覆盖更好
				//delBlockEntry = blockEntry;
				blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
				/*
				withdrawMemRecArr(delBlockEntry->recArr);
				TAILQ_REMOVE(srcRes[0]->listHead, delBlockEntry, entries);
				withdrawRecBlockEntry(delBlockEntry);
				*/
			}

//returnPoint:
//			if(TAILQ_EMPTY(destRes[0]->listHead)){
//				MSG("List empty\n");
//			}else{
//				MSG("List NOT empty\n");
//			}
		}
	}
	END_TIMING(mem_from_mem_trace_t, mem_from_mem_trace_time);
#endif	//USE_TS_FUNC
	
	return ret;
}

//xzjin 普通的memcpy但是会做地址跟踪,这个是针对从mmap区域的copy
//ts_memcpy_traced是针对拷贝traced（从mmap或traced区域拷贝过一次的）的内容
void* ts_memcpy(void *dest, void *src, size_t n){
//	DEBUG("Memcpy start,dest:%p, src:%p, len:%llu\n",
//				 dest, src, n);
#if REC_INSERT_TEST 
	struct recTreeNode node,**pt;
#endif
	void *ret;
	void* destTail __attribute__ ((__unused__));
	void* startPageNum __attribute__ ((__unused__));
	void* endPageNum __attribute__ ((__unused__));
	unsigned long fileOffset;
	struct fileMapTreeNode fileMapTreeNode;
	struct fileMapTreeNode **fileMapSearRes;
	struct fileMapTreeNode *fmNode;
//	MSG("ts_memcpy\n");

	ret = memcpy(CALL_MEMCPY);
	//xzjin 如果copy的内容是跨文件的（好像不太可能）这里只记录前半段
#if USE_TS_FUNC
 
	START_TIMING(mem_from_file_trace_t, mem_from_file_trace_time);

	fileMapTreeNode.start = src;
	fileMapTreeNode.tail = src + n;
	fmNode = lastTs_memcpyFmNode;
	//xzjin TODO 整个函数的大约75%开销来自这里
	START_TIMING(ts_memcpy_tfind_file_t, ts_memcpy_tfind_file_time);
	//xzjin 当前buf不在上次查询的文件的映射里面
//	if(!(LIKELY(lastTs_memcpyFmNode) && (lastTs_memcpyFmNode->start<=(unsigned long)src &&
//		 (unsigned long)src<=lastTs_memcpyFmNode->tail)))
	//xzjin TODO 这里添加一个最近使用文件的缓存
#if USE_FMAP_CACHE
	struct fileMapTreeNode* fmtp;
	for(int i=0; i< FILEMAPCACHESIZE; i++){
		fmtp = fileMapCache[i];
		if(LIKELY(fmtp) && fmtp->start<=(unsigned long)src && (unsigned long)src<=fmtp->tail){
			fmNode = fmtp;
			goto insertRecPoint;
		}
		if(LIKELY(fmtp) && leastRecFCache.leastUsedTime >fmtp->usedTime){
			leastRecFCache.leastUsedTime = fmtp->usedTime;
			leastRecFCache.idx = i;
		}
	}
	fileMapSearRes = tfind(&fileMapTreeNode, &fileMapTreeRoot, fileMapTreeSearchCompare);
	if(fileMapSearRes){
		fmNode = fileMapSearRes[0];
		//xzjin insert into fileMapCache
		fmNode->usedTime = 0;
		fileMapCache[leastRecFCache.idx] = fmNode;
		leastRecFCache.leastUsedTime = INT32_MAX;
		//MSG("File Name:%s.\n", fmNode->fileName);
	}else{
		goto ts_memcpy_returnPoint;
	}
#else
	fileMapSearRes = tfind(&fileMapTreeNode, &fileMapTreeRoot, fileMapTreeSearchCompare);
	if(fileMapSearRes){
		fmNode = fileMapSearRes[0];
		//MSG("File Name:%s.\n", fmNode->fileName);
	}else{
		goto ts_memcpy_returnPoint;
	}
#endif	// USE_FMAP_CACHE
	END_TIMING(ts_memcpy_tfind_file_t, ts_memcpy_tfind_file_time);
	fmNode->usedTime++;
//	MSG("copy src found, copyFrom:%lu, copyTo:%lu, map start:%lu, map tail:%lu, map offset:%ld, fileName: %s\n",
//		src, dest,fmNode->start, fmNode->tail, fmNode->offset, fmNode->fileName);
	//xzjin 记录log信息
	fileOffset = fmNode->offset+(src-fmNode->start);
	START_TIMING(insert_rec_t,  insert_rec_time);
#if PATCH
	//xzjin 对于patch, 每页加一个记录
	destTail = dest+n;
	startPageNum = addr2PageNum(dest);
	endPageNum = addr2PageNum(destTail);
	do{
		unsigned long diff;
		insertRec(fileOffset, src, dest, fmNode->fileName);
		startPageNum++;
		diff = ((unsigned long)startPageNum<<PAGENUMSHIFT)-(unsigned long)dest;
		fileOffset += diff;
		src += diff;
		dest += diff;
	}while(startPageNum<=endPageNum);
#else
	insertRec(fileOffset, src, dest, fmNode->fileName);
#endif //PATCH
	END_TIMING(insert_rec_t,  insert_rec_time);
	
#if REC_INSERT_TEST 
	node.pageNum = addr2PageNum(dest);
	pt = tfind(&node, &recTreeRoot, recCompare);
	if(pt && pt[0]->memRecIdx){	//xzjin 在树里找到了
		void *diffPos;
		void *fileCmpStart;
		int cmpRet;
		struct memRec *mrp = pt[0]->recModEntry->recArr;
		mrp += pt[0]->memRe:Idx;
		mrp-- ;

		fileCmpStart = fmNode->start+mrp->fileOffset-fmNode->offset;
		cmpRet = memcmp_avx2_asm(fileCmpStart, dest, n, &diffPos);
		if(cmpRet){
			MSG("Compare diff, fileCmpStart:%lu, diffPos:%lu, sameLen:%lu, copyFrom:%lu, copyTo:%lu\n",
				fileCmpStart, diffPos, (diffPos-fileCmpStart), src, dest);
			MSG("file in mem:\n");
			printMem(fileCmpStart, 2);
			MSG("dest mem:\n");
			printMem(dest, 2);
			exit(-1);
		}else{
			MSG("Compare correct, sameLen:%lu\n", n);
		}
	}else{
		ERROR("REC_INSERT test fail, tfind NULL!\n");
		exit(-1);
	}
#endif	//REC_INSERT_TEST 
//		MSG("copy src:           %lu not fount in file list.\n", src);
ts_memcpy_returnPoint:
	END_TIMING(mem_from_file_trace_t, mem_from_file_trace_time);
#endif	//USE_TS_FUNC
	return ret;
}

//xzjin read content to buf use ts_memcpy and set offset to correct pos
ssize_t ts_read(int fd, void *buf, size_t nbytes){

	struct fileMapTreeNode **targetNodep;
	struct fileMapTreeNode *node;
	struct fileMapTreeNode *treeNode;
	void* copyBegin;
	size_t copyLen;
	off_t seekRet;
	int err;

	treeNode = allocateFileMapTreeNode();
	//treeNode->fileName = fd2path[fd%100];
	treeNode->fileName = GETPATH(fd);
	targetNodep = tfind(treeNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
	withdrawFileMapTreeNode(treeNode);
	if(UNLIKELY(!targetNodep)){
		MSG("find mmap ERROR, file not mapped.\n");
		exit(-1);
	}
//	MSG("read from file:%s\n", fd2path[fd%100]);
	node = targetNodep[0];
	//xzjin 直接用lseek是不是太耗时了
	seekRet = lseek(fd, 0, SEEK_CUR);
	if(seekRet == -1){
		err = errno;
		MSG("lseek ERROR, %s, errno %d.\n", strerror(err), err);
		exit(-1);
	}
	copyBegin = (void *)(node->start+(seekRet - node->offset));
	copyLen = (size_t)node->tail-(size_t)copyBegin;
	copyLen = MIN(copyLen, nbytes);
	ts_memcpy(buf, copyBegin, copyLen);

	seekRet = lseek(fd, copyLen, SEEK_CUR);
	err = errno;
	if(seekRet == -1){
		MSG("seek to %d.\n", seekRet+copyLen);
		MSG("lseek ERROR, %s, errno %d.\n", strerror(err), err);
		exit(-1);
	}
	MSG("ts_read to %p ,len %d ,from %p, fd:%d, fileName:%s.\n",
		buf, copyLen, copyBegin, fd, GETPATH(fd));
	return copyLen;
}

void* ts_realloc(void *ptr, size_t size, void* tail){
	void *ret;
	unsigned long long start = (unsigned long long)addr2PageNum(ptr);
	unsigned long long end = (unsigned long long)addr2PageNum(tail);
	struct recTreeNode searchNode;

	ret = realloc(ptr, size);

#if USE_TS_FUNC
	START_TIMING(remap_mem_rec_t, remap_mem_rec_time);
	for(int i=0; start<=end; start++,i++){
		searchNode.pageNum = (void*)start;
		struct recTreeNode **res = tfind(&searchNode, &recTreeRoot, recCompare);
		if(res){
			struct tailhead* head = res[0]->listHead;
			struct recBlockEntry *mostRecBlock = res[0]->recModEntry;
			struct recBlockEntry *blockEntry = TAILQ_LAST(head, tailhead);
			struct recBlockEntry *delBlockEntry __attribute__((unused));
//			struct recBlockEntry *delBlockEntry;
			struct memRec* rec;
			int uplimit;

			do{	//loop for every block/recArray
				delBlockEntry = blockEntry;
				int overlaped __attribute__((unused));
				overlaped = 0;
				if(blockEntry == mostRecBlock){
					uplimit = res[0]->memRecIdx;
				}else{
					uplimit = MEMRECPERENTRY;
				}
				rec = blockEntry->recArr;
/**	
 * Realloc area				|________________|
 * rec area
 * case.1		|_______|
 * case.2		|________________|
 * case.3		|_________________________________________|
 * case.4						|______|
 * case.5						|_________________________|
 * case.6											|_____|
 */
				//Precheck, case6, because of the ordered queue, later recorder
				//must be larger, continue
				if(getAddr((void*)start, rec[uplimit].pageOffset)>=tail){
					continue;
				}
				for(int i=0; i< uplimit; i++){	//loop for every single record
					void* dest;
					void* memAddr = getAddr((void*)start, rec[i].pageOffset);
					//case 1,2,3 ommit, for case 1,2,3 we do not kown the len,
					//so do not now which case it belogs to, omit.
					if(memAddr <= ptr){
						continue;
					}
					//case6, because of the ordered queue, later recorder
					//must be larger, continue
					if(memAddr >= tail){
						break;
					}
					overlaped = 1;
					//Add record
					dest = (void*)((unsigned long)memAddr-(unsigned long)ptr+ret);
					DEBUG("in ts_realloc.\n");
					insertRec(rec[i].fileOffset, NULL, dest, rec[i].fileName);
				}
				blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
				//xzjin after find relocate area, del all the rec in current array.
				//it is a compromise bwtween complexity and effectiveness
#if DEL_REALLOC_ADDR
				if(overlaped){
					withdrawMemRecArr(delBlockEntry->recArr);
					TAILQ_REMOVE(head, delBlockEntry, entries);
					withdrawRecBlockEntry(delBlockEntry);
				}
#endif	//DEL_REALLOC_ADDR
			}while(blockEntry);
		}
	}
	END_TIMING(remap_mem_rec_t, remap_mem_rec_time);
#endif 	//USE_TS_FUNC
	return ret;
}

//xzjin 释放内存，同时释放内存跟踪的内容
void ts_free(void* ptr, void* tail){
//	unsigned long long start = addr2PageNum(ptr);
//	unsigned long long end = addr2PageNum(tail);
//	struct recTreeNode searchNode;
//	for(;start<=end; start++){
//		searchNode.pageNum = start;
//		struct recTreeNode **res = tfind(&searchNode, &recTreeRoot, recCompare);
//		if(res){
//			struct slisthead *headp = res[0]->listHead;
//		    while (!SLIST_EMPTY(headp)) {           /* List Deletion. */
//				struct recBlockEntry *rbe = SLIST_FIRST(headp);
//				if(rbe->recArr<(unsigned long long)RECARRPOOL || rbe->recArr>RECARRPOOLTAIL){
//					ERROR("地址错误 rbe:%p pool:%p poolTail:%x\n", 
//							rbe, RECARRPOOL, RECARRPOOLTAIL);
//				}
//				withdrawMemRecArr(rbe->recArr);
//		        withdrawRecBlockEntry(rbe);
//		        SLIST_REMOVE_HEAD(headp, entries);
//		    }
//			withdrawSlistHead(headp);
//			/** xzjin TODO 这里需要注意怎么确保他不回收地址,这用free是在
//			 * malloc申请的地址中间free，会不会有很大的问题，暂时把rbTree
//			 * 的node改成单独malloc的 */
//			tdelete(&searchNode, &recTreeRoot, recCompare);
//		}
//	}

//	if(LIKELY(libcFree)){
//		libcFree(ptr);
//		return;
//	}else{
//		if(! _libc_so_p){
//			_libc_so_p = dlopen(LIBC_SO_LOC, RTLD_LAZY|RTLD_LOCAL);
//		}
//		if(! _libc_so_p){
//			ERROR("Failed to open libc.\n");
//			assert(0);
//		}
//		libcFree= dlsym(_libc_so_p, "free" );
//		if(!libcFree){
//			ERROR("Failed to find libc free.\n");
//			assert(0);
//		}
//		libcFree(ptr);
//	}
	free(ptr);
	return;
}

__attribute__((destructor))void fini(void) {
	print_statistics();
	PRINT_TIME();
}

int ts_open(const char *path, int oflag, ...){
	char *abpath = NULL;
    struct stat st;
	int result, err;
	mode_t mode;
	long mmapRet __attribute__((unused));
    va_list ap;

    va_start(ap, oflag);
    if (oflag & O_CREAT)
        mode = va_arg(ap, unsigned int);
    va_end(ap);
	result = open(path, oflag, mode);
	err = errno;
	if(UNLIKELY(result == -1)){
		ERROR("file open error, %s, errno:%d.", strerror(err), err);
		return result;
	}
#if PATCH
	//xzjin If creat file, do not mmap
    if (oflag & O_CREAT) return result;
#endif //PATCH

	abpath = realpath(path,NULL);
	if(abpath){
		STOREPATH(result, abpath);
//		MSG("Open %s ,Return = %d\n", fd2path[result], result);
	}else {
		ERROR("Open %s , get realpath failed.\n", path, result);
	}
	if(fstat(result, &st)){
	    err = errno;
	    ERROR("fstat error, %s, errno:%d.\n", strerror(err), err);
		exit(-1);
	}
#if PATCH
	mmapRet = (long)ts_mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, result, 0);
	err = errno;
	if(mmapRet == -1){
	    ERROR("ts_mmap error, %s, errno:%d.\n", strerror(err), err);
		exit(-1);
	}
#endif //PATCH
//	MSG("file: %s, fd:%d\n",path, result);
	return result;
}

int ts_openat (char* dirName, int dirfd, const char *path, int oflag, ...){
	char* abpath = NULL;
	char fullpath[PATH_MAX] = "\0";
    struct stat st;
	int result, err;
	mode_t mode;
	long mmapRet __attribute__((unused));
    va_list ap;

    va_start(ap, oflag);
    if (oflag & O_CREAT)
        mode = va_arg(ap, unsigned int);
    va_end(ap);
	result = openat(dirfd, path, oflag, mode);
	err = errno;
	if(UNLIKELY(result == -1)){
		ERROR("file open error, %s, errno:%d.", strerror(err), err);
		return result;
	}
#if PATCH
	//xzjin If creat file, do not mmap
    if (oflag & O_CREAT) return result;
#endif //PATCH

//	strcat(fullpath, dirName);
//	strcat(fullpath, "/");
//	strcat(fullpath, path);
//	abpath = realpath(fullpath, NULL);
	abpath = (char*)path;
	err = errno;
	if(abpath){
		STOREPATH(result, abpath);
//		MSG("Open %s ,Return = %d\n", fd2path[result], result);
	}else {
		ERROR("Open %s , get realpath failed, path:%s, error, %s, errno:%d.\n",
			 path, result, fullpath, strerror(err), err);
	}
	if(fstat(result, &st)){
	    err = errno;
	    ERROR("fstat error, %s, errno:%d.\n", strerror(err), err);
		exit(-1);
	}
#if PATCH
	mmapRet = (long)ts_mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, result, 0);
	err = errno;
	if(mmapRet == -1){
	    ERROR("ts_mmap error, %s, errno:%d.\n", strerror(err), err);
		exit(-1);
	}
#endif //PATCH
	MSG("file: %s, fd:%d\n",path, result);
	return result;
}

FILE *ts_fopen (const char *filename, const char *modes){
	char *abpath = NULL;
    struct stat st;
	FILE* result;
	int err, fd;
	long mmapRet __attribute__((unused));

	result = fopen(filename, modes);
	err = errno;
	if(UNLIKELY(!result)){
		ERROR("file open error, %s, errno:%d.", strerror(err), err);
		return result;
	}

	abpath = realpath(filename, NULL);
	if(abpath){
		fd = fileno(result);
//		MSG("Open %s ,Return = %d\n", fd2path[result], result);
	}else {
		ERROR("Open %s , get realpath failed.\n", filename, result);
	}
	if(fstat(fd, &st)){
	    err = errno;
	    ERROR("fstat error, %s, errno:%d.\n", strerror(err), err);
		exit(-1);
	}
	if(st.st_size<=0) return result;

//	fd2path[fd%100] = abpath;
	STOREPATH(fd, abpath);
#if PATCH
	mmapRet = (long)ts_mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	err = errno;
	if(mmapRet == -1){
	    ERROR("ts_mmap error, %s, errno:%d.\n", strerror(err), err);
		exit(-1);
	}
#endif //PATCH
	MSG("file: %s, fd:%d\n", filename, result);
	return result;
}

int ts_close (int fd){
	char *abpath __attribute__((unused));
    struct stat st __attribute__((unused));
	struct fileMapTreeNode treeNode;
	struct fileMapTreeNode** delNodep;
	int result, err;
	mode_t mode __attribute__((unused));
	long mmapRet __attribute__((unused));

	abpath = NULL;
	result = close(fd);
	err = errno;
	if(UNLIKELY(result == -1)){
		ERROR("file close error, %s, errno:%d.", strerror(err), err);
		return result;
	}

	//abpath = fd2path[fd%100];
	abpath = GETPATH(fd);
	if(!abpath){
		ERROR("close %d , get realpath failed.\n",fd);
		return result;
	}else{
		MSG("Close file:%s\n", abpath);
	}

	treeNode.fileName = abpath;
	//TODO 这里需要的到被删除的节点，释放节点和包含文件名的字符串
	delNodep = tfind(&treeNode, &fileMapTreeRoot, fileMapNameTreeInsDelCompare);
	if(delNodep && delNodep[0]){
		struct fileMapTreeNode *freep = *delNodep;
		//xzjin 同样，tdelete会改变delNodep指向位置的内容
		tdelete(&treeNode, &fileMapTreeRoot, fileMapNameTreeInsDelCompare);
		//这里用的比较算法还是fileMapTree的插入/删除算法
		tdelete(&treeNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
		MSG("deleted node is: %p, fileName:%s, name addr:%p.\n",
			 freep, freep->fileName, freep->fileName);
		//free(freep->fileName);
		//free(freep);
		withdrawFileMapTreeNode(freep);
	}else{
		MSG("Nothing deleted.\n");
	}

	return result;
}