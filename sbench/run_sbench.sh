#!/bin/bash

NAME=${1:-"default"}
MAX=${2:-100}
PER_RUN=${3:-60}


set -e

echo running inside ${NAME} for ${MAX} iterations of ${PER_RUN} seconds

mkdir ${NAME} || true
cd ${NAME}
zcat /proc/config.gz > kernel_config
cat /proc/cmdline > cmdline

# remove if it exists
rm -fr results.txt || true

for i in `seq $MAX`
do
        echo -n .
        set -x
        ../sbench -r -t ${PER_RUN} p `nproc` >> results.txt
done
echo
