#!/bin/bash


sudo rmdir /sys/fs/cgroup/my_memcg || true
sudo mkdir /sys/fs/cgroup/my_memcg

sudo dmesg -c > /dev/null

echo "100M" | sudo tee /sys/fs/cgroup/my_memcg/memory.max
make
make run

sudo rm -fr /sys/fs/cgroup/my_memcg
