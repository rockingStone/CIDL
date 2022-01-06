#! /bin/bash

dbBash=/pmem/ 
#valueSize=(128 256 512 1024 2048 4096 8192)
valueSize=(1024)
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
#		time LD_PRELOAD=/home/xzjin/awn/libawn.so /home/xzjin/leveldb_modified/build/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath --block_size=$blockSize 
		time /home/xzjin/leveldb_modified/build/db_bench --benchmarks=fillrandom --value_size=$s --db=$dbPath --block_size=$blockSize 
		set +o xtrace
		du -sb $dbPath
		echo; echo; 
	done
}

function base(){
	cd /home/xzjin/awn/
	make base
	sudo make install
	cd /home/xzjin/awn/expriment/leveldb
	echo "base version"
	runDB
}

function AVX(){
	cd /home/xzjin/awn/
	make pg_threelevel_delNode
	sudo make install
	cd /home/xzjin/awn/expriment/leveldb
	echo "disable AVX-2"
	runDB
}

function normal(){
	cd /home/xzjin/awn/
	make 
	sudo make install
	cd /home/xzjin/awn/expriment/leveldb
	echo "disable AVX-2"
	runDB
}

base
AVX
normal
