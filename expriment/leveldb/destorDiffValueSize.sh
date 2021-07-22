#! /bin/bash
dbPathBase=/dbRepo/
dedupPath=/pmem/dedupDir
chunkSize=(128 256 512 1024 4096 8192)
dbPath=""
for s in "${chunkSize[@]}"
do
	#clean path
	set -o xtrace
	$dedupPath/rebuild
	dbPath="$dbPathBase/db$s/"

	destor $dbPath -p"chunk-algorithm fastcdc"
	du -sb $dedupPath/*
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
