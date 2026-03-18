#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# Compare workqueue affinity scope (cache vs cache_shard) using fio over NFS.
# Sets up a local NFS server backed by tmpfs, runs read/write benchmarks, and
# reports BW deltas.

set -euo pipefail

TMPFS_SIZE="8G"
EXPORT_DIR="/tmp/nfsexport"
MOUNT_DIR="/mnt/nfs"

die() { echo "ERROR: $*" >&2; exit 1; }
[[ $EUID -eq 0 ]] || die "must run as root"

echo "==> Stopping chef temporarily..."
/usr/facebook/ops/scripts/chef/stop_chef_temporarily

# --- NFS environment setup ---
echo "==> Installing nfs-utils..."
dnf install -y nfs-utils fio-engine-libaio

echo "==> Creating export directory: ${EXPORT_DIR}"
mkdir -p "${EXPORT_DIR}"

echo "==> Mounting tmpfs (${TMPFS_SIZE}) on ${EXPORT_DIR}..."
if mountpoint -q "${EXPORT_DIR}"; then
        echo "    already mounted, skipping"
else
        mount -t tmpfs -o "size=${TMPFS_SIZE}" tmpfs "${EXPORT_DIR}"
fi

echo "==> Configuring /etc/exports..."
sed -i "\|^${EXPORT_DIR} |d" /etc/exports
echo "${EXPORT_DIR} 127.0.0.1(rw,sync,no_root_squash,no_subtree_check)" >> /etc/exports

echo "==> Starting nfs-server..."
systemctl start nfs-server
exportfs -ra

echo "==> Creating mount point: ${MOUNT_DIR}"
mkdir -p "${MOUNT_DIR}"

echo "==> Mounting NFS share..."
if mountpoint -q "${MOUNT_DIR}"; then
        echo "    already mounted, skipping"
else
        mount -t nfs "127.0.0.1:${EXPORT_DIR}" "${MOUNT_DIR}"
fi

# --- Benchmark ---
AFFINITY_PATH="/sys/module/workqueue/parameters/default_affinity_scope"
IOENGINES=(sync psync vsync pvsync pvsync2 libaio)
FIO_BASE="fio --bs=512 --size=4G --numjobs=200 --direct=1 --filename=/mnt/nfs/foo --runtime=10 --output-format=json"

shard_count=$(drgn -e 'print(prog["wq_cache_shard_count"])' 2>/dev/null | tr -d '(unsigned int)')
echo "=== Workqueue Affinity Scope: cache vs cache_shard ==="
echo "Cache shard count: ${shard_count}"
echo ""

declare -a RESULTS

for engine in "${IOENGINES[@]}"; do
        echo "Testing ioengine=$engine ..."

        # cache: write
        rm -fr /mnt/nfs/*
        echo "cache" > "$AFFINITY_PATH"
        w1=$(${FIO_BASE} --name=wb --rw=write --ioengine="$engine" 2>/dev/null)
        w1_bw=$(echo "$w1" | jq '[.jobs[].write.bw_bytes] | add')

        # cache: read
        r1=$(${FIO_BASE} --name=rb --rw=read --ioengine="$engine" 2>/dev/null)
        r1_bw=$(echo "$r1" | jq '[.jobs[].read.bw_bytes] | add')

        # cache_shard: write
        rm -fr /mnt/nfs/*
        echo "cache_shard" > "$AFFINITY_PATH"
        w2=$(${FIO_BASE} --name=wb --rw=write --ioengine="$engine" 2>/dev/null)
        w2_bw=$(echo "$w2" | jq '[.jobs[].write.bw_bytes] | add')

        # cache_shard: read
        r2=$(${FIO_BASE} --name=rb --rw=read --ioengine="$engine" 2>/dev/null)
        r2_bw=$(echo "$r2" | jq '[.jobs[].read.bw_bytes] | add')

        RESULTS+=("$engine $w1_bw $w2_bw $r1_bw $r2_bw")
done

echo ""
printf "%-12s %12s %12s %10s %12s %12s %10s\n" \
        "ioengine" "write(cache)" "write(shard)" "w_delta" "read(cache)" "read(shard)" "r_delta"
printf "%-12s %12s %12s %10s %12s %12s %10s\n" \
        "--------" "------------" "------------" "-------" "-----------" "-----------" "-------"

for entry in "${RESULTS[@]}"; do
        read -r eng w1b w2b r1b r2b <<< "$entry"
        w1_mb=$(echo "scale=1; $w1b / 1048576" | bc)
        w2_mb=$(echo "scale=1; $w2b / 1048576" | bc)
        r1_mb=$(echo "scale=1; $r1b / 1048576" | bc)
        r2_mb=$(echo "scale=1; $r2b / 1048576" | bc)
        w_delta=$(echo "scale=1; ($w2b - $w1b) * 100 / $w1b" | bc)
        r_delta=$(echo "scale=1; ($r2b - $r1b) * 100 / $r1b" | bc)
        printf "%-12s %10sM %10sM %9s%% %10sM %10sM %9s%%\n" \
                "$eng" "$w1_mb" "$w2_mb" "$w_delta" "$r1_mb" "$r2_mb" "$r_delta"
done

# Restore original
echo "cache" > "$AFFINITY_PATH"
echo ""
echo "Restored default_affinity_scope to: $(cat "$AFFINITY_PATH")"
