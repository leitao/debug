#!/bin/bash


sudo rmdir /sys/fs/cgroup/my_memcg || true
sudo mkdir /sys/fs/cgroup/my_memcg

sudo dmesg -c > /dev/null

echo "100M" | sudo tee /sys/fs/cgroup/my_memcg/memory.max

# we want to kill all the processes in the cgroup
# Without this, the problem doesn't happen
echo 1 | sudo tee /sys/fs/cgroup/my_memcg/memory.oom.group

make
for i in $(seq 2000)
do
	echo -n "."
	make run &
done
# Wait here
make run

sudo rm -fr /sys/fs/cgroup/my_memcg
