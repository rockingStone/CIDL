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
#include <iostream>
#include <fstream>
#include <search.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "pin.H"
using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

ofstream OutFile;

#define TRLASTCPY
// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;
static UINT64 memcpyTime = 0;
static UINT64 inTreeCount = 0;
static UINT64 memReadInRec = 0;
unsigned long beginAddr = 0;
unsigned long endAddr = 0;
void *recTreeRoot;

//static unsigned long MemoryRead = 0;
//static unsigned long MemoryWrite = 0;
//static unsigned long Lea = 0;
//static unsigned long Nop = 0;
//static unsigned long Branch = 0;
//static unsigned long DirectBranch = 0;
//static unsigned long DirectCall = 0;
//static unsigned long DirectControlFlow = 0;
//static unsigned long Call = 0;
//static unsigned long ControlFlow = 0;
//static unsigned long ValidForIpointAfter = 0;
//static unsigned long ValidForIpointTakenBranch = 0;
//static unsigned long ProcedureCall = 0;
//static unsigned long Ret = 0;
//static unsigned long Sysret = 0;
//static unsigned long Prefetch = 0;
//static unsigned long AtomicUpdate = 0;
//static unsigned long IndirectControlFlow = 0;
//static unsigned long StackRead = 0;
//static unsigned long StackWrite = 0;
//static unsigned long IpRelRead = 0;
//static unsigned long IpRelWrite = 0;
//static unsigned long Predicated = 0;
//static unsigned long Original = 0;
//static unsigned long Syscall = 0;

struct memRec{
    unsigned long UID;
    unsigned long beginAddr;
    unsigned long endAddr;
};

int recCompare(const void *pa, const void *pb);
void showRecMapNodeDetail(struct memRec** nodep);
void listRecTreeDetail();
struct memRec tmpRec;
// This function is called before every block
// Use the fast linkage for calls
VOID PIN_FAST_ANALYSIS_CALL docount(ADDRINT c) { icount += c; }
    
// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID *v) {
    // Visit every basic block  in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        // Insert a call to docount for every bbl, passing the number of instructions.
        // IPOINT_ANYWHERE allows Pin to schedule the call anywhere in the bbl to obtain best performance.
        // Use a fast linkage for the call.
        BBL_InsertCall(bbl, IPOINT_ANYWHERE, AFUNPTR(docount), IARG_FAST_ANALYSIS_CALL, IARG_UINT32, BBL_NumIns(bbl), IARG_END);
    }
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "inscount.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v){
    listRecTreeDetail();
    // Write to a file since cout and cerr maybe closed by the application
    OutFile.setf(ios::showbase);
    OutFile << "Count " << icount << endl;
    OutFile << "Memcpy " << memcpyTime << endl;
    OutFile << "inTreeCount " << inTreeCount << endl;
    OutFile << "memReadInRec " << memReadInRec << endl;

//    OutFile << "MemoryRead Num: " << MemoryRead << endl;
//    OutFile << "MemoryWrite Num: " << MemoryWrite << endl;
//    OutFile << "Lea Num: " << Lea << endl;
//    OutFile << "Nop Num: " << Nop << endl;
//    OutFile << "Branch Num: " << Branch << endl;
//    OutFile << "DirectBranch Num: " << DirectBranch << endl;
//    OutFile << "DirectCall Num: " << DirectCall << endl;
//    OutFile << "DirectControlFlow Num: " << DirectControlFlow << endl;
//    OutFile << "Call Num: " << Call << endl;
//    OutFile << "ControlFlow Num: " << ControlFlow << endl;
//    OutFile << "ValidForIpointAfter Num: " << ValidForIpointAfter << endl;
//    OutFile << "ValidForIpointTakenBranch Num: " << ValidForIpointTakenBranch << endl;
//    OutFile << "ProcedureCall Num: " << ProcedureCall << endl;
//    OutFile << "Ret Num: " << Ret << endl;
//    OutFile << "Sysret Num: " << Sysret << endl;
//    OutFile << "Prefetch Num: " << Prefetch << endl;
//    OutFile << "AtomicUpdate Num: " << AtomicUpdate << endl;
//    OutFile << "IndirectControlFlow Num: " << IndirectControlFlow << endl;
//    OutFile << "StackRead Num: " << StackRead << endl;
//    OutFile << "StackWrite Num: " << StackWrite << endl;
//    OutFile << "IpRelRead Num: " << IpRelRead << endl;
//    OutFile << "IpRelWrite Num: " << IpRelWrite << endl;
//    OutFile << "Predicated Num: " << Predicated << endl;
//    OutFile << "Original Num: " << Original << endl;
//    OutFile << "Syscall Num: " << Syscall << endl;

    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */
   
