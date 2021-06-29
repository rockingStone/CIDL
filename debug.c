#include "debug.h"
typedef unsigned char u8;

/** xzjin 16进制打印内存数据,每行16 byte
 * @mem 内存开始地址
 * @lineNr 打印的行数
*/
void printMem(void* mem, int lineNr){
		int i=0;
		for(;i<lineNr;i++){
			u8* id __attribute__((unused));
			id = (u8*)(mem+16*i);
			MSG("%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
				 *(u8*)((unsigned long long)id + 0), *(u8*)((unsigned long long)id + 1),
				 *(u8*)((unsigned long long)id + 2), *(u8*)((unsigned long long)id + 3),
				 *(u8*)((unsigned long long)id + 4), *(u8*)((unsigned long long)id + 5),
				 *(u8*)((unsigned long long)id + 6), *(u8*)((unsigned long long)id + 7),
				 *(u8*)((unsigned long long)id + 8), *(u8*)((unsigned long long)id + 9),
				 *(u8*)((unsigned long long)id + 10), *(u8*)((unsigned long long)id + 11),
				 *(u8*)((unsigned long long)id + 12), *(u8*)((unsigned long long)id + 13),
				 *(u8*)((unsigned long long)id + 14), *(u8*)((unsigned long long)id + 15));
		}
}
