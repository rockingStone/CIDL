#!/bin/bash

category=(128 256 512 1024 2048 4096 8192)
column=("FSC" "TTTD" "AE" "RABIN" "FastCDC" "CIDL")
#use array?
#lineNum="$(grep -n "needle" haystack.txt | head -n 1 | cut -d: -f1)"

for a in "${category[@]}"
do
	echo -n > $a.csv
	chunkMethods=" "
	bigTitle=$a
	echo $bigTitle
	for c in "${column[@]}"
	do
		chunkMethods=$chunkMethods","$c
		bigTitle=$bigTitle","
	done
	echo $bigTitle >> $a.csv
	echo $chunkMethods >> $a.csv
	
	set -o xtrace
	fileName="inlineDestorDiffValueSize.sh_log_4K_fullIndex"
	awk "{if(FNR>=1 && FNR<226) {printf(\"%s\n\", \$0)} }" $fileName > tmp/filteredData
	grep -e "new metadata item" -e "hash number" -e "--value_size=" -e "MB/s" -e "real" -e "ts_write same bytes" -e "duplicate percentage" -e "chunck algorithm" tmp/filteredData > tmp/grepedData
	
	#Throughput
	awk "BEGIN{line=\"throuthput\"} /^fillrandom/{line=line\",\"\$(NF-1) } END{print line}" tmp/grepedData >> $a.csv
	
	#DedupRatio
	awk "/duplicate percentage/{print \$(NF)} /ts_write same bytes/{print \$(NF)}" tmp/grepedData > tmp/DedepRatio
	awk -F"%" "BEGIN{line=\"dedupRatio\"} {line=line\",\"\$1 } END{print line}" tmp/DedepRatio >> $a.csv
	
	#Time
	awk "/real/{print \$(NF)}" tmp/grepedData > tmp/Time
	awk -F"[ms]" "BEGIN{line=\"time\"} {time=\$1*60+\$2; line=line\",\"time} END{print line}" tmp/Time >> $a.csv
	
	#hash/meta num
	awk "BEGIN{line=\"hash/meta num\"} /hash number/{line=line\",\"\$(NF)} /new metadata item/{metaNum=substr(\$7, 2, length(\$7)-2); line=line\",\"metaNum} END{print line}" tmp/grepedData >> $a.csv

done

#merge files
for a in "${category[@]}"
do
	mergePara=$mergePara$a".csv "
done
paste -d"," $mergePara > 4K.csv
