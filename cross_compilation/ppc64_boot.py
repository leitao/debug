#!/usr/bin/env python3
"""
Script to download a PowerPC64 Debian rootfs, compile a kernel, and boot with QEMU.
"""

import subprocess
import os
import sys
import argparse
import shutil
from pathlib import Path


WORK_DIR = Path("ppc64_workdir")
ROOTFS_DIR = WORK_DIR / "rootfs"
KERNEL_DIR = Path("/home/leit/Devel/linux-next")
ROOTFS_IMAGE = WORK_DIR / "rootfs.ext4"


def run_cmd(cmd, check=True, cwd=None, env=None):
    """Run a shell command and print it."""
    print(f">>> {cmd}")
    return subprocess.run(cmd, shell=True, check=check, cwd=cwd, env=env)


def check_dependencies():
    """Check if required tools are installed."""
    required = [
        "qemu-system-ppc64",
        "debootstrap",
        "make",
        "clang",
        "lld",
    ]
    missing = []
    for tool in required:
        if shutil.which(tool.split()[0]) is None:
            missing.append(tool)

    if missing:
        print("Missing required tools:")
        for tool in missing:
            print(f"  - {tool}")
        print("\nInstall with:")
        print("  sudo dnf install qemu-system-ppc debootstrap clang lld llvm make flex bison openssl-devel bc elfutils-libelf-devel")
        sys.exit(1)


def download_rootfs():
    """Create a Debian PowerPC64LE rootfs using debootstrap --foreign."""
    print("\n=== Creating Debian PowerPC64LE rootfs ===\n")

    if ROOTFS_DIR.exists():
        print(f"Rootfs directory {ROOTFS_DIR} already exists, skipping.")
        return

    ROOTFS_DIR.mkdir(parents=True, exist_ok=True)

    # Use debootstrap with --foreign (no chroot execution needed)
    run_cmd(
        f"sudo debootstrap --arch=ppc64el --variant=minbase --foreign "
        f"bookworm {ROOTFS_DIR} http://deb.debian.org/debian"
    )

    # Create a simple init script that completes debootstrap and starts shell
    init_content = """#!/bin/sh
export PATH=/usr/sbin:/usr/bin:/sbin:/bin

mount -t proc proc /proc
mount -t sysfs sys /sys
mount -t devtmpfs dev /dev

if [ -x /debootstrap/debootstrap ]; then
    echo "Completing debootstrap second stage..."
    /debootstrap/debootstrap --second-stage
fi

echo "Welcome to PowerPC64LE Debian!"
exec /bin/sh
"""
    with open("/tmp/ppc64_init", "w") as f:
        f.write(init_content)
    run_cmd(f"sudo cp /tmp/ppc64_init {ROOTFS_DIR}/init")
    run_cmd(f"sudo chmod +x {ROOTFS_DIR}/init")

    print("Rootfs created successfully.")


def create_rootfs_image():
    """Create an ext4 image from the rootfs directory."""
    print("\n=== Creating rootfs ext4 image ===\n")

    if ROOTFS_IMAGE.exists():
        print(f"Rootfs image {ROOTFS_IMAGE} already exists, skipping.")
        return

    # Create a 1GB ext4 image
    run_cmd(f"dd if=/dev/zero of={ROOTFS_IMAGE} bs=1M count=1024")
    run_cmd(f"mkfs.ext4 {ROOTFS_IMAGE}")

    # Mount and copy rootfs contents
    mount_point = WORK_DIR / "mnt"
    mount_point.mkdir(exist_ok=True)

    run_cmd(f"sudo mount -o loop {ROOTFS_IMAGE} {mount_point}")
    run_cmd(f"sudo cp -a {ROOTFS_DIR}/. {mount_point}/")
    run_cmd(f"sudo umount {mount_point}")

    print("Rootfs image created successfully.")


