#! /bin/bash
# --num=5000000

dbBash=/dbRepo/ 
dedupPath=/pmem/dedupDir
valueSize=(128 256 512 1024 4096 8192)
output=""
#valueSize=(128 256 )
for s in "${valueSize[@]}"
do
	#clean path
	dbPath="${dbBash}db${s}/"
	set -o xtrace
	find $dbPath -type f -exec rm {} \;
	$dedupPath/rebuild

	time /pmem/benchmark/db_bench_modified --benchmarks=fillrandom --value_size=$s --db=$dbPath
#	time /pmem/benchmark/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath
# 2>&1 | grep ts_write
	#/pmem/benchmark/db_bench_modified --benchmarks=fillrandom --value_size=$s --db=$dbPath 2>&1
	du -sb $dbPath
	/usr/local/bin/destor $dbPath -p"chunk-algorithm fastcdc" 
#| grep -e "Dedup time" -e "deduplication ratio" -e "job[[:space:]]+" -e "Total time"
	set +o xtrace
	echo; echo; 
done
#destor /home/xzjin/dbtmp -p"chunk-algorithm 'normalized rabin'"
#destor /home/xzjin/dbtmp -p"chunk-algorithm file"
#destor /home/xzjin/dbtmp -p"chunk-algorithm fixed"
#destor /home/xzjin/dbtmp -p"chunk-algorithm tttd"
#destor /home/xzjin/dbtmp -p"chunk-algorithm ae"
#destor /home/xzjin/dbtmp -p"chunk-algorithm rabin"
#destor /pmem/dbtmp -p"chunk-algorithm fastcdc"
