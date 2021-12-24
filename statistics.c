#include "common.h"
#include "statistics.h"
#include "tree.h"

void ts_print_statistics(void){
	float tmp;
	unsigned long usedMemory __attribute__((unused));

	MSG("====================== IO statistics: ======================\n");
	MSG(" mmap %u\n", num_mmap);
	MSG(" memcpy READ: count %u, size %llu, average %llu\n",
		num_memcpy_read, memcpy_read_size,
		num_memcpy_read ? memcpy_read_size / num_memcpy_read : 0);
	MSG(" memcpy WRITE: count %u, size %llu, average %llu\n",
		num_memcpy_write, memcpy_write_size,
		num_memcpy_write ? memcpy_write_size / num_memcpy_write : 0);

	MSG(" RECTREENODEPOOLIDX:%d\n", RECTREENODEPOOLIDX);
	//MSG(" SLISTHEADPOOLIDX:%d\n", SLISTHEADPOOLIDX);
	MSG(" TAILHEADPOOLIDX:%d\n", TAILHEADPOOLIDX);
	MSG(" RECBLOCKENTRYPOOLIDX:%d\n", RECBLOCKENTRYPOOLIDX);
	MSG(" RECARRPOOLIDX:%d\n", RECARRPOOLIDX);
	usedMemory =(( RECTREENODEPOOLSIZE - RECTREENODEPOOLIDX)*(sizeof(struct recTreeNode*)+sizeof(struct recTreeNode))) +
		 (( TAILHEADPOOLSIZE - TAILHEADPOOLIDX)*(sizeof(struct tailhead*)+sizeof(struct tailhead))) +
		 (( RECBLOCKENTRYPOOLSIZE - RECBLOCKENTRYPOOLIDX)*(sizeof(struct recBlockEntry*)+sizeof(struct recBlockEntry))) +
		 (( RECARRPOOLSIZE - RECARRPOOLIDX)*(sizeof(struct memRec*)+sizeof(struct memRec)));
	MSG(" Really used memory:%lu\n", usedMemory);
	MSG(" Total allocated memory:%llu\n", totalAllocSize);
 	UNBLOCKABLE_MSG(" ts_write bytes :%llu\n", ts_write_size);
	tmp = ((double)ts_write_same_size/ts_write_size)*100;
 	UNBLOCKABLE_MSG(" ts_write same bytes :%llu, occupy %.2f%%\n", ts_write_same_size,  tmp);
	MSG("ts_write_same_size addr :%p, pid:%d, ppid:%d\n", &ts_write_same_size, getpid(), getppid());
	tmp = ((double)ts_write_not_found_size/ts_write_size)*100;
	MSG(" ts_write not found bytes :%llu, occupy %.2f%%\n", ts_write_not_found_size, tmp);
	tmp = (double)(ts_metadataItem*32*100)/ts_write_size;
	MSG(" ts_write new metadata item :%lu, metadata occupy %.2f%% write data.\n", ts_metadataItem, tmp);
}

const char *Instruprint[INSTRUMENT_NUM] = {
	"appends_t",
	"append_log_entry_t",
	"append_log_reinit_t",
	"bg_thread_t",
	"clear_dr_t",
	"clear_mmap_tbl_t",
	"clf_lock_t",
	"close_syscall_t",
	"close_t",
	"cmp_write_t",
	"compare_mem_t",
	"copy_appendread_t",
	"copy_appendwrite_t",
	"copy_to_dr_pool_t",
	"copy_to_mmap_cache_t",
	"copy_overread_t",
	"copy_overwrite_t",
	"dr_mem_queue_t",
	"fileMap_t",
	"fileUnmap_t",
	"file_mmap_t",
	"fsync_t", 
	"insert_tbl_mmap_t",
	"insert_rec_t",
	"get_dr_mmap_t",
	"get_mmap_t",
	"give_up_node_t",
	"memcmp_asm_t",
	"mem_from_file_trace_t",
	"mem_from_mem_trace_t",
	"memcpy_from_mapped_trace_t",
	"node_lookup_lock_t",
	"nvnode_lock_t",
	"open_t",
	"op_log_entry_t",
	"pread_t",
	"pwrite_t",
	"read_t",
	"read_tbl_mmap_t",
	"remap_mem_rec_t",
	"remove_overlapping_entry_t",
	"seek_t",
	"swap_extents_t",
	"tfind_t",
	"ts_memcpy_tfind_file_t",
	"ts_write_t",
	"unlink_t",
	"write_t",
};
