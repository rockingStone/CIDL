#include "addr.h"
#include "awn.h"
#include "common.h"
#include "mem.h"
#include <sys/mman.h>
#include "tree.h"

#define PATCH 0
#define DEL_REALLOC_ADDR 0
#define USE_TS_FUNC 1
#define USE_FMAP_CACHE 0
#define REC_INSERT_TEST 0
//#define DELETE_TREENODE 0
//#define BYTE_COMPARE 1
#define CMPWRITE
#undef TS_MEMCPY_CMPWRITE

static int ts_writeTime = 0;

//Debug help func
void init(void);
void listRecTreeNode(void *pageNum);
void listFileMapTree();
void listRecTree();
void listRecTreeDetail();

//exported function
int ts_open(const char *path, int oflag, ...);
int CIDL_open(const char *path, int oflag, ...) __attribute__ ((weak, alias ("ts_open")));
int ts_openat(char* dirName, int dirfd, const char *path, int oflag, ...);
int CIDL_openat(char* dirName, int dirfd, const char *path, int oflag, ...) __attribute__ ((weak, alias ("ts_openat")));
FILE *ts_fopen(const char *filename, const char *modes);
FILE *CIDL_fopen(const char *filename, const char *modes) __attribute__ ((weak, alias ("ts_fopen")));
int ts_close (int fd);
int CIDL_close(int fd) __attribute__ ((weak, alias ("ts_close")));
ssize_t ts_write(int file, void *buf, size_t length);
ssize_t CIDL_write(int file, void *buf, size_t length) __attribute__ ((weak, alias ("ts_write")));
size_t ts_fwrite (const void *buf, size_t size, size_t n, FILE *s);
size_t CIDL_fwrite(const void *buf, size_t size, size_t n, FILE *s) __attribute__ ((weak, alias ("ts_fwrite")));
ssize_t ts_pwrite(int file, void *buf, size_t length, off64_t offset);
ssize_t CIDL_pwrite(int file, void *buf, size_t length, off64_t offset) __attribute__ ((weak, alias ("ts_pwrite")));
void* ts_memcpy_traced(void *dest, void *src, size_t n);
void* CIDL_memcpy_traced(void *dest, void *src, size_t n) __attribute__ ((weak, alias ("ts_memcpy_traced")));
void* ts_memcpy(void *dest, void *src, size_t n);
void* CIDL_memcpy(void *dest, void *src, size_t n) __attribute__ ((weak, alias ("ts_memcpy")));
RETT_MMAP ts_mmap(INTF_MMAP);
RETT_MMAP CIDL_mmap(INTF_MMAP) __attribute__ ((weak, alias ("ts_mmap")));
RETT_MUNMAP ts_munmap(INTF_MUNMAP);
RETT_MUNMAP CIDL_munmap(INTF_MUNMAP) __attribute__ ((weak, alias ("ts_munmap")));
ssize_t ts_read(int fd, void *buf, size_t nbytes);
ssize_t CIDL_read(int fd, void *buf, size_t nbytes) __attribute__ ((weak, alias ("ts_read")));
void* ts_realloc(void *ptr, size_t size, void* tail);
void* CIDL_realloc(void *ptr, size_t size, void* tail) __attribute__ ((weak, alias ("ts_realloc")));
void ts_free(void* ptr, void* tail);
void CIDL_free(void* ptr, void* tail) __attribute__ ((weak, alias ("ts_free")));


instrumentation_type compare_mem_time, remap_mem_rec_time, ts_write_time;
instrumentation_type mem_from_file_trace_time, mem_from_mem_trace_time;
instrumentation_type memcmp_asm_time, tfind_time, cmp_write_time;
instrumentation_type fileMap_time, fileUnmap_time, insert_rec_time;
instrumentation_type ts_memcpy_tfind_file_time;

pthread_mutex_t recTreeMutex;

unsigned long long totalAllocSize = 0;
#if defined(BASE_VERSION) || BYTE_COMPARE 
int memcmp_avx2_asm(const void *s1, const void *s2, size_t n, void** firstDiffPos);
int memcmp_avx2_asm(const void *s1, const void *s2, size_t n, void** firstDiffPos){
	char *c1 = (char*) s1;
	char *c2 = (char*) s2;
	for(int i=0; i<n; i++){
		if(*c1-*c2 != 0){
			*firstDiffPos = c1;
			return (*c1-*c2);
		}
		c1++;
		c2++;
	}
	return 0;
}
#else
extern int memcmp_avx2_asm(const void *s1, const void *s2, size_t n, void** firstDiffPos);
#endif //BASE_VERSION

#ifndef BASE_VERSION
inline void writeRec(unsigned long fileOffset, void* src, void* dest,
	 void* pageNum, char* fileName, short fd) __attribute__((always_inline));
inline void writeRec(unsigned long fileOffset, void* src, void* dest,
	 void* pageNum, char* fileName, short fd){
	int idx = *(lastRec.lastIdx);
	struct memRec* pt = lastRec.lastMemRec;
	unsigned long pageOffset = (unsigned long)dest-((unsigned long)pageNum<<PAGENUMSHIFT);
	//xzjin ??????????????????????????????lastRec??????????????????????????????????????????
	if(idx<MEMRECPERENTRY){		//??????????????????
		pt[idx].fileName= fileName;
		pt[idx].fileOffset = fileOffset;
		//?????????ts_memcpy_traced???????????????pageOffset>page size???????????????????????????????????????
		pt[idx].pageOffset = pageOffset;
		pt[idx].fd=fd;
		//xzjin update lastRec
//			lastRec.lastPageNum = ***;
//			lastRec.lastMemRec = ***;
		*(lastRec.lastIdx) = idx+1;
//		checkEmptyRecTreeNode();
	}else{	//???????????????????????????list
//		DEBUG("Append list node\n");
		struct recTreeNode tmpRecTreeNode,**rec;
		//xzjin ?????????????????????memRec
		struct memRec *recArr;
		//xzjin ??????????????????????????????????????????????????????memRec?????????
		struct recBlockEntry *rbe;
		tmpRecTreeNode.pageNum = pageNum;
		rec = tfind(&tmpRecTreeNode, &recTreeRoot,recCompare);
		assert(rec!=NULL);
		recArr = allocateMemRecArr();
		rbe = allocateRecBlockEntry();
		rbe->recArr = recArr;
		//xzjin ?????????????????????????????????????????????????????????????????????????????????????????????
		//xzjin ?????????????????????????????????????????????
		TAILQ_INSERT_HEAD(rec[0]->listHead, rbe, entries);
		rec[0]->recModEntry = rbe;
		
		//xzjin update Rec
		recArr[0].fileName= fileName;
		recArr[0].fileOffset = fileOffset;
		//?????????ts_memcpy_traced???????????????pageOffset>page size???????????????????????????????????????
		recArr[0].pageOffset = pageOffset;
		recArr[0].fd = fd;

		//xzjin update lastRec
		lastRec.lastMemRec = recArr;
		*(lastRec.lastIdx) = 1;
//		checkEmptyRecTreeNode();
	}
	//MSG("Insert into page: %p, idx:%d, offset:%lu\n", lastRec.lastPageNum, *(lastRec.lastIdx), pageOffset);
}

