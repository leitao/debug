#!/bin/bash

sysbench fileio --file-total-size=10G --file-num=1 --file-block-size=16K prepare

for i in `seq 10`;
do
	sysbench fileio --file-total-size=10G --file-num=1 --file-block-size=16K --file-test-mode=rndrw --file-io-mode=async --file-extra-flags=direct --file-fsync-freq=0 --time=60 run > file_$i;
done
