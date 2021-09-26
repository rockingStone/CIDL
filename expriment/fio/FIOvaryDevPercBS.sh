#! /bin/bash

dbBash=/dbRepo/ 
dedupPath=/pmem/dedupDir/
#directory=("/home/xzjin/tmp/FIO" "/dbtmp/FIO/" )
directory=("/dmdedup/FIO" )
blockSize=(128 256 512 1024 2048 4096 8192)
randomPer=(0 20 40 60 80 100)
output=""
size=0
#valueSize=(128 256 )
for dir in "${directory[@]}"
do
	if grep -q "dbtmp" <<< "$dir"; then
		size=8G
	else
		size=200M
	fi
	for s in "${blockSize[@]}"
	do
		for perc in "${randomPer[@]}"
		do
			set -o xtrace
			fio --name=job1 --rw=randread --size=$size --bs=$s --directory=$dir --percentage_random=$perc
			echo; echo; 
			set +o xtrace
		done
	done
	rm -i $dir"/*"
done
