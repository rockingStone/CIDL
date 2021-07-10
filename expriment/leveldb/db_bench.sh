#! /bin/bash
time /pmem/benchmark/db_bench --benchmarks=fillrandom --db=/pmem/dbtmp
# --num=5000000
time /pmem/benchmark/db_bench_modified --benchmarks=fillrandom --db=/pmem/dbtmp
# --num=5000000
/home/xzjin/destorTest/rebuild
#destor /home/xzjin/dbtmp -p"chunk-algorithm 'normalized rabin'"
#destor /home/xzjin/dbtmp -p"chunk-algorithm file"
#destor /home/xzjin/dbtmp -p"chunk-algorithm fixed"
#destor /home/xzjin/dbtmp -p"chunk-algorithm tttd"
#destor /home/xzjin/dbtmp -p"chunk-algorithm ae"
#destor /home/xzjin/dbtmp -p"chunk-algorithm rabin"
destor /pmem/dbtmp -p"chunk-algorithm fastcdc"
