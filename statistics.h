#ifndef __AWN_TIMER__
#define __AWN_TIMER__

#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>
#include "debug.h"

#define INSTRUMENT_CALLS 0

unsigned int num_memcpy_read;
unsigned int num_memcpy_write;
unsigned int num_mmap;
unsigned long long memcpy_read_size;
unsigned long long memcpy_write_size;
unsigned long long ts_write_size;
unsigned long long ts_write_not_found_size;
unsigned long long ts_write_same_size;
unsigned long ts_metadataItem;
//xzjin
long long able_to_reduce_size;
long long able_to_reduce_time;
unsigned long long total_syscalls;
unsigned long long deleted_size;

void print_statistics(void);

enum instrumentation_vars {
	appends_t,
	append_log_entry_t,
	append_log_reinit_t,
	bg_thread_t,
	clear_dr_t,
	clear_mmap_tbl_t,
	clf_lock_t,
	close_syscall_t,
	close_t,
	cmp_write_t,
	compare_mem_t,
	copy_appendread_t,
	copy_appendwrite_t,
	copy_to_dr_pool_t,
	copy_to_mmap_cache_t,
	copy_overread_t,
	copy_overwrite_t,
	dr_mem_queue_t,
	fileMap_t,
	fileUnmap_t,
	file_mmap_t,
	fsync_t,
	insert_tbl_mmap_t,
	insert_rec_t,
	get_dr_mmap_t,
	get_mmap_t,
	give_up_node_t,
	memcmp_asm_t,
	mem_from_file_trace_t,
	mem_from_mem_trace_t,
	node_lookup_lock_t,
	nvnode_lock_t,
	open_t,      
	op_log_entry_t,
	pread_t,
	pwrite_t,
	read_t,
	read_tbl_mmap_t,
	remap_mem_rec_t,
	remove_overlapping_entry_t,
	seek_t,
	swap_extents_t,
	tfind_t,
	ts_memcpy_tfind_file_t,
	ts_write_t,
	unlink_t,
	write_t,
	INSTRUMENT_NUM,
};

extern atomic_uint_least64_t Instrustats[INSTRUMENT_NUM];
extern const char *Instruprint[INSTRUMENT_NUM];
typedef struct timespec instrumentation_type;

#define INITIALIZE_TIMERS()					\
	{							\
		int i;						\
		for (i = 0; i < INSTRUMENT_NUM; i++)		\
			Instrustats[i] = 0;			\
	}							\

#if INSTRUMENT_CALLS 

#define START_TIMING(name, start)				\
	{							\
		clock_gettime(CLOCK_MONOTONIC, &start);		\
        }

#define END_TIMING(name, start)						\
	{                                                               \
		instrumentation_type end;				\
		clock_gettime(CLOCK_MONOTONIC, &end);			\
		__atomic_fetch_add(&Instrustats[name], (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec), __ATOMIC_SEQ_CST); \
        }

#define PRINT_TIME()							\
	{								\
		printf("\n");		\
		int i;							\
		for(i=0; i<INSTRUMENT_NUM; i++)	{			\
			if (Instrustats[i] != 0) 			\
				MSG("%s: timing = %lu us\n",		\
				    Instruprint[i], Instrustats[i]/1000);	\
		}							\
		if(Instrustats[ts_write_t]!=0){		\
			MSG("compare occupy ts_write: ");		\
			printf("%.2f%%\n",		\
				((double)Instrustats[compare_mem_t]*100/	\
				(Instrustats[ts_write_t])) );	\
		}	\
		printf("\n");		\
	}


#else

#define START_TIMING(name, start) {(void)(start);}
#define END_TIMING(name, start) {(void)(start);}
#define PRINT_TIME() {(void)(Instrustats[0]);}
	
#endif

#endif //__AWN_TIMER__