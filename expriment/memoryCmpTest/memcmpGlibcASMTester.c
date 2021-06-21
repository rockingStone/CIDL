#include <assert.h>
#include <fcntl.h>
#include <numa.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef char __u8;

//#define memSize 30      //1K
//#define memSize 1024      //1K
#define memSize 819200      //800K
//#define memSize 33554432    //32M
//536870912       //512M
//1073741824      //1G
//#define memSize 1073741824      //1G
//#define memSize 4294967296      //4G
#define TEST_NR 300
//#define memSize 687194767360    //10G 
static long get_nanos() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (long)ts.tv_sec * 1000000000L + ts.tv_nsec;
}

void printMem(void* mem, int lineNr){
		int i=0;
		for(;i<lineNr;i++){
			__u8* id = (__u8*)(mem+16*i);
			printf("%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
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

void printLine(void* mem, int charNum){
    int lineNr = charNum/16+((charNum)%16)?1:0;
    printMem(mem, lineNr);
}

int compareByteByByte(void* dest, void* src, int cmpSize, void** diffAddr){
    void* end = dest + cmpSize;
    for( ; dest<end; dest++, src++){
        if(*(char*)dest != *(char*)src){
            *diffAddr = dest;
            break;
        }
    }

    if(dest<end){
        return (*(char*)dest - *(char*)src);
    }
    return 0;
}

extern int memcmp_avx2_asm(const void *s1, const void *s2, size_t n, void* firstDiffPos);	
#define errExit(msg)    \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(void) {
    int fd, writefd;
    char * const dest = malloc(memSize);;
    char * const src = malloc(memSize);;
    char *destCmp, *srcCmp;
    char* diffAddr;
    int cmpSize, tmp, arraySize = 6;
    int cmpArray[] = {64, 128, 256, 1024, 4096, 8192};
	unsigned long long timeBegin, timeEnd;
    unsigned long long memcmpTotalTime[arraySize],
         byteByByteTotalTime[arraySize], writeTotalTime[arraySize];

    if (src <0 || dest <0)
        errExit("malloc\n");
    fd = open("./target.txt", O_RDONLY);
    assert(fd!= -1);
    writefd = open("/pmem/tmp.txt", O_RDWR);
    assert(writefd != -1);

    tmp = pread(fd, src, memSize, 0);
    assert(tmp!= -1);
    memcpy(dest, src, memSize);

//Clear cache before compare
//    tmp = cacheflush(dest, memSize, DCACHE);
//    assert(tmp ==0);
//    tmp = cacheflush(src, memSize, DCACHE);
//    assert(tmp ==0);
    for(int j=0;j<arraySize; j++){
        int cmpOffset;
        memcmpTotalTime[j] = 0;
        cmpSize = cmpArray[j];

        for(int i=0;i<100;i++){
            cmpOffset = i*cmpSize;
            timeBegin = get_nanos();
            int res = memcmp_avx2_asm(dest+cmpOffset, src+cmpOffset, cmpSize, &diffAddr);
            timeEnd = get_nanos();
            memcmpTotalTime[j] += (timeEnd-timeBegin);
            assert(res==0);
        }

    }

//Clear cache before compare
//    tmp = cacheflush(dest, memSize, DCACHE);
//    assert(tmp ==0);
//    tmp = cacheflush(src, memSize, DCACHE);
//    assert(tmp ==0);
    for(int j=0;j<arraySize;j++){
        int cmpOffset;
        byteByByteTotalTime[j] = 0;
        cmpSize = cmpArray[j];

        for(int i=0;i<100;i++){
            cmpOffset = i*cmpSize;
            timeBegin = get_nanos();
            int res =compareByteByByte(dest+cmpOffset, src+cmpOffset, cmpSize, (void**)(&diffAddr));
            timeEnd = get_nanos();
            byteByByteTotalTime[j] += (timeEnd-timeBegin);
            assert(res==0);
        }
    }

    for(int j=0;j<arraySize;j++){
        int cmpOffset;
        writeTotalTime[j] = 0;
        cmpSize = cmpArray[j];

        for(int i=0;i<100;i++){
            cmpOffset = i*cmpSize;
            timeBegin = get_nanos();
            int res = write(writefd, dest+cmpOffset, cmpSize);
            timeEnd = get_nanos();
            writeTotalTime[j] += (timeEnd-timeBegin);
            assert(res!=0);
        }
    }

    for(int j=0; j<arraySize; j++){
        printf("test size %4d,           memcmp time:%8lld\n", cmpArray[j], memcmpTotalTime[j]/100);
        printf("test size %4d, byte by byte cmp time:%8lld\n", cmpArray[j], byteByByteTotalTime[j]/100);
        printf("test size %4d,      write to PM time:%8lld\n\n", cmpArray[j], writeTotalTime[j]/100);
    }
    return 0;
}
