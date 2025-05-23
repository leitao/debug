#!/bin/bash


# sudo rmdir /sys/fs/cgroup/my_memcg || true
sudo mkdir /sys/fs/cgroup/my_memcg

sudo dmesg -c > /dev/null

echo "300M" | sudo tee /sys/fs/cgroup/my_memcg/memory.max
echo "300M" | sudo tee /sys/fs/cgroup/my_memcg/memory.high

cat /sys/fs/cgroup/my_memcg/memory.max

# we want to kill all the processes in the cgroup
# Without this, the problem doesn't happen
echo 1 | sudo tee /sys/fs/cgroup/my_memcg/memory.oom.group
echo 0 | sudo tee /sys/fs/cgroup/my_memcg/memory.swap.max

cat /sys/fs/cgroup/my_memcg/memory.oom.group

make
echo "Starting the programs under my_memcg..."
for i in $(seq 5000)
do
	make run &
done
# Wait here
make run