//xzjin src is UNUSED and UNCHECKED
inline void insertRec(unsigned long fileOffset, void* src, void* dest,
	 char* fileName, short fd) __attribute__((always_inline));
inline void insertRec(unsigned long fileOffset, void* src, void* dest,
	 char* fileName, short fd){
	void* pageNum = addr2PageNum(dest);

	//DEBUG("insterRec.\n");
	//xzjin ????????????????????????????????????????????????????????????????????????
	//xzjin ?????????????????????????????????????????????????????????????????????
	//xzjin ??????????????????????????????????????????????????????????????????????????????????????????????????????
//	if(pageNum==lastRec.lastPageNum){	//xzjin ?????????????????????????????????
//		writeRec(fileOffset, src, dest, pageNum, fileName);
//	}else
	{		//xzjin ????????????????????????????????????
		struct recTreeNode node, *nodePtr, **pt;
		node.pageNum = pageNum;
		pt = tfind(&node, &recTreeRoot, recCompare);
		if(UNLIKELY(pt)){	//xzjin ??????????????????
			//MSG("Found in tree\n");
			nodePtr = pt[0];
			//xzjin update lastRec, it is one of parameters that writeRec needs, 
			//must update lastRec before callwriteRec
			lastRec.lastPageNum = pageNum;
			lastRec.lastMemRec = nodePtr->recModEntry->recArr;
			//xzjin ????????????????????????????????????
			lastRec.lastIdx = &(nodePtr->memRecIdx);
			//Check whether new address is smaller than older
			//If true, delete node content
#if DELETE_TREENODE
			{
				struct memRec* lastMenRec = lastRec.lastMemRec;
				int idx = *lastRec.lastIdx;
				struct recBlockEntry *ent;
			    struct tailhead *head = nodePtr->listHead;
				assert(idx>=0);
				//TODO idx=0?????????
				//	1. memcpy treced ????????????????????????
				//	2. memcpy treced ???dest???????????????????????????????????????????????????
				//NOTE: bigger or equal, equal is important in some cases
				if(idx>0 && lastMenRec[idx-1].pageOffset>=addr2PageOffset(dest)){
//					MSG("\nDeleting memory record list, pageNum:%p, intents to offset:%d\n",
//						 pageNum, addr2PageOffset(dest));
//					listRecTreeNode(pageNum);
					TAILQ_FOREACH(ent, head, entries){
//						DEBUG("delete list head:%p, element:%p\n", head, ent);
						if(TAILQ_NEXT(ent, entries)){
							withdrawMemRecArr(ent->recArr);
							TAILQ_REMOVE(head, TAILQ_FIRST(head), entries);
							withdrawRecBlockEntry(ent);
						}
					}
					nodePtr->recModEntry =  TAILQ_FIRST(head);
					lastRec.lastMemRec = nodePtr->recModEntry->recArr;
					*(lastRec.lastIdx) = 0;
				}
			}
#else
//TODO add search tree node and traverse the node, delete and reorder

#endif	//DELETE_TREENODE
			//DEBUG("insterRec if\n");
			writeRec(fileOffset, src, dest, pageNum, fileName, fd);
//		    struct tailhead *head = pt[0]->listHead;
//			checkEmptyRecTreeNode();
		}else{	//xzjin ??????????????????
			//MSG("Not found in tree\n");
			struct recTreeNode *treeNode = addTreeNode(pageNum);
			nodePtr = treeNode;
			//xzjin update lastRec
			lastRec.lastPageNum = pageNum;
			lastRec.lastMemRec = treeNode->recModEntry->recArr;
			lastRec.lastIdx = &(treeNode->memRecIdx);

			if(UNLIKELY(*lastRec.lastIdx<0 ||*lastRec.lastIdx>MEMRECPERENTRY)){
				MSG("lastRec.lastIdx ERROR\n");
				exit(-1);
			}
			writeRec(fileOffset, src, dest, pageNum, fileName, fd);
//			checkEmptyRecTreeNode();
		}
//		MSG("Node pageNum:%p, memRecIdx:%d, nodePtr:%p.\n",
//			 nodePtr->pageNum, nodePtr->memRecIdx, nodePtr);
	}
//	checkEmptyRecTreeNode();
	if(UNLIKELY(*lastRec.lastIdx<0 ||*lastRec.lastIdx>MEMRECPERENTRY)){
		MSG("lastRec.lastIdx ERROR\n");
	}
}

#else

void deleteRec(void* src, void* dest, unsigned long length){
	struct memRec *rec, **searchRes = NULL;
	//g_hash_table_remove_all(searchedMemRec);
	rec = allocateMemRecArr();
	//MSG("alloc rec:%p\n", rec);
	rec->startMemory = (unsigned long long)dest;
	rec->tailMemory = (unsigned long long)dest + length;
	rec->fileName = NULL;
	rec->fileOffset = 0;
	if(rec->startMemory >= rec->tailMemory){
		MSG("rec error\n");
	}
	//xzjin assert
	assert(rec->startMemory<rec->tailMemory);
	assert(rec->startMemory>343596402010 && rec->tailMemory>343596402010);

	searchRes = tfind(rec, &recTreeRoot, overlapRec);
/** xzjin compare relationship			
 * a.			|_________|			  a ? b
 * b.2 	|____________|					= b.tailMemory = a.startMemory-1;
 * 			//TODO add the tail part into memory record
 * b.3 	|__________________________|	= b.tailMemory = a.startMemory-1;
 * b.4 				|___|				= delete b;
 * b.5 				|______________|	= b.startMemory = a.tail;
*/
	while(searchRes){
		//MSG("Modify/delete record.\n");
		//gpointer hashEntry = malloc(sizeof(struct memRec*));
		struct memRec *b = *searchRes;
		tdelete(rec, &recTreeRoot, overlapRec);
//		if(b==rec)	listRecTreeDetail();
//		//printRec(b);
//		//hashEntry = b;
//		//TODO This assumes the tfind searchs from head to tail.
//		rec->startMemory = b->tailMemory + 1;
//		//g_hash_table_add(searchedMemRec, hashEntry);
//
//		if(b->startMemory<rec->startMemory){	//b.2, b.3
//			b->tailMemory = rec->startMemory -1;
//			//xzjin assert
//			assertRec(b);
//		}else if(b->tailMemory <= rec->startMemory){		//b.4
//			tdelete(rec, &recTreeRoot, overlapRec);
//			MSG("b:%p\n", b);
//			withdrawMemRecArr(b);
//		}else{	//b.5
//			b->tailMemory = rec->startMemory;
//			//xzjin assert
//			assertRec(b);
//		}
//		if(rec->startMemory >= rec->tailMemory) break;
//		//xzjin assert
//		assert(rec->startMemory<rec->tailMemory);
//		assert(rec->startMemory>343596402010 && rec->tailMemory>343596402010);
		searchRes = tfind(rec, &recTreeRoot, overlapRec);
	}

	//MSG("free rec:%p\n", rec);
	withdrawMemRecArr(rec);
}

