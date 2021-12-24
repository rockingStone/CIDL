#! /bin/bash
dbPathBase=/dbRepo/
dedupPath=/pmem/dedupDir
#chunkSize=(128 256 512 1024 4096 8192)
chunkSize=(4096)
#chunkMethod=("'normalized rabin'")
chunkMethod=(fixed tttd ae rabin fastcdc)
dbPath=""
for s in "${chunkSize[@]}"
do
#	dbPath="$dbPathBase/db$s/"
	dbPath="/dedupRepo/db8193"

	for cm in "${chunkMethod[@]}"
	do
		#clean path
		set -o xtrace
		$dedupPath/rebuild
	
#		destor $dbPath -p"chunk-algorithm fastcdc"
		destor $dbPath -p"chunk-algorithm $cm"
		du -sb $dedupPath/*
		set +o xtrace
		echo; echo; 
	done
done
#destor /home/xzjin/dbtmp -p"chunk-algorithm 'normalized rabin'"
#destor /home/xzjin/dbtmp -p"chunk-algorithm file"
#destor /home/xzjin/dbtmp -p"chunk-algorithm fixed"
#destor /home/xzjin/dbtmp -p"chunk-algorithm tttd"
#destor /home/xzjin/dbtmp -p"chunk-algorithm ae"
#destor /home/xzjin/dbtmp -p"chunk-algorithm rabin"
#destor /home/xzjin/dbtmp -p"chunk-algorithm fastcdc"



