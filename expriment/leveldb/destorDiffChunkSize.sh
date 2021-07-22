#! /bin/bash
dbPath=/dbRepo/db4096/
#dbPath=/dbRepo/db128/
dedupPath=/pmem/dedupDir
chunkSize=(512 1024 2048 4096 8192)
output=""
for s in "${chunkSize[@]}"
do
	#clean path
	set -o xtrace
	$dedupPath/rebuild

	destor $dbPath -p"chunk-avg-size $s" 
#chunk-avg-size 4096
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
