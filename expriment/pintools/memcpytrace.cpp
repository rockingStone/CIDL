/*
 * Copyright 2002-2020 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */


#include "pin.H"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <memcpytrace.h>
#include <search.h>
using std::hex;
using std::cerr;
using std::string;
using std::ios;
using std::endl;

/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#define MEMCPY "memcpy@plt"
#define MSG printf

unsigned long memcpyTime;
unsigned long insertExistingTime;
unsigned long arrSize;
unsigned long currNodeNum;

void* memcpyRecordTree;
void* memcpyRecordArr;
std::ofstream TraceFile;

int memcpyRecTreeInsDelCompare(const void *pa, const void *pb) {
    const struct memcpyNode* a = (struct memcpyNode*)pa;
    const struct memcpyNode* b = (struct memcpyNode*)pb;
    if (a->start < b->start)
        return -1;
    if (a->start > b->start)
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
int memcpyRecTreeSearchCompare(const void *pa, const void *pb) {
    const struct memcpyNode* a = (struct memcpyNode*)pa;
    const struct memcpyNode* b = (struct memcpyNode*)pb;
    if ((unsigned long)(a->start) > (unsigned long)(b->start)&&
            (unsigned long)(a->start) > (unsigned long)(b->tail))
        return 1;
    if ((unsigned long)(a->start) < (unsigned long)(b->start)&&
            (unsigned long)(a->tail) < (unsigned long)(b->start))
        return -1;
    return 0;
}

void showRecMapNode(struct memcpyNode** nodep){
    struct memcpyNode *fmNode;
	fmNode = *(struct memcpyNode**) nodep;

	MSG("start: %p, tail:%p.\n", fmNode->start, fmNode->tail);
}

void listMemcpyRecAction(const void *nodep, const VISIT which, const int depth){

    switch (which) {
    case preorder:
        break;
    case postorder:
		showRecMapNode((struct memcpyNode **)nodep);
        break;
    case endorder:
        break;
    case leaf:
		showRecMapNode((struct memcpyNode **)nodep);
        break;
    }
}

void listRecTree(){
	twalk(memcpyRecordTree, listMemcpyRecAction);
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "memcpytrace.out", "specify trace file name");

inline struct memcpyNode* allocateMemcpyTreeNode()  __attribute__((always_inline));
inline struct memcpyNode* allocateMemcpyTreeNode(){
    return (struct memcpyNode*)malloc(sizeof(struct memcpyNode));
}

inline void withdrawTreeNode()  __attribute__((always_inline));
inline void withdrawTreeNode(void* addr){
    free(addr);
    return;
}

inline struct memcpyNode** addrInMemcpyTreeNode(void* start, void* tail)  __attribute__((always_inline));
inline struct memcpyNode** addrInMemcpyTreeNode(void* start, void* tail){
	struct memcpyNode **nodeExist;
	struct memcpyNode *treeNode = allocateMemcpyTreeNode();

	treeNode->start = start;
	treeNode->tail = tail;
	nodeExist = (struct memcpyNode**)tfind(treeNode, &memcpyRecordTree, memcpyRecTreeSearchCompare);
    withdrawTreeNode((void*)treeNode);
    return nodeExist;
}

int delTreeNode(void* start, void* tail, struct memcpyNode* node){
    void* freep;
	struct memcpyNode *treeNode = allocateMemcpyTreeNode();

    if(!node){
        freep = node;
    }else{
        freep = *(addrInMemcpyTreeNode(start, tail));
    }
	treeNode->start = start;
	treeNode->tail = tail;
	tdelete(treeNode, &memcpyRecordTree, memcpyRecTreeInsDelCompare);
    withdrawTreeNode((void*)treeNode);
    withdrawTreeNode(freep);
    return 1;
}

void addMemcpyTreeNode(struct memcpyNode* node){
	tsearch(node, &memcpyRecordTree, memcpyRecTreeInsDelCompare);
    if(currNodeNum >= arrSize){
        arrSize = arrSize*2;
        memcpyRecordArr = realloc(memcpyRecordArr, 
            sizeof(struct memcpyNode)*arrSize);
        if(memcpyRecordArr == NULL){
            printf("realloc array ERROR.\n");
            exit(-1);
        }
    }
    //DONE xzjin This is linear search, change to binary search?
    //Or directly add to tail.
    //bsearch able to search in sorted array.
//    lsearch(node, memcpyRecordArr, &currNodeNum,
//         sizeof(struct memcpyNode*), memcpyRecTreeInsDelCompare);
    {
        struct memcpyNode* array = (struct memcpyNode*)memcpyRecordArr;
        array[currNodeNum].start = node->start;
        array[currNodeNum].tail = node->tail;
        currNodeNum++;
    }
    qsort(memcpyRecordArr, currNodeNum, sizeof(struct memcpyNode*),
         memcpyRecTreeInsDelCompare);
    return;
}

VOID ArgBeforeExe(CHAR * name, ADDRINT from, ADDRINT to, ADDRINT size){
	//struct memcpyNode **nodeExist;
	void* nodeExist;
	struct memcpyNode *existingNode;
	struct memcpyNode *treeNode = allocateMemcpyTreeNode();

    memcpyTime++;
    TraceFile << name << "from: " << from << ", to: " << to 
    <<", length: "<< size << endl;
	treeNode->start = (void*)to;
	treeNode->tail = (void*)(to+size);
    
    /** if exist node overlap with current, 
     *  (1) current covers existing node range, delete old node
     *  (2) current overlap partily with existing node, modify
     *  exist node to not overlap.
     *  repeat until not overlap found.
     */
    nodeExist = addrInMemcpyTreeNode((void*)to, (void*)(to+size));
    while(nodeExist){
        insertExistingTime++;
        existingNode = *(struct memcpyNode**)nodeExist;
/** xzjin compare relationship			
 * a.			|_________|			  a(treeNode) ? b(existingNode)
 * b.1 	|___|							>
 * b.2 	|____________|					= b.tail = a.head-1
 * b.3 	|__________________________|	=
 *           add c.head = a.tail+1, c.tail = b.tail; reset b.tail = a.head-1
 * b.4 				|___|				= delete b
 * b.5 				|______________|	= b.head = a.tail+1
 * b.6 						|___|		<
*/
        if(treeNode->start<=existingNode->start &&
             treeNode->tail>=existingNode->tail){   //case b.4
            // delete existingNode
            delTreeNode(existingNode->start, existingNode->tail, existingNode);
        }else if(treeNode->start>existingNode->start &&
             treeNode->tail<existingNode->tail){    //case b.3
	        struct memcpyNode *addNode = allocateMemcpyTreeNode();
            addNode->start = (void*)((unsigned long)treeNode->tail+1);
            addNode->tail = existingNode->tail;
	        //tsearch(addNode, &memcpyRecordTree, memcpyRecTreeInsDelCompare);
            addMemcpyTreeNode(addNode);
            existingNode->tail = (void*)((unsigned long)treeNode->start-1);
        }else if(treeNode->start<=existingNode->tail){  //case b.2
            existingNode->tail = (void*)((unsigned long)treeNode->start-1);
        }else if(treeNode->tail>=existingNode->start){  //case b.5
            existingNode->start = (void*)((unsigned long)treeNode->tail+1);
        }else{
            printf("        node->start:%p,         node->tail:%p\n",
                 treeNode->start, treeNode->tail);
            printf("existingNode->start:%p, existingNode->tail:%p\n",
                 existingNode->start, existingNode->tail);
            exit(-1);
        }
        nodeExist = addrInMemcpyTreeNode((void*)to, (void*)(to+size));
    }
	//tsearch(treeNode, &memcpyRecordTree, memcpyRecTreeInsDelCompare);
    addMemcpyTreeNode(treeNode);
}

VOID MallocAfter(ADDRINT ret){
    TraceFile << "  returns " << ret << endl;
}

void findImageAddIns(IMG img, const char* funName){
    // Instrument the malloc() and free() functions.  Print the input argument
    // of each malloc() or free(), and the return value of malloc().
    //
    //  Find the malloc() function.
    RTN memcpyRtn = RTN_FindByName(img, funName);
    if(memcpyRtn == RTN_Invalid()){
        printf("find rtn failed.\n");
    }

    if (RTN_Valid(memcpyRtn)){
        RTN_Open(memcpyRtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(memcpyRtn, IPOINT_BEFORE, (AFUNPTR)ArgBeforeExe,
                       IARG_ADDRINT, funName,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                       IARG_END);
        RTN_InsertCall(memcpyRtn, IPOINT_AFTER, (AFUNPTR)MallocAfter,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);

        RTN_Close(memcpyRtn);
    }else{
        printf("open rtn failed.\n");
    }
}

/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */
   
VOID Image(IMG img, VOID *v){
    findImageAddIns(img, MEMCPY);
}

VOID RecordMemRead(VOID * ip, VOID * addr){
    if(addrInMemcpyTreeNode(addr, addr)){
        printf("%p: R %p, is copied data.\n", ip, addr);
    }else{
        printf("%p: R %p, is not copied data.\n", ip, addr);
    }
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr){
    if(addrInMemcpyTreeNode(addr, addr)){
        printf("%p: W %p, is copied data.\n", ip, addr);
    }else{
        printf("%p: W %p, is not copied data.\n", ip, addr);
    }
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
    }
}
/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
    MSG("Pin tools memcpy call %ld times.\n", memcpyTime);
    MSG("Pin tools existing insert %ld times.\n", insertExistingTime);
 
    TraceFile.close();
    listRecTree();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */
   
INT32 Usage()
{
    cerr << "This tool produces a trace of calls to malloc." << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{

    memcpyRecordTree = NULL;
    memcpyTime = 0;
    insertExistingTime = 0;
    arrSize = 200;
    currNodeNum = 0;

    memcpyRecordArr = malloc(sizeof(struct memcpyNode) * arrSize);
    if(memcpyRecordArr == NULL){
        printf("malloc memcpyRecordArr ERROR.\n");
        exit(-1);
    }

    // Initialize pin & symbol manager
    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    // Write to a file since cout and cerr maybe closed by the application
    //printf("open para: %s\n", KnobOutputFile.Value().c_str());
    TraceFile.open(KnobOutputFile.Value().c_str());
    TraceFile << hex;
    TraceFile.setf(ios::showbase);

    INS_AddInstrumentFunction(Instruction, 0);
    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
