#!/bin/bash
#

FREQ=3393000

cpupower frequency-set -g performance

for i in $(seq $(nproc))
do
	echo $FREQ > /sys/devices/system/cpu/cpu${i}/cpufreq/scaling_max_freq
	echo $FREQ > /sys/devices/system/cpu/cpu${i}/cpufreq/scaling_min_freq
	# echo $FREQ > /sys/devices/system/cpu/cpu${i}/cpufreq/cpuinfo_max_freq
	# echo $FREQ > /sys/devices/system/cpu/cpu${i}/cpufreq/cpuinfo_min_freq
done