inline void insertRec(unsigned long fileOffset, void* src, void* dest,
	 char* fileName, unsigned long length) __attribute__((always_inline));
inline void insertRec(unsigned long fileOffset, void* src, void* dest,
	 char* fileName, unsigned long length){

	struct memRec *rec;
	//MSG("%s: START\n", __func__);
	rec = allocateMemRecArr();
	//MSG("length:%lu, fileOffset:%lu\n", length, fileOffset);
	deleteRec(src, dest, length);
	rec->fileName= fileName;
	rec->fileOffset = fileOffset;
	rec->startMemory = (unsigned long long)dest;
	rec->tailMemory = (unsigned long long)dest+ length;
//	rec->startMemory = (unsigned long long)src;
//	rec->tailMemory = (unsigned long long)src + length;

	//printRec(rec);
	//xzjin assert
	assertRec(rec);
	//assert(rec->fileOffset<100000000000002);
    pthread_mutex_lock(&recTreeMutex);
	tsearch(rec, &recTreeRoot, recCompare);
    pthread_mutex_unlock(&recTreeMutex);
	//MSG("%s: END\n", __func__);
}
#endif //BASE_VERSION

// Creates the set of standard posix functions as a module.
__attribute__((constructor))void init(void) {
#ifndef BASE_VERSION
	char* version = "";
#else
	char* version = "BASE";
#endif	//BASE_VERSION
	execv_done = 0;
	int tmp;
	int err __attribute__((unused));
	MSG("Initializing the libawn.so, %s.\n", version);

#if DELETE_TREENODE
	MSG("DELETE_TREENODE.\n");
#else
	MSG("NOT DELETE_TREENODE.\n");
#endif	//DELETE_TREENODE

#if BYTE_COMPARE
	MSG("BYTE_COMPARE.\n");
#else
	MSG("NOT BYTE_COMPARE.\n");
#endif	//DELETE_TREENODE
	 
	if (pthread_mutex_init(&recTreeMutex, 0)){
        ERROR("Failed to init mutex!");
        exit(-1);
    }

	//xzjin Put dlopen before call to memcpy and mmap call.
	ts_write_size = 0;
 	ts_write_same_size = 0;
 	ts_write_not_found_size = 0;
	ts_metadataItem = 0;
	mmapSrcCache = calloc(MMAPCACHESIZE, sizeof(struct searchCache));
	mmapDestCache = calloc(MMAPCACHESIZE, sizeof(struct searchCache));
	FD2PATH = calloc(FILEMAPTREENODEPOOLSIZE, sizeof(char*));
#ifdef BASE_VERSION
//	searchedMemRec = g_hash_table_new(NULL, NULL);
#endif	//BASE_VERSION

	memset(openFileArray, 0, sizeof(openFileArray));
	debug_fd = fopen("log", "w");
	err = errno;
	if(debug_fd==NULL){
		MSG("open log file error, %s.\n", strerror(err));
	}

	lastTs_memcpyFmNode = NULL;
	leastRecFCache.idx=-1;
	leastRecFCache.leastUsedTime = INT32_MAX;

	//xzjin Allocate pool for node
	initMemory();

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
	DEBUG("PAGENUMMASK is %llX\n",PAGENUMMASK);

	proMapper = NULL;
	/** xzjin ?????????pvalloc??????????????????????????????, need to be set before 
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
	DEBUG("_hub_mmap returned:%p fileName:%s, fd:%d\n",ret, GETPATH(file), file);

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
		treeNode->fd = file;
		openFileArray[file%MAX_FILE_NUM] = treeNode;
		tsearch(treeNode, &fileMapTreeRoot, fileMapTreeInsDelCompare);
		tsearch(treeNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
	}
	END_TIMING(fileMap_t, fileMap_time);
out:
	return ret;
}

//Add by xzjin
//?????????unmap????????????????????????
RETT_MUNMAP ts_munmap(INTF_MUNMAP) {
	
	int ret;
//	MSG("_hub_unmmap addr:%p,len:%llu\n", addr, len);

//	ret = _hub_fileops->MUNMAP(CALL_MUNMAP);
	ret = munmap(CALL_MUNMAP);
	DEBUG("_hub_munmap returned:%d.\n",ret);
	START_TIMING(fileUnmap_t, fileUnmap_time);
	struct fileMapTreeNode treeNode, **delNodep;
	treeNode.start = addr;
	//TODO ?????????????????????????????????????????????????????????????????????????????????
	delNodep = tfind(&treeNode, &fileMapTreeRoot, fileMapTreeInsDelCompare);
	if(delNodep && delNodep[0]){
		struct fileMapTreeNode *freep = *delNodep;
		openFileArray[(delNodep[0]->fd)%MAX_FILE_NUM] = NULL;
		//xzjin ?????????tdelete?????????delNodep?????????????????????
		tdelete(&treeNode, &fileMapTreeRoot, fileMapTreeInsDelCompare);
		//??????????????????????????????fileMapTree?????????/????????????
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

#ifndef BASE_VERSION
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
	//TODO xzjin ???????????????????????????????????????????????????????????????????????????
	fmNode.fileName = mrp->fileName;
	fmSearNodep = tfind(&fmNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
//	assert(fmSearNodep);
	//zxjin TODO maybe add UNLIKELY?
	if(UNLIKELY(!fmSearNodep)) return (~0);
	fmTarNode = *fmSearNodep;

	//xzjin ???????????????????????????
	if(LIKELY(fmTarNode->offset <= mrp->fileOffset)){
		//xzjin ?????????????????????+???????????????????????????-?????????????????????
		fileCmpStart = (unsigned long)(fmTarNode->start+mrp->fileOffset-fmTarNode->offset);
#if PATCH
		//xzjin ??????patch?????????fileCmpStart??????????????????????????????????????????????????????????????????
		fileCmpStart += (unsigned long)buf-bufCmpStart;
		bufCmpStart = (unsigned long)buf;
#endif //PATCH
		cmpLen = tail-bufCmpStart;
		START_TIMING(memcmp_asm_t, memcmp_asm_time);
		//xzjin TODO ??????????????????????????????????????????diffPos????????????????????????if??????
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

void do_ts_write(int file, void *buf, size_t length, unsigned long tail,
	 unsigned long long start, unsigned long long end){
	START_TIMING(compare_mem_t, compare_mem_time);
	int toTailLen, idx; 
	struct recTreeNode searchNode;
	void* diffPos = buf;
	void* bufCmpStart;
	struct memRec *mrp;
	unsigned long sameLen;
	int sameContentTimes = 0;
	int curWriteSameLen = 0;
	//xzjin ?????????????????????????????????????????????????????????????????????diffPos??????
	for(int i=0; start<=end; start++,i++){
//		MSG("compare page: %p\n", (void*)start);
//		int writeLenCurPage = 4096;
		int sameLenCurPage = 0;
		int notFoundLen;
		toTailLen = length-((unsigned long)getAddr((void*)start,0)-(unsigned long)buf);
//		float samePercent;
//		DEBUG("compare, Page num:%lu.\n", start);
		if((unsigned long)getAddr((void*)start, 4095)<(unsigned long)diffPos) continue;
//		if(start == (unsigned long long)addr2PageNum(buf)){
//			writeLenCurPage = 4096-addr2PageOffset(buf);
//		}else if(end == start){
//			writeLenCurPage = addr2PageOffset((void*)tail);
//		}
		searchNode.pageNum = (void*)start;
//		START_TIMING(tfind_t, tfind_time);
		struct recTreeNode **res = tfind(&searchNode, &recTreeRoot, recCompare);
//		END_TIMING(tfind_t, tfind_time);
		if(res){
//			MSG("inside page: %p\n", (void*)start);
//			listRecTreeNode((void*)start);
			struct recBlockEntry *blockEntry = TAILQ_LAST(res[0]->listHead, tailhead);
			//xzjin ??????????????????foreach???????????????????????????????
			while(blockEntry){
				int i=0;
				mrp = blockEntry->recArr;
				if(res[0]->recModEntry == blockEntry ){
					idx = res[0]->memRecIdx;
				}else{
					idx = MEMRECPERENTRY;
				}					
				//xzjin ????????????buf????????????
//				while (((unsigned long)getAddr((void*)start, mrp->pageOffset) < (unsigned long) buf) &&
				//xzjin ????????????diffPos????????????
				while (((unsigned long)getAddr((void*)start, mrp->pageOffset) < (unsigned long) diffPos) &&
					i<idx ) {
					mrp++; i++;
				}
				
				for(; i<idx; mrp++,i++){
					DEBUG("i=%d, mrp:%p.\n", i, mrp);
					bufCmpStart = getAddr((void*)start, mrp->pageOffset);
					if((unsigned long)bufCmpStart < (unsigned long) diffPos) continue;
					if((unsigned long)bufCmpStart > (unsigned long) tail) ;
					sameLen = cmpWrite((unsigned long)bufCmpStart, mrp, buf, tail, &diffPos, file);
					sameContentTimes += sameLen?1:0;
					sameLenCurPage += sameLen;
					curWriteSameLen +=sameLen;
				}
				blockEntry = TAILQ_PREV(blockEntry, tailhead, entries);
			}

//delTail:
//			float samePercent = ((double)sameLenCurPage/writeLenCurPage)*100;
//			MSG("same percentage this page: %.2f%%, pageLen:%d, samelen:%d, writePageNum:%p, curCmpPageNum:%p\n", samePercent, writeLenCurPage, sameLenCurPage, addr2PageNum(buf), (void*)start);
//			listRecTreeNode((void*)start);
//			MSG("\n");
			//xzjin ?????????????????????????????????????????????
			//xzjin TODO????????????????????????????????????????????????????????????????????????????????????
			//????????????????????????????????????????????????
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
				//xzjin tdelete???????????????delNodep??????????????????????????????tdelete????????????
				struct recTreeNode *freep = *delNodep;
				tdelete(&searchNode, &recTreeRoot, recCompare);
				//free(freep);
				withdrawRecTreeNode(freep);
			}
			*/
//			checkEmptyRecTreeNode();
		}else{
//			MSG("page:%p not found, size:%d.\n", (void*)start, notFoundLen);
			notFoundLen = MIN(PAGESIZE, toTailLen);
			//TODO ???????????????????????????????????????????????????????????????????????????
			ts_write_not_found_size += notFoundLen;
//			checkEmptyRecTreeNode();
		}
	}
