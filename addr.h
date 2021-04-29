#ifndef __AWN_ADDR__
#define __AWN_ADDR__

#include "common.h"

#define getAddr(pageNum, offset) \
	 ((void *)(((unsigned long long)pageNum<<PAGENUMSHIFT)+offset))

#define addr2PageNum(addr)    \
	 ((void *)((unsigned long long)addr>>PAGENUMSHIFT))

#define addr2PageBegin(addr)  \
	 ((void *)((unsigned long long)addr & PAGENUMMASK))

#define addr2PageTail(addr)   \
	 ((void *)((unsigned long long)(addr+PAGESIZE) & PAGENUMMASK))

#define addr2PageOffset(addr) \
	 ((short)((unsigned long long)(addr) & PAGEOFFMASK))

#endif //__AWN_ADDR__