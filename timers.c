#include "nv_common.h"
#include "timers.h"

void nvp_print_io_stats(void)
{
	MSG("====================== NVP IO stats: ======================\n");
	MSG("open %u, close %u, async close %u\n", num_open, num_close, num_async_close);
	MSG("mmap %u, unlink %u, stat %u\n", num_mmap, num_unlink, num_stat);
	MSG("dr mmap %u, dr mmap critical path %u\n", num_drs, num_drs_critical_path);
	MSG("fsync %u, appendfsync: count %u size %llu average %llu\n",
	    num_fsync, num_appendfsync, appendfsync_size,
	    num_appendfsync ? appendfsync_size / num_appendfsync : 0);
	MSG("READ: count %u, size %llu, average %llu\n", num_read,
		read_size, num_read ? read_size / num_read : 0);
	MSG("WRITE: count %u, size %llu, average %llu\n", num_write,
		write_size, num_write ? write_size / num_write : 0);
	MSG("memcpy READ: count %u, size %llu, average %llu\n",
		num_memcpy_read, memcpy_read_size,
		num_memcpy_read ? memcpy_read_size / num_memcpy_read : 0);
	MSG("anon READ: count %u, size %llu, average %llu\n",
		num_anon_read, anon_read_size,
		num_anon_read ? anon_read_size / num_anon_read : 0);
	MSG("memcpy WRITE: count %u, size %llu, average %llu\n",
		num_memcpy_write, memcpy_write_size,
		num_memcpy_write ? memcpy_write_size / num_memcpy_write : 0);
	MSG("anon WRITE: count %u, size %llu, average %llu\n",
		num_append_write, append_write_size,
		num_append_write ? append_write_size / num_append_write : 0);
	MSG("posix READ: count %u, size %llu, average %llu\n",
		num_posix_read, posix_read_size,
		num_posix_read ? posix_read_size / num_posix_read : 0);
	MSG("posix WRITE: count %u, size %llu, average %llu\n",
		num_posix_write, posix_write_size,
		num_posix_write ? posix_write_size / num_posix_write : 0);
	MSG("write extends %lu, total %lu\n", _nvp_wr_extended,
		_nvp_wr_total);
	MSG("MFENCE: count %u\n",
	       num_mfence);
	MSG("CLFLUSHOPT: count %u\n",
	       num_clflushopt);
	MSG("NON_TEMPORAL_WRITES: count %u, size %llu, average %llu\n",
	       num_write_nontemporal, non_temporal_write_size,
	       num_write_nontemporal ? non_temporal_write_size / num_write_nontemporal : 0);
	MSG("TEMPORAL WRITES: count %u, size %llu, average %llu\n",
	       num_write_temporal, temporal_write_size,
	       num_write_temporal ? temporal_write_size / num_write_temporal : 0);
	MSG("TOTAL SYSCALLS (open + close + read + write + fsync): count %llu\n",
	       num_open + num_close + num_posix_read + num_posix_write);

	MSG(" RECTREENODEPOOLIDX:%d\n", RECTREENODEPOOLIDX);
	//MSG(" SLISTHEADPOOLIDX:%d\n", SLISTHEADPOOLIDX);
	MSG(" TAILHEADPOOLIDX:%d\n", TAILHEADPOOLIDX);
	MSG(" RECBLOCKENTRYPOOLIDX:%d\n", RECBLOCKENTRYPOOLIDX);
	MSG(" RECARRPOOLIDX:%d\n", RECARRPOOLIDX);
	MSG(" nvp_write bytes :%llu\n", nvpWriteSize);
	MSG(" ts_write bytes :%llu\n", ts_write_size);
	float tmp = ((double)ts_write_same_size/ts_write_size)*100;
	MSG(" ts_write same bytes :%llu, ", ts_write_same_size);
	printf("occupy %.2f%%\n",tmp);
	MSG(" ts_write new metadata item :%lu, ", ts_metadataItem);
	printf("metadata occupy %.2f%% write data.\n",(double)(ts_metadataItem*32*100)/ts_write_size);
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
