#! /bin/bash
# --num=5000000

dbBash=/pmem/ 
valueSize=(128 256 512 1024 2048 4096 8192)
#valueSize=(128)
#chunkMethod=("normalized rabin" "file" "fixed" "fastcdc" "tttd" "ae" "rabin")
chunkMethod=("fixed" "tttd" "ae" "rabin" "fastcdc" )
blockSize="4096"
#blockSize="32768"

function runDB(){
	for s in "${valueSize[@]}"
	do
		#clean path
		dbPath="${dbBash}db${s}/"
		echo "dbPath:"$dbPath
	
	#	find $dbPath -type f -exec rm {} \;
		set -o xtrace
		time /home/xzjin/leveldb_modified/build/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath --block_size=$blockSize 
		set +o xtrace
		du -sb $dbPath
		echo; echo; 
	done
}

cd /home/xzjin/awn/
make base
sudo make install
echo "base version"
runDB
