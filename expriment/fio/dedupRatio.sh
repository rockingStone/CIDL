#! /bin/bash
# --num=5000000

dbBash=/dbRepo/ 
dedupPath=/pmem/dedupDir/
configPath=/home/xzjin/awn/expriment/fio/
fioFilePath="/dbtmp/SSD-rand.0.0"
valueSize=(128 256 512 1024 4096 8192)
output=""
#valueSize=(128 256 )
for s in "${valueSize[@]}"
do
	#clean path
	fioConfig="${configPath}dedupRatio-${s}.fio"
	set -o xtrace
	$dedupPath/rebuild

	fio_modified $fioConfig
	destor $fioFilePath -p"chunk-algorithm fastcdc" 
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
