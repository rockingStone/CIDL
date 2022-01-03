#! /bin/bash
# --num=5000000

dbBash=/pmem/ 
dedupPath=/pmem/dedupDir
valueSize=(128 256 512 1024 2048 4096 8192)
#valueSize=(128)
#chunkMethod=("normalized rabin" "file" "fixed" "fastcdc" "tttd" "ae" "rabin")
chunkMethod=("fixed" "tttd" "ae" "rabin" "fastcdc" )
#blockSize="4096"
blockSize="32768"

output=""
for s in "${valueSize[@]}"
do
	#clean path
	dbPath="${dbBash}db${s}/"

	for c in "${chunkMethod[@]}"
	do
		find $dbPath -type f -exec rm {} \;
		newline="chunk-algorithm $c"
		gawk -i inplace "/^chunk-algorithm/{gsub(\"^chunk-algorithm[a-zA-Z ]*\" "," \"$newline\")};{print}" destor.config
		set -o xtrace
		time LD_PRELOAD=/home/xzjin/Documents/destor/src/.libs/libentrance.so.0.0.0  /home/xzjin/leveldb/build/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath --block_size=$blockSize 
		du -sb $dbPath
		set +o xtrace
		echo; echo;
	done

	find $dbPath -type f -exec rm {} \;
	set -o xtrace
	time /home/xzjin/leveldb_modified/build/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath --block_size=$blockSize 
	set +o xtrace
	du -sb $dbPath
#	time /usr/local/bin/destor $dbPath -p"chunk-algorithm fastcdc" 
	echo; echo; 
done