#define MEMCPYPTL "memcpy@plt"
#define MEMCPY "memcpy"

/** xzjin compare relationship			
 * a.			|_________|			  a ? b
 * b.1 	|___|							>
 * b.2 	|____________|					=
 * b.3 	|__________________________|	=
 * b.4 				|___|				=
 * b.5 				|______________|	=
 * b.6 						|___|		<
*/
int overlapRec(const void *pa, const void *pb){
    const struct memRec* a = (struct memRec*)pa;
    const struct memRec* b = (struct memRec*)pb;
    if ((a->beginAddr) > (b->beginAddr)&&
            (a->beginAddr) > (b->endAddr))
        return 1;
    if ((a->beginAddr) < (b->beginAddr)&&
            (a->endAddr) < (b->beginAddr))
        return -1;
    return 0;
}

int recContainsIn(const void *pa, const void *pb) {
    const struct memRec* a = (struct memRec*)pa;
    const struct memRec* b = (struct memRec*)pb;
    if (a->beginAddr < b->beginAddr)
        return -1;
    if (a->beginAddr >= b->beginAddr &&
        a->beginAddr <= b->endAddr)
        return 0;
    return 1;
}

void showRecMapNodeDetail(struct memRec** nodep){
    struct  memRec* rec;
	rec = *(struct memRec**) nodep;
    LOG("UID: "+decstr(rec->UID)+", memcpy begin address: "+hexstr(rec->beginAddr)+", tail address: "+hexstr(rec->endAddr)+".\n");
}

void listRecMapActionDetail(const void *nodep, const VISIT which, const int depth){

    switch (which) {
    case preorder:
        break;
    case postorder:
		showRecMapNodeDetail((struct memRec**)nodep);
        break;
    case endorder:
        break;
    case leaf:
		showRecMapNodeDetail((struct memRec**)nodep);
        break;
    }
}

void listRecTreeDetail(){
	twalk(recTreeRoot, listRecMapActionDetail);
}

VOID MemcpyBefore(CHAR * name, ADDRINT from, ADDRINT to, ADDRINT size){
    memcpyTime++;
#ifdef  TRLASTCPY
    beginAddr = (unsigned long)to;
    endAddr = (unsigned long)to + (unsigned long)size;
    LOG(hexstr(beginAddr)+" - "+hexstr(endAddr)+"\n");
#else
    if(memcpyTime>1000) return;
//    printf("memcopy from %p, to %p, length: %lu.\n", (void*)from, (void*)to, size);
//    LOG("memcopy from: "+ hexstr(from) +", to: "+ hexstr(to) +" ,length:" + decstr(size)+".\n");
    int err;
    struct memRec *rec;
    struct memRec **searchRes;


    tmpRec.UID = memcpyTime;
    tmpRec.beginAddr = to;
    tmpRec.endAddr = to+size;

    searchRes = (struct memRec**)tfind(&tmpRec, &recTreeRoot, overlapRec);
    if(searchRes){  //conflict, update existing node
        rec = *searchRes;
        rec->UID = memcpyTime;
        rec->beginAddr = to;
        rec->endAddr = to+size;
    }else{  //not exist, add new node
        rec = (struct memRec*)malloc(sizeof(struct memRec));
        err = errno;
        if(!rec){
            LOG("alloc Mem error.");
            LOG(strerror(err));
            exit(-1);
        }
        rec->UID = memcpyTime;
        rec->beginAddr = to;
        rec->endAddr = to+size;

        tsearch(rec, &recTreeRoot, overlapRec);
        inTreeCount++;
    }
#endif //TRLASTCPY
}

