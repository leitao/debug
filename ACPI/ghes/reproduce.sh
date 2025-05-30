#!/bin/bash
#
dmesg -c
for i in `seq 0 50000 10000000000`
do
	echo $i > /sys/kernel/debug/apei/einj/param4
	echo $i > /dev/kmsg
	cat /sys/kernel/debug/apei/einj/param4
	echo 1 > /sys/kernel/debug/apei/einj/error_inject
	sleep .05
	if [ dmesg | grep AER ]
	then
		exit 0;
	done
done