//	double samePercent = curWriteSameLen*100/length;
//	MSG("ts_write buf:%p, tail:%p, length:%lu, same time:%d, same len:%d, same occupy:%.2f\n", buf, (void*)tail, length, sameContentTimes, curWriteSameLen, samePercent);
	END_TIMING(compare_mem_t, compare_mem_time);
}

#else

int memcmp_bbb(void *s1, void *s2, size_t n, void** firstDiffPos){
	char* a = (char*)s1;
	char* b = (char*)s2;
	//MSG("%s, start.\n", __func__);
	for(int i=0; i<n; i++, a++, b++){
		if((*a)-(*b)){
			*firstDiffPos = (void*)a;
			//MSG("%s, end.\n", __func__);
			return (*a)-(*b);
		}
	}
	//MSG("%s, end.\n", __func__);
	return 0;
}
//compare and call write to file metedata
inline unsigned long cmpWrite(struct memRec *mrp, 
		  unsigned long tail, void** diffPos) __attribute__((always_inline));
inline unsigned long cmpWrite(struct memRec *mrp, unsigned long tail, void** diffPos){
	START_TIMING(cmp_write_t, cmp_write_time);
	unsigned long bufCmpStart = mrp->startMemory;
	unsigned long fileCmpStart;
	unsigned long cmpLen;
	unsigned long addLen = 0;	//same content length	
	struct fileMapTreeNode fmNode, **fmSearNodep, *fmTarNode;
//	MSG("CMP write, fileOffset:%lu, pageOffset:%d, fileName:%s.\n",
//		mrp[i].fileOffset, mrp[i].pageOffset, mrp[i].fileName);

	//MSG("%s, start.\n", __func__);
#ifdef CMPWRITE
	//TODO xzjin ???????????????????????????????????????????????????????????????????????????
	fmNode.fileName = mrp->fileName;
	fmSearNodep = tfind(&fmNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
//	assert(fmSearNodep);
	//zxjin TODO maybe add UNLIKELY?
	if(UNLIKELY(!fmSearNodep)) return (~0);
	fmTarNode = *fmSearNodep;

	//xzjin ???????????????????????????
	if(LIKELY(fmTarNode->offset <= mrp->fileOffset)){
		//xzjin ?????????????????????+???????????????????????????-?????????????????????
		fileCmpStart = (unsigned long)(fmTarNode->start+mrp->fileOffset-fmTarNode->offset);
#if PATCH
		//xzjin ??????patch?????????fileCmpStart??????????????????????????????????????????????????????????????????
		fileCmpStart += (unsigned long)buf-bufCmpStart;
		bufCmpStart = (unsigned long)buf;
#endif //PATCH
		cmpLen = tail-bufCmpStart;
		START_TIMING(memcmp_asm_t, memcmp_asm_time);
		//int cmpRet = memcmp_avx2_asm((void*)bufCmpStart, (void*)fileCmpStart, cmpLen, diffPos);
		int cmpRet = memcmp_bbb((void*)bufCmpStart, (void*)fileCmpStart, cmpLen, diffPos);
		END_TIMING(memcmp_asm_t, memcmp_asm_time);
		if(cmpRet){		//different
			addLen = (tail<(unsigned long)*diffPos?tail:(unsigned long)*diffPos) - bufCmpStart;
		}else{		//same
			addLen = cmpLen;
			*diffPos = (void*)tail;
		}
		ts_metadataItem++;
//		MSG("addLen: %lu\n", addLen);
		ts_write_same_size += addLen;
	}else{
		//MSG("ts_write fMapCache not hit\n");
	}
//	MSG("buf cmp start:%lu, file cmp start:%lu, diffPos:%lu\n",
//		 (unsigned long)bufCmpStart, (unsigned long)fileCmpStart, (unsigned long)(*diffPos));
//	MSG("same length: %llu, %p, pid:%d, ppid:%d.\n",
//		 ts_write_same_size, &ts_write_same_size, getpid(), getppid());
// 	MSG(" ts_write bytes :%llu\n", ts_write_size);
//	END_TIMING(cmp_write_t, cmp_write_time);
	//MSG("%s, end.\n", __func__);
#endif //CMPWRITE
	return addLen;
}

void do_ts_write(int file, void *buf, size_t length, unsigned long tail,
	 unsigned long long start, unsigned long long end){

	//MSG("%s, start.\n", __func__);
	struct memRec *rec, **searchRes;

	//listRecTreeDetail();
	void* diffPos = NULL;
	rec = allocateMemRecArr();
	rec->startMemory = (unsigned long long)buf;
	rec->tailMemory = (unsigned long long)buf + length;
	searchRes = tfind(rec, &recTreeRoot, overlapRecBegBigger);

	while(searchRes){
		struct memRec *b = *searchRes;
		//MSG("find rec: %s\n", b->fileName);
		//TODO This assumes the tfind searchs from head to tail.
		rec->startMemory = b->tailMemory + 1;
		//printRec(b);
		//printRec(rec);
		cmpWrite(b, tail, &diffPos);
		if(rec->startMemory >= rec->tailMemory) break;
		//xzjin assert
		assert(rec->startMemory<rec->tailMemory);
		assert(rec->startMemory>343596402010 && rec->tailMemory>343596402010);
		//xzjin assert
		assertRec(b);
		searchRes = tfind(rec, &recTreeRoot, overlapRecBegBigger);
	}

	withdrawMemRecArr(rec);
	//MSG("%s, end.\n", __func__);
}

#endif	//BASE_VERSION

ssize_t ts_write(int file, void *buf, size_t length){
	ts_writeTime++;
	ts_writeTime = ts_writeTime%0xFFFFFF;
	void *t = buf;
	unsigned long tail = (unsigned long)buf+length;
	unsigned long long start = (unsigned long long)addr2PageNum(t);
	unsigned long long end = (unsigned long long)addr2PageNum((void*)((unsigned long long)buf+length));
//	MSG("\n\n\n\n\n\npage: %llx, tail page:%llx, buf:%p, len:%lu\n", start, end, buf, length);

	//TODO xzjin ????????????????????????????????????????????????????????????
	//off_t fpos = lseek(file, 0, SEEK_CUR);
	ts_write_size += length;
	START_TIMING(ts_write_t, ts_write_time);
	unsigned long ret = write(CALL_WRITE);
	END_TIMING(ts_write_t, ts_write_time);
//	MSG("ts_write: buf:%p, file:%d, len: %llu, fileName:%s\n", 
//		buf, file, length, GETPATH(file));
//	listRecTreeDetail();
#if USE_TS_FUNC 
	do_ts_write(file, buf, length, tail, start, end);
#endif //USE_TS_FUNC 
	//MSG("%s, end.\n", __func__);
	return ret;
}

ssize_t ts_pwrite(int file, void *buf, size_t length, off64_t offset){
	void *t = buf;
	unsigned long tail = (unsigned long)buf+length;
	unsigned long long start = (unsigned long long)addr2PageNum(t);
	unsigned long long end = (unsigned long long)addr2PageNum((void*)((unsigned long long)buf+length));

	//TODO xzjin ????????????????????????????????????????????????????????????
	//off_t fpos = lseek(file, 0, SEEK_CUR);
	ts_write_size += length;
	START_TIMING(ts_write_t, ts_write_time);
	unsigned long ret = pwrite(CALL_WRITE, offset);
	END_TIMING(ts_write_t, ts_write_time);
//	MSG("ts_write: buf:%p, file:%d, len: %llu, fileName:%s\n", 
//		buf, file, length, GETPATH(file));
//	listRecTreeDetail();
#if USE_TS_FUNC 
	//MSG("fd:%d, buf:%lu, length:%lu\n", file, (unsigned long)buf, length);
	do_ts_write(file, buf, length, tail, start, end);
#endif //USE_TS_FUNC 
	return ret;
}

#ifndef BASE_VERSION
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
	//TODO xzjin ????????????????????????????????????????????????????????????
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
	//xzjin ?????????????????????????????????????????????????????????????????????diffPos??????
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
			//xzjin ??????????????????foreach???????????????????????????????
			while(blockEntry){
				int i=0;
				mrp = blockEntry->recArr;
				if(res[0]->recModEntry == blockEntry ){
					idx = res[0]->memRecIdx;
				}else{
					idx = MEMRECPERENTRY;
				}					
				//xzjin ????????????buf????????????
				//xzjin ?????????????????????????????????patch???????????????????????????????????????????????????
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
			//xzjin ?????????????????????????????????????????????
			//xzjin TODO????????????????????????????????????????????????????????????????????????????????????
			//????????????????????????????????????????????????
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
				//xzjin tdelete???????????????delNodep??????????????????????????????tdelete????????????
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
	//xzjin ?????????????????????????????????????????????????????????????????????diffPos??????
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
			//xzjin ??????????????????foreach???????????????????????????????
			while(blockEntry){
				int i=0;
				mrp = blockEntry->recArr;
				if(res[0]->recModEntry == blockEntry ){
					idx = res[0]->memRecIdx;
				}else{
					idx = MEMRECPERENTRY;
				}					
				//xzjin ????????????buf????????????
				//xzjin ?????????????????????????????????patch???????????????????????????????????????????????????
				while (((unsigned long)getAddr((void*)start, mrp->pageOffset) < (unsigned long) buf) &&
					i<idx ) {
					mrp++; i++;
				}
				
				for(; i<idx; mrp++,i++){
					DEBUG("i=%d, mrp:%p.\n", i, mrp);
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
			//xzjin ?????????????????????????????????????????????
			//xzjin TODO????????????????????????????????????????????????????????????????????????????????????
			//????????????????????????????????????????????????
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
				//xzjin tdelete???????????????delNodep??????????????????????????????tdelete????????????
				struct recTreeNode *freep = *delNodep;
				tdelete(&searchNode, &recTreeRoot, recCompare);
				//free(freep);
				withdrawRecTreeNode(freep);
			}
			*/
//			checkEmptyRecTreeNode();
		}else{
			notFoundLen = MIN(PAGESIZE, toTailLen);
			MSG("buf page: %llX not found, not found size:%d.\n", start, notFoundLen);
			ts_write_not_found_size += notFoundLen;
//			checkEmptyRecTreeNode();
		}
	}
#endif //NOT_PATCH
//	MSG("ts_write buf:%p, tail:%p, length:%lu, same time:%d, same len:%d, same occupy:%d%%\n\n\n",
//		 buf, tail, length, sameContentTimes, curWriteSameLen, (int)(curWriteSameLen*100/length));
	END_TIMING(compare_mem_t, compare_mem_time);
#endif //USE_TS_FUNC 
	return ret;
}
#endif	//BASE_VERSION

