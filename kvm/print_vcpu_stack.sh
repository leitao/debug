#!/bin/bash

QEMU_PID=$(ps aux | grep -v grep | grep qemu-sys | awk '{print $2}')

CPUS=$(ps -T -p ${QEMU_PID} | grep CPU | awk '{print $2}')

for CPU in ${CPUS}
do
        cat /proc/${CPU}/comm
        cat /proc/${CPU}/stack
done