void findImageAddIns(IMG img, const char* funName){
    // Instrument the malloc() and free() functions.  Print the input argument
    // of each malloc() or free(), and the return value of malloc().
    //
    RTN memcpyRtn = RTN_FindByName(img, funName);

    if (RTN_Valid(memcpyRtn)){
        RTN_Open(memcpyRtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(memcpyRtn, IPOINT_BEFORE, (AFUNPTR)MemcpyBefore,
                       IARG_ADDRINT, funName,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                       IARG_END);

        RTN_Close(memcpyRtn);
    }
}

VOID Image(IMG img, VOID *v){
    findImageAddIns(img, MEMCPY);
    findImageAddIns(img, MEMCPYPTL);
}

VOID RecordMemRead(VOID * ip, VOID * addr){
#ifdef  TRLASTCPY
    //struct timespec start;
    unsigned long memAddress = (unsigned long)addr;
    if(beginAddr<=memAddress && memAddress<=endAddr){
        memReadInRec++;
		//clock_gettime(CLOCK_MONOTONIC, &start);
        //LOG(hexstr(memAddress)+" "+decstr((start.tv_sec)%100)+"."+decstr(start.tv_nsec)+"\n");
        LOG(hexstr(memAddress)+"\n");
    }
#else
    tmpRec.UID = 0;
    tmpRec.beginAddr = (unsigned long)addr;
    tmpRec.endAddr = 0;

    if(tfind(&tmpRec, &recTreeRoot, recContainsIn)){
        memReadInRec++;
    }
#endif //TRLASTCPY
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v) {
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);

//    if(INS_IsMemoryRead (ins)){
//        MemoryRead++;
//    }else if(INS_IsMemoryWrite (ins)){
//        MemoryWrite++;
//    }else if(INS_IsLea (ins)){
//        Lea++;
//    }else if(INS_IsNop (ins)){
//        Nop++;
//    }else if(INS_IsBranch (ins)){
//        Branch++;
//    }else if(INS_IsDirectBranch (ins)){
//        DirectBranch++;
//    }else if(INS_IsDirectCall (ins)){
//        DirectCall++;
//    }else if(INS_IsDirectControlFlow (ins)){
//        DirectControlFlow++;
//    }else if(INS_IsCall (ins)){
//        Call++;
//    }else if(INS_IsControlFlow (ins)){
//        ControlFlow++;
//    }else if(INS_IsValidForIpointAfter (ins)){
//        ValidForIpointAfter++;
//    }else if(INS_IsValidForIpointTakenBranch (ins)){
//        ValidForIpointTakenBranch++;
//    }else if(INS_IsProcedureCall (ins)){
//        ProcedureCall++;
//    }else if(INS_IsRet (ins)){
//        Ret++;
//    }else if(INS_IsSysret (ins)){
//        Sysret++;
//    }else if(INS_IsPrefetch (ins)){
//        Prefetch++;
//    }else if(INS_IsAtomicUpdate (ins)){
//        AtomicUpdate++;
//    }else if(INS_IsIndirectControlFlow (ins)){
//        IndirectControlFlow++;
//    }else if(INS_IsStackRead (ins)){
//        StackRead++;
//    }else if(INS_IsStackWrite (ins)){
//        StackWrite++;
//    }else if(INS_IsIpRelRead (ins)){
//        IpRelRead++;
//    }else if(INS_IsIpRelWrite (ins)){
//        IpRelWrite++;
//    }else if(INS_IsPredicated (ins)){
//        Predicated++;
//    }else if(INS_IsOriginal (ins)){
//        Original++;
//    }else if(INS_IsSyscall (ins)){
//        Syscall++;
//    }
    
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
    }

//    if(INS_IsMemoryRead(ins)){
//
//    }
}
/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[]){

    recTreeRoot = NULL;
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    PIN_InitSymbols();
    OutFile.open(KnobOutputFile.Value().c_str());

    INS_AddInstrumentFunction(Instruction, 0);
    // Register Instruction to be called to instrument instructions
    TRACE_AddInstrumentFunction(Trace, 0);

    IMG_AddInstrumentFunction(Image, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}