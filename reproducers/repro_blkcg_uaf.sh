#!/bin/bash
# Reproducer for use-after-free in blkcg_unpin_online()
#
# The race: in cgwb_release_workfn(), css_put(wb->blkcg_css) drops the
# CSS refcount. If it reaches zero, the async free chain starts
# (css_release → RCU → css_release_work_fn → css_free_rwork_fn → kfree).
# Then blkcg_unpin_online(wb->blkcg_css) accesses the freed blkcg.
#
# To trigger: we need the CSS free to complete between css_put and
# blkcg_unpin_online. We speed up RCU grace periods, use multiple CPUs,
# and create massive cgroup churn to hit the window.
#
# Author: Breno Leitao <leitao@debian.org>

set -eu

CGROOT=$(findmnt -n -o TARGET -t cgroup2 | head -1)
if [ -z "$CGROOT" ]; then
	CGROOT=/tmp/cg2
	mkdir -p "$CGROOT"
	mount -t cgroup2 none "$CGROOT"
fi

# Speed up RCU grace periods to narrow the free latency
echo 1 > /sys/kernel/rcu_expedited 2>/dev/null || true

# Enable io and memory controllers
echo "+io" > "$CGROOT/cgroup.subtree_control" 2>/dev/null || true
echo "+memory" > "$CGROOT/cgroup.subtree_control" 2>/dev/null || true

TESTDIR="$CGROOT/blkcg_uaf_test"
mkdir -p "$TESTDIR"
echo "+io" > "$TESTDIR/cgroup.subtree_control" 2>/dev/null || true
echo "+memory" > "$TESTDIR/cgroup.subtree_control" 2>/dev/null || true

# Mount a real block device (needed for cgwb creation, tmpfs won't work)
MNTDIR=/tmp/blkcg_test_mnt
mkdir -p "$MNTDIR"

BLKDEV=""
for dev in /dev/vda /dev/vdb /dev/sda /dev/sdb; do
	if [ -b "$dev" ] && ! findmnt -n "$dev" >/dev/null 2>&1; then
		BLKDEV="$dev"
		break
	fi
done

if [ -z "$BLKDEV" ]; then
	echo "ERROR: No available block device found."
	exit 1
fi

echo "Using block device: $BLKDEV"
mkfs.ext4 -q -F "$BLKDEV"
mount "$BLKDEV" "$MNTDIR"

trap "umount $MNTDIR 2>/dev/null; rmdir $TESTDIR/child_* 2>/dev/null; rmdir $TESTDIR 2>/dev/null" EXIT

# Compile C helper that does IO inside a cgroup
HELPER=/tmp/blkcg_writer
cat > /tmp/blkcg_writer.c << 'CEOF'
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
	char buf[4096];
	char pidbuf[32];
	int procs_fd, fd, len, i;

	if (argc < 3)
		return 1;

	/* Move into the cgroup */
	procs_fd = open(argv[1], O_WRONLY);
	if (procs_fd < 0)
		return 1;
	len = snprintf(pidbuf, sizeof(pidbuf), "%d", getpid());
	write(procs_fd, pidbuf, len);
	close(procs_fd);

	/* Buffered writes to create cgwb */
	memset(buf, 'X', sizeof(buf));
	fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return 1;
	for (i = 0; i < 512; i++)
		write(fd, buf, sizeof(buf));
	/* Leave dirty - don't fsync */
	close(fd);
	return 0;
}
CEOF
gcc -O2 -o "$HELPER" /tmp/blkcg_writer.c

echo "=== blkcg_unpin_online UAF reproducer ==="
echo "cgroup: $TESTDIR"
echo "block:  $BLKDEV → $MNTDIR"
echo "rcu_expedited: $(cat /sys/kernel/rcu_expedited 2>/dev/null)"
echo ""
echo "Running... watch dmesg for KASAN splat."
echo ""

# Strategy: run N parallel workers, each rapidly creating/destroying
# cgroups with IO. The parallelism increases chances of the cgwb
# release workfn running while css_free is in progress on another CPU.

run_worker() {
	local wid=$1
	local iters=$2

	for i in $(seq 1 $iters); do
		local CG="$TESTDIR/w${wid}_${i}"
		mkdir "$CG" 2>/dev/null || continue

		# Do IO in this cgroup
		"$HELPER" "$CG/cgroup.procs" "$MNTDIR/wfile_${wid}_${i}" 2>/dev/null || true

		# Immediately destroy - race with cgwb_release_workfn
		rmdir "$CG" 2>/dev/null || true
		rm -f "$MNTDIR/wfile_${wid}_${i}" 2>/dev/null

		# Periodically sync to trigger writeback / cgwb release
		if (( i % 50 == 0 )); then
			sync
		fi
	done
}

NWORKERS=4
ITERS_PER_WORKER=3000

for w in $(seq 1 $NWORKERS); do
	run_worker "$w" "$ITERS_PER_WORKER" &
done

echo "Started $NWORKERS workers, $ITERS_PER_WORKER iterations each"
wait
echo "All workers done."

sync
sleep 3

echo ""
echo "Checking dmesg..."
dmesg | grep -c "BUG: KASAN" && dmesg | grep -A10 "BUG: KASAN" || echo "No KASAN splat detected."
