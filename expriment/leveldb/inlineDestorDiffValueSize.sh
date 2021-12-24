#! /bin/bash
# --num=5000000

dbBash=/pmem/ 
dedupPath=/pmem/dedupDir
#valueSize=(128 256 512 1024 2048 4096 8192)
valueSize=(1024)
output=""
for s in "${valueSize[@]}"
do
	#clean path
	dbPath="${dbBash}db${s}/"
	set -o xtrace
	find $dbPath -type f -exec rm {} \;

#	time /pmem/benchmark/db_bench_modified --benchmarks=fillrandom --value_size=$s --db=$dbPath
	time LD_PRELOAD=/home/xzjin/Documents/destor/src/.libs/libentrance.so.0.0.0  /home/xzjin/leveldb/build/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath
	find $dbPath -type f -exec rm {} \;
	time /home/xzjin/leveldb_modified/build/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath
	du -sb $dbPath
#	time /usr/local/bin/destor $dbPath -p"chunk-algorithm fastcdc" 
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