#ifndef BASE_VERSION
//xzjin ?????????????????????????????????????????????
void* ts_memcpy_traced(void *dest, void *src, size_t n){
	unsigned long long start = (unsigned long long)addr2PageNum(src);
	unsigned long long srcPageNum = start;
	unsigned long long destStart = (unsigned long long)addr2PageNum(dest);
	unsigned long long end = (unsigned long long)addr2PageNum((void*)((unsigned long long)src+n));
	struct recTreeNode searchNode;
	void *ret;

	ret = memcpy(CALL_MEMCPY);

#if USE_TS_FUNC
//	DEBUG("ts_memcpy_traced: From: %lu, from tail: %lu, to??? %lu, to tail: %lu, length: %lu.\n",
//		src, src+n, dest, dest+n, n);
	START_TIMING(mem_from_mem_trace_t, mem_from_mem_trace_time);
	//xzjin TODO,?????????????????????????????????memRec?????????
	for(int i=0; start<=end; start++,destStart++,i++){
//		DEBUG("start:%lu.\n", start);
		searchNode.pageNum = (void*)start;
		struct recTreeNode **srcRes = tfind(&searchNode, &recTreeRoot, recCompare);
		//src address is in tree and dest address is not
		if(srcRes){
			//MSG("\n\n------------xzjin-list-tree-node:%p-------------\n\n", (void*)start);
			//listRecTreeNode((void*)start);
			struct recBlockEntry *srcLastEntry = srcRes[0]->recModEntry;
			struct recBlockEntry *blockEntry = TAILQ_LAST(srcRes[0]->listHead, tailhead);
//			struct recBlockEntry *delBlockEntry;
			struct memRec* rec = blockEntry->recArr;
			int idx = 0, uplimit = MEMRECPERENTRY;
			//xzjin ???????????????????????????src???????????????src????????????
			//xzjin ??????????????????????????????????????????????????????
			//if(blockEntry == srcLastEntry){
			//	uplimit = srcRes[0]->memRecIdx;
			//}
			//int destOffset = addr2PageOffset(dest);
            // if current page is begin page, omit 
			if(start == srcPageNum){
				int destOffset = addr2PageOffset(src);
				//xzjin ??????????????????recArr????????????
				do{
					if(srcRes[0]->recModEntry == srcLastEntry){
						uplimit = srcRes[0]->memRecIdx;
					}
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

				//xzjin ?????????????????????????????????????????????????????????,
				//????????????????????????????????????
				//xzjin ??????recArr????????????????????????
				for(idx=0; idx < uplimit; idx++){
					if(rec[idx].pageOffset >= destOffset){
						srcRes[0]->memRecIdx = idx;
						srcRes[0]->recModEntry = blockEntry;
						break;
					}
				}
			}else{
				uplimit = srcRes[0]->memRecIdx;
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
				struct memRec *curMemRec = rec+idx;
//				MSG("curMemRec:%p, rec:%p, idx:%d\n", curMemRec, rec, idx);
				void* copySrc = getAddr((void*)start, curMemRec->pageOffset);
				void* copyDest;
 				copyDest = dest+(copySrc-src);

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
					//xzjin ???????????????????????????????????????????????????????????????????????????????????????????????????
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
				insertRec(curMemRec->fileOffset, copySrc, copyDest, curMemRec->fileName, curMemRec->fd);
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
			//xzjin TODO ????????????????????????????????????,???????????????
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
//					DEBUG("ts_memcpy_traced DOWN idx:%d, uplimit:%d\n", idx, uplimit);
					insertRec(curMemRec->fileOffset, copySrc, copyDest, curMemRec->fileName, curMemRec->fd);
				}
				//xzjin TODO ????????????????????????????????????????????????????????????
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
//		checkEmptyRecTreeNode();
	}
	END_TIMING(mem_from_mem_trace_t, mem_from_mem_trace_time);
#endif	//USE_TS_FUNC
	
	return ret;
}
#else
//xzjin ?????????????????????????????????????????????
void* ts_memcpy_traced(void *dest, void *src, size_t n){
	unsigned long long end = (unsigned long long)src+n;
	void *ret;

	ret = memcpy(CALL_MEMCPY);

#if USE_TS_FUNC
//	DEBUG("ts_memcpy_traced: From: %lu, from tail: %lu, to??? %lu, to tail: %lu, length: %lu.\n",
//		src, src+n, dest, dest+n, n);
	START_TIMING(mem_from_mem_trace_t, mem_from_mem_trace_time);
	struct memRec *rec, **searchRes;
	//g_hash_table_remove_all(searchedMemRec);
	rec = allocateMemRecArr();
	rec->startMemory = (unsigned long long)src;
	rec->tailMemory = (unsigned long long)src + n;
	searchRes = tfind(rec, &recTreeRoot, overlapRecBegBigger);
	while(searchRes){
		struct memRec *b = *searchRes;
		void* insertDest = dest+(b->startMemory - (unsigned long long)src);
		//xzjin trace tail and copy src tail, choose the smaller one
		size_t copyLen = b->tailMemory<end?b->tailMemory:end;
		//printRec(b);
		//MSG("length:%lu, insertDest:%llu, end:%llu\n",
		//	 copyLen, (unsigned long long)insertDest, end);
		//copyLen = copyLen - (unsigned long long)insertDest;
		copyLen = copyLen - (unsigned long long)src;
//		if(copyLen>32768){
//			printRec(b);
//			MSG("length:%lu, insertDest:%llu, end:%llu\n",
//				 copyLen, (unsigned long long)insertDest, end);
//			MSG("123\n");
//		}
		//memcpy require src, dest not overlap
		//Insert only delete the record overlapped with memcpy dest range
		//b is the memory record in memcpy src, thus will not be deleted
		//xzjin ???delete,???insert,?????????????????????delete?????????
		tdelete(rec, &recTreeRoot, overlapRecBegBigger);
		//xzjin must release b after insert, because b may change after release
		//influence offset and fileName
		insertRec(b->fileOffset, src, insertDest, b->fileName, copyLen);
		withdrawMemRecArr(b);

		searchRes = tfind(rec, &recTreeRoot, overlapRecBegBigger);
	}

	withdrawMemRecArr(rec);
	END_TIMING(mem_from_mem_trace_t, mem_from_mem_trace_time);
#endif	//USE_TS_FUNC
	
	return ret;
}
#endif	//BASE_VERSION

//xzjin ?????????memcpy????????????????????????,??????????????????mmap?????????copy
//ts_memcpy_traced???????????????traced??????mmap???traced????????????????????????????????????
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
	//xzjin ??????copy????????????????????????????????????????????????????????????????????????
#if USE_TS_FUNC
 
	START_TIMING( memcpy_from_mapped_trace_t, mem_from_file_trace_time);

	fileMapTreeNode.start = src;
	fileMapTreeNode.tail = src + n;
	//xzjin TODO ?????????????????????75%??????????????????
	//START_TIMING(ts_memcpy_tfind_file_t, ts_memcpy_tfind_file_time);
	//xzjin ??????buf??????????????????????????????????????????
//	if(!(LIKELY(lastTs_memcpyFmNode) && (lastTs_memcpyFmNode->start<=(unsigned long)src &&
//		 (unsigned long)src<=lastTs_memcpyFmNode->tail)))
	//xzjin TODO ?????????????????????????????????????????????
#if USE_FMAP_CACHE
	struct fileMapTreeNode* fmtp;
	fmNode = lastTs_memcpyFmNode;
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
	//END_TIMING(ts_memcpy_tfind_file_t, ts_memcpy_tfind_file_time);
	fmNode->usedTime++;
//	MSG("copy src found, copyFrom:%lu, copyTo:%lu, map start:%lu, map tail:%lu, map offset:%ld, fileName: %s\n",
//		src, dest,fmNode->start, fmNode->tail, fmNode->offset, fmNode->fileName);
	//xzjin ??????log??????
	fileOffset = fmNode->offset+(src-fmNode->start);
//	MSG("file offset: %lu, fmNode->offset:%lu, src:%lu, fmNode->start:%lu\n",
//		fileOffset, fmNode->offset, src, fmNode->start);
	//START_TIMING(insert_rec_t,  insert_rec_time);
#if PATCH
	//xzjin ??????patch, ?????????????????????
	destTail = dest+n;
	startPageNum = addr2PageNum(dest);
	endPageNum = addr2PageNum(destTail);
	do{
		unsigned long diff;
		insertRec(fileOffset, src, dest, fmNode->fileName, fmNode->fd);
		startPageNum++;
		diff = ((unsigned long)startPageNum<<PAGENUMSHIFT)-(unsigned long)dest;
		fileOffset += diff;
		src += diff;
		dest += diff;
	}while(startPageNum<=endPageNum);
#else
#ifndef	BASE_VERSION
	insertRec(fileOffset, src, dest, fmNode->fileName, fmNode->fd);
#else
	insertRec(fileOffset, src, dest, fmNode->fileName, n);
#endif	//BASE_VERSION
#endif //PATCH
	//END_TIMING(insert_rec_t,  insert_rec_time);
	
#if REC_INSERT_TEST 
	node.pageNum = addr2PageNum(dest);
	pt = tfind(&node, &recTreeRoot, recCompare);
	if(pt && pt[0]->memRecIdx){	//xzjin ??????????????????
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
//	checkEmptyRecTreeNode();
#endif	//REC_INSERT_TEST 
//		MSG("copy src:           %lu not fount in file list.\n", src);
ts_memcpy_returnPoint:
	END_TIMING( memcpy_from_mapped_trace_t, mem_from_file_trace_time);
#endif	//USE_TS_FUNC
	return ret;
}

//ts_memcpy_traced???????????????traced??????mmap???traced????????????????????????????????????
//???????????????????????????????????????????????????????????????????????????
//???????????????ts_read??????
void* ts_memcpy_withFile(void *dest, void *src, size_t n,
	 struct fileMapTreeNode *fmNode){
//	MSG("Memcpy dest:%llu, src:%llu, len:%lu\n",
//				 (unsigned long long)dest, (unsigned long long)src, n);

#if REC_INSERT_TEST 
	struct recTreeNode node,**pt;
#endif
	void *ret;
	void* destTail __attribute__ ((__unused__));
	void* startPageNum __attribute__ ((__unused__));
	void* endPageNum __attribute__ ((__unused__));
	unsigned long fileOffset;
//	MSG("ts_memcpy\n");

	ret = memcpy(CALL_MEMCPY);
	//xzjin ??????copy????????????????????????????????????????????????????????????????????????
#if USE_TS_FUNC
 
	START_TIMING(mem_from_file_trace_t, mem_from_file_trace_time);

	//xzjin TODO ?????????????????????75%??????????????????
	//START_TIMING(ts_memcpy_tfind_file_t, ts_memcpy_tfind_file_time);
	//xzjin ??????buf??????????????????????????????????????????
//	if(!(LIKELY(lastTs_memcpyFmNode) && (lastTs_memcpyFmNode->start<=(unsigned long)src &&
//		 (unsigned long)src<=lastTs_memcpyFmNode->tail)))
	//xzjin TODO ?????????????????????????????????????????????
#if USE_FMAP_CACHE
	struct fileMapTreeNode* fmtp;
	fmNode = lastTs_memcpyFmNode;
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
	if(!fmNode){
		goto ts_memcpy_returnPoint;
	}
#endif	// USE_FMAP_CACHE
	//END_TIMING(ts_memcpy_tfind_file_t, ts_memcpy_tfind_file_time);
	fmNode->usedTime++;
//	MSG("copy src found, copyFrom:%lu, copyTo:%lu, map start:%lu, map tail:%lu, map offset:%ld, fileName: %s\n",
//		src, dest,fmNode->start, fmNode->tail, fmNode->offset, fmNode->fileName);
	//xzjin ??????log??????
	fileOffset = fmNode->offset+(src-fmNode->start);
	//START_TIMING(insert_rec_t,  insert_rec_time);
#if PATCH
	//xzjin ??????patch, ?????????????????????
	destTail = dest+n;
	startPageNum = addr2PageNum(dest);
	endPageNum = addr2PageNum(destTail);
	do{
		unsigned long diff;
		insertRec(fileOffset, src, dest, fmNode->fileName, fmNode->fd);
		startPageNum++;
		diff = ((unsigned long)startPageNum<<PAGENUMSHIFT)-(unsigned long)dest;
		fileOffset += diff;
		src += diff;
		dest += diff;
	}while(startPageNum<=endPageNum);
#else
#ifndef BASE_VERSION
	insertRec(fileOffset, src, dest, fmNode->fileName, fmNode->fd);
#else
	insertRec(fileOffset, src, dest, fmNode->fileName, n);
#endif	//BASE_VERSION

#endif //PATCH
	//END_TIMING(insert_rec_t,  insert_rec_time);
	
#if REC_INSERT_TEST 
	node.pageNum = addr2PageNum(dest);
	pt = tfind(&node, &recTreeRoot, recCompare);
	if(pt && pt[0]->memRecIdx){	//xzjin ??????????????????
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
//	checkEmptyRecTreeNode();
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
	int err __attribute__((unused));

	treeNode = allocateFileMapTreeNode();
	//treeNode->fileName = fd2path[fd%100];
	treeNode->fileName = GETPATH(fd);
	targetNodep = tfind(treeNode, &fileMapNameTreeRoot, fileMapNameTreeInsDelCompare);
	withdrawFileMapTreeNode(treeNode);
	if(UNLIKELY(!targetNodep)){
		ERROR("find mmap ERROR, file not mapped.\n");
		exit(-1);
	}
//	MSG("read from file:%s\n", fd2path[fd%100]);
	node = targetNodep[0];
	//xzjin ?????????lseek?????????????????????
	seekRet = lseek(fd, 0, SEEK_CUR);
	if(seekRet == -1){
		err = errno;
		ERROR("lseek ERROR, %s, errno %d.\n", strerror(err), err);
		exit(-1);
	}
	copyBegin = (void *)(node->start+(seekRet - node->offset));
	copyLen = (size_t)node->tail-(size_t)copyBegin;
	copyLen = MIN(copyLen, nbytes);
	//ts_memcpy(buf, copyBegin, copyLen);
 	ts_memcpy_withFile(buf, copyBegin, copyLen, openFileArray[fd%MAX_FILE_NUM]);

	seekRet = lseek(fd, copyLen, SEEK_CUR);
	err = errno;
	if(seekRet == -1){
		ERROR("seek to %ld.\n", seekRet+copyLen);
		ERROR("lseek ERROR, %s, errno %d.\n", strerror(err), err);
		exit(-1);
	}
	DEBUG("ts_read to %p ,len %ld ,from %p, fd:%d, fileName:%s.\n",
		buf, copyLen, copyBegin, fd, GETPATH(fd));
	return copyLen;
}

#ifndef BASE_VERSION
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
					insertRec(rec[i].fileOffset, NULL, dest, rec[i].fileName, rec[i].fd);
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
//	checkEmptyRecTreeNode();
	END_TIMING(remap_mem_rec_t, remap_mem_rec_time);
#endif 	//USE_TS_FUNC
	return ret;
}
#endif	//BASE_VERSION

//xzjin ????????????????????????????????????????????????
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
//					ERROR("???????????? rbe:%p pool:%p poolTail:%x\n", 
//							rbe, RECARRPOOL, RECARRPOOLTAIL);
//				}
//				withdrawMemRecArr(rbe->recArr);
//		        withdrawRecBlockEntry(rbe);
//		        SLIST_REMOVE_HEAD(headp, entries);
//		    }
//			withdrawSlistHead(headp);
//			/** xzjin TODO ????????????????????????????????????????????????,??????free??????
//			 * malloc?????????????????????free??????????????????????????????????????????rbTree
//			 * ???node????????????malloc??? */
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
	ts_print_statistics();
	PRINT_TIME();
}

int ts_open(const char *path, int oflag, ...){
	char *abpath = NULL;
    struct stat st;
	int result, err;
	mode_t mode = 0;
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
		ERROR("Open %s, get realpath failed, return:%d.\n", path, result);
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
	mode_t mode = 0;
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
			 path, fullpath, strerror(err), err);
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
	int err, fd = 0;
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
		ERROR("Open %s , get realpath failed.\n", filename);
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
	MSG("file: %s\n", filename);
	return result;
}

//TODO ts_close ?????????????????????unmap?????????
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
	//TODO ?????????????????????????????????????????????????????????????????????????????????
	delNodep = tfind(&treeNode, &fileMapTreeRoot, fileMapNameTreeInsDelCompare);
	if(delNodep && delNodep[0]){
		struct fileMapTreeNode *freep = *delNodep;
		openFileArray[(delNodep[0]->fd)%MAX_FILE_NUM] = NULL;
		//xzjin ?????????tdelete?????????delNodep?????????????????????
		tdelete(&treeNode, &fileMapTreeRoot, fileMapNameTreeInsDelCompare);
		//??????????????????????????????fileMapTree?????????/????????????
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
