#!/bin/bash

RUNS=10
NAME=${1:-default}
# 10 seconds
TIMEOUT=10
# number of parallel threads
THREADS=10

make clean
make

mkdir -p results
echo "Starting a test with name: ${NAME}..."

for i in $(seq ${RUNS}) ; do
	DATE=$(date +%s)
	./vdso_bench -t "${TIMEOUT}" -p "{THREADS}" - 2> /dev/null > results/run_${NAME}_${DATE}_${i}
done
