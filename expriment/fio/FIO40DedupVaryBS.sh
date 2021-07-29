#! /bin/bash
# --num=5000000

#valueSize=(128 256 512 1024 4096 8192)
valueSize=(2048)
for s in "${valueSize[@]}"
do
	fioConfig="--name=job1 --size=2G --loops=1 --dedupe_percentage=40 --directory=/dbRepo/FIO"$s" --rw=randwrite --bs="$s
	set -o xtrace

	fio_modified $fioConfig
	set +o xtrace
	echo; echo; 
done