def compile_kernel():
    """Compile Linux kernel for PowerPC64LE using LLVM."""
    print("\n=== Compiling Linux kernel for PowerPC64LE ===\n")

    vmlinux = KERNEL_DIR / "vmlinux"
    if vmlinux.exists():
        print(f"Kernel {vmlinux} already exists, skipping compilation.")
        return

    if not KERNEL_DIR.exists():
        print(f"Error: Kernel source not found at {KERNEL_DIR}")
        sys.exit(1)

    # Set up LLVM cross-compilation environment
    env = os.environ.copy()
    env["ARCH"] = "powerpc"
    env["LLVM"] = "1"

    # Configure kernel with default ppc64le config
    run_cmd("make ppc64le_defconfig", cwd=KERNEL_DIR, env=env)

    # Enable virtio drivers for QEMU
    config_options = [
        "CONFIG_VIRTIO=y",
        "CONFIG_VIRTIO_PCI=y",
        "CONFIG_VIRTIO_BLK=y",
        "CONFIG_VIRTIO_NET=y",
        "CONFIG_VIRTIO_CONSOLE=y",
        "CONFIG_EXT4_FS=y",
        "CONFIG_PPC_PSERIES=y",
        # Debug options
        "CONFIG_DEBUG_INFO=y",
        "CONFIG_DEBUG_INFO_DWARF5=y",
        "CONFIG_DEBUG_KERNEL=y",
        "CONFIG_DEBUG_MISC=y",
        "CONFIG_MAGIC_SYSRQ=y",
        "CONFIG_DEBUG_FS=y",
        "CONFIG_KGDB=y",
        "CONFIG_KGDB_SERIAL_CONSOLE=y",
        "CONFIG_PRINTK_TIME=y",
        "CONFIG_DYNAMIC_DEBUG=y",
        "CONFIG_STACKTRACE=y",
        "CONFIG_DEBUG_BUGVERBOSE=y",
        "CONFIG_SCHED_DEBUG=y",
        "CONFIG_DEBUG_PREEMPT=y",
        "CONFIG_PROVE_LOCKING=y",
        "CONFIG_LOCK_STAT=y",
        "CONFIG_DEBUG_ATOMIC_SLEEP=y",
        "CONFIG_DEBUG_LIST=y",
        "CONFIG_DEBUG_PLIST=y",
        "CONFIG_DEBUG_SG=y",
        "CONFIG_DEBUG_NOTIFIERS=y",
        "CONFIG_FTRACE=y",
        "CONFIG_FUNCTION_TRACER=y",
        "CONFIG_FUNCTION_GRAPH_TRACER=y",
        "CONFIG_STACK_TRACER=y",
    ]

    for opt in config_options:
        run_cmd(f"./scripts/config --enable {opt.split('=')[0].replace('CONFIG_', '')}",
                cwd=KERNEL_DIR, env=env)

    run_cmd("make olddefconfig", cwd=KERNEL_DIR, env=env)

    # Compile kernel
    nproc = os.cpu_count() or 4
    run_cmd(f"make -j{nproc} vmlinux", cwd=KERNEL_DIR, env=env)

    print("Kernel compiled successfully.")


def boot_qemu():
    """Boot the kernel with QEMU using the rootfs."""
    print("\n=== Booting with QEMU ===\n")

    vmlinux = KERNEL_DIR / "vmlinux"

    if not vmlinux.exists():
        print(f"Error: Kernel not found at {vmlinux}")
        sys.exit(1)

    if not ROOTFS_IMAGE.exists():
        print(f"Error: Rootfs image not found at {ROOTFS_IMAGE}")
        sys.exit(1)

    qemu_cmd = [
        "qemu-system-ppc64",
        "-machine", "pseries",
        "-cpu", "power9",
        "-m", "2G",
        "-smp", "2",
        "-kernel", str(vmlinux),
        "-drive", f"file={ROOTFS_IMAGE},format=raw,if=virtio",
        "-append", "root=/dev/vda rw console=hvc0 init=/init",
        "-nographic",
        "-serial", "mon:stdio",
    ]

    print(f"Running: {' '.join(qemu_cmd)}")
    subprocess.run(qemu_cmd)


def main():
    parser = argparse.ArgumentParser(
        description="Download PowerPC64 Debian rootfs, compile kernel, and boot with QEMU"
    )
    parser.add_argument(
        "--skip-rootfs", action="store_true", help="Skip rootfs download"
    )
    parser.add_argument(
        "--skip-kernel", action="store_true", help="Skip kernel compilation"
    )
    parser.add_argument(
        "--boot-only", action="store_true", help="Only boot (skip download and compile)"
    )
    args = parser.parse_args()

    WORK_DIR.mkdir(exist_ok=True)

    check_dependencies()

    if not args.boot_only:
        if not args.skip_rootfs:
            download_rootfs()
            create_rootfs_image()

        if not args.skip_kernel:
            compile_kernel()

    boot_qemu()


if __name__ == "__main__":
    main()
