#! /bin/bash

#valueSize=(128 256 512 1024 4096 8192)
valueSize=(8192)
for s in "${valueSize[@]}"
do
	fioConfig="--name=job1 --size=2G --loops=1 --dedupe_percentage=40 --directory=/pmem/FIO"$s" --rw=randwrite --bs="$s
	set -o xtrace

	/home/xzjin/Documents/fio_modified/fio $fioConfig
	set +o xtrace
	echo; echo; 
done
