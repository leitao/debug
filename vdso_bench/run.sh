#!/bin/bash

RUNS=10
NAME=${1:-default}
# 10 seconds by default
TIMEOUT=${2:-10}
# number of parallel threads
THREADS=10
DATE=$(date +%s)

make clean
make

mkdir -p results
echo "Starting a test with name: ${NAME}..."
for i in $(seq ${RUNS}) ; do
	set -x
	./vdso_bench -t "${TIMEOUT}" -p "${THREADS}" > results/run_${NAME}_${DATE}_${i}
	set +x
done
