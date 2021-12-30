#! /bin/bash
chunkMethod=("fixed" "tttd" "ae" "rabin" "fastcdc" )

for c in "${chunkMethod[@]}"
do
	newline="chunk-algorithm $c"
	gawk -i inplace "/^chunk-algorithm/{gsub(\"^chunk-algorithm[a-zA-Z ]*\" "," \"$newline\")};{print}" destor.config
	set -o xtrace
	time LD_PRELOAD=/home/xzjin/Documents/destor/src/.libs/libentrance.so.0.0.0 /home/xzjin/Documents/ffmpeg/ffmpeg -y -i ./Norah.avi -ss 00:00:00 -t 00:00:50 -c copy Norah_first.avi
	set +o xtrace
	echo; echo;
done
