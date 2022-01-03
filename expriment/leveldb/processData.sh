#!/bin/bash

#stop on error
set -e

category=(128 256 512 1024 2048 4096 8192)
column=("FSC" "TTTD" "AE" "RABIN" "FastCDC" "CIDL")
#For 4K
startLineNum=(1 229 457 687 923 1175 1467)
endLineNum=(226 455 685 921 1173 1465 1841)
output=4K.CSV
fileName="inlineDestorDiffValueSize.sh_log_4K_fullIndex"

#For 32K
#startLineNum=(1 228 456 685 915 1145 1375)
#endLineNum=(227 455 684 914 1144 1374 1604)
#output=32K.CSV
#fileName="inlineDestorDiffValueSize.sh_log_32K_fullIndex"

mkdir -p ./tmp/

for a in "${!category[@]}"
do
	echo -n > ${category[a]}.csv
	chunkMethods=" "
	bigTitle=${category[a]}
	echo $bigTitle
	for c in "${column[@]}"
	do
		chunkMethods=$chunkMethods","$c
		bigTitle=$bigTitle","
	done
	echo $bigTitle >> ${category[a]}.csv
	echo $chunkMethods >> ${category[a]}.csv
	
	set -o xtrace
	awk "{if(FNR>=${startLineNum[a]} && FNR<${endLineNum[a]}) {printf(\"%s\n\", \$0)} }" $fileName > tmp/filteredData
	grep -e "new metadata item" -e "hash number" -e "--value_size=" -e "MB/s" -e "real" -e "ts_write same bytes" -e "duplicate percentage" -e "chunck algorithm" tmp/filteredData > tmp/grepedData
	
	#Throughput
	awk "BEGIN{line=\"throuthput\"} /^fillrandom/{line=line\",\"\$(NF-1) } END{print line}" tmp/grepedData >> ${category[a]}.csv
	
	#DedupRatio
	awk "/duplicate percentage/{print \$(NF)} /ts_write same bytes/{print \$(NF)}" tmp/grepedData > tmp/DedepRatio
	awk -F"%" "BEGIN{line=\"dedupRatio\"} {line=line\",\"\$1 } END{print line}" tmp/DedepRatio >> ${category[a]}.csv
	
	#Time
	awk "/real/{print \$(NF)}" tmp/grepedData > tmp/Time
	awk -F"[ms]" "BEGIN{line=\"time\"} {time=\$1*60+\$2; line=line\",\"time} END{print line}" tmp/Time >> ${category[a]}.csv
	
	#hash/meta num
	awk "BEGIN{line=\"hash/meta num\"} /hash number/{line=line\",\"\$(NF)} /new metadata item/{metaNum=substr(\$7, 2, length(\$7)-2); line=line\",\"metaNum} END{print line}" tmp/grepedData >> ${category[a]}.csv

done

#merge files
for a in "${category[@]}"
do
	mergePara=$mergePara$a".csv "
done
paste -d"," $mergePara > $output
