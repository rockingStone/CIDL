#! /bin/bash
dedupPath=/pmem/dedupDir
# NO 2048
blockSize=(128 256 512 1024 2048 4096 8192)
chunkMethod=(fixed tttd ae rabin fastcdc)

for s in "${blockSize[@]}"
do
	for cm in "${chunkMethod[@]}"
	do

		#clean path
		set -o xtrace
		$dedupPath/rebuild
	
		destor "/dbRepo/FIO"$s -p"chunk-algorithm $cm" -p"chunk-avg-size 4096"
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
#destor /pmem/dbtmp -p"chunk-algorithm fastcdc"
