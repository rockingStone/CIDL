#ifndef __MEMCPYTRACE__
#define __MEMCPYTRACE__
struct memcpyNode{
	void* start;	//memcpy目的地的开始
	void* tail;		//memcpy目的地的结束
};
#endif  //__MEMCPYTRACE__