#!/usr/bin/env python3
"""
Script to download a Debian rootfs, compile a kernel, and boot with QEMU.
Supports ppc64el and arm64 architectures.
"""

import subprocess
import os
import sys
import argparse
import shutil
from pathlib import Path


KERNEL_DIR = Path("/home/leit/Devel/linux-next")

# Architecture-specific configurations
ARCH_CONFIG = {
    "ppc64el": {
        "debian_arch": "ppc64el",
        "kernel_arch": "powerpc",
        "defconfig": "ppc64le_defconfig",
        "kernel_image": "vmlinux",
        "qemu_bin": "qemu-system-ppc64",
        "qemu_machine": "pseries",
        "qemu_cpu": "power9",
        "qemu_console": "hvc0",
        "extra_kernel_opts": ["CONFIG_PPC_PSERIES=y"],
        "welcome_msg": "PowerPC64LE",
    },
    "arm64": {
        "debian_arch": "arm64",
        "kernel_arch": "arm64",
        "defconfig": "defconfig",
        "kernel_image": "arch/arm64/boot/Image",
        "qemu_bin": "qemu-system-aarch64",
        "qemu_machine": "virt",
        "qemu_cpu": "cortex-a72",
        "qemu_console": "ttyAMA0",
        "extra_kernel_opts": ["CONFIG_VIRTIO_MMIO=y"],
        "welcome_msg": "ARM64",
    },
}


def get_paths(arch):
    """Get architecture-specific paths."""
    work_dir = Path(f"{arch}_workdir")
    return {
        "work_dir": work_dir,
        "rootfs_dir": work_dir / "rootfs",
        "rootfs_image": work_dir / "rootfs.ext4",
    }


def run_cmd(cmd, check=True, cwd=None, env=None):
    """Run a shell command and print it."""
    print(f">>> {cmd}")
    return subprocess.run(cmd, shell=True, check=check, cwd=cwd, env=env)


def check_dependencies(arch):
    """Check if required tools are installed."""
    config = ARCH_CONFIG[arch]
    required = [
        config["qemu_bin"],
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
        print("  sudo dnf install qemu-system-ppc qemu-system-aarch64 debootstrap clang lld llvm make flex bison openssl-devel bc elfutils-libelf-devel")
        sys.exit(1)


def download_rootfs(arch):
    """Create a Debian rootfs using debootstrap --foreign."""
    config = ARCH_CONFIG[arch]
    paths = get_paths(arch)

    print(f"\n=== Creating Debian {config['welcome_msg']} rootfs ===\n")

    if paths["rootfs_dir"].exists():
        print(f"Rootfs directory {paths['rootfs_dir']} already exists, skipping.")
        return

    paths["rootfs_dir"].mkdir(parents=True, exist_ok=True)

    # Use debootstrap with --foreign (no chroot execution needed)
    run_cmd(
        f"sudo debootstrap --arch={config['debian_arch']} --variant=minbase --foreign "
        f"bookworm {paths['rootfs_dir']} http://deb.debian.org/debian"
    )

    # Create a simple init script that completes debootstrap and starts shell
    init_content = f"""#!/bin/sh
export PATH=/usr/sbin:/usr/bin:/sbin:/bin

mount -t proc proc /proc
mount -t sysfs sys /sys
mount -t devtmpfs dev /dev

if [ -x /debootstrap/debootstrap ]; then
    echo "Completing debootstrap second stage..."
    /debootstrap/debootstrap --second-stage
fi

echo "Welcome to {config['welcome_msg']} Debian!"
exec /bin/sh
"""
    with open("/tmp/qemu_init", "w") as f:
        f.write(init_content)
    run_cmd(f"sudo cp /tmp/qemu_init {paths['rootfs_dir']}/init")
    run_cmd(f"sudo chmod +x {paths['rootfs_dir']}/init")

    print("Rootfs created successfully.")


def create_rootfs_image(arch):
    """Create an ext4 image from the rootfs directory."""
    paths = get_paths(arch)

    print("\n=== Creating rootfs ext4 image ===\n")

    if paths["rootfs_image"].exists():
        print(f"Rootfs image {paths['rootfs_image']} already exists, skipping.")
        return

    # Create a 1GB ext4 image
    run_cmd(f"dd if=/dev/zero of={paths['rootfs_image']} bs=1M count=1024")
    run_cmd(f"mkfs.ext4 {paths['rootfs_image']}")

    # Mount and copy rootfs contents
    mount_point = paths["work_dir"] / "mnt"
    mount_point.mkdir(exist_ok=True)

    run_cmd(f"sudo mount -o loop {paths['rootfs_image']} {mount_point}")
    run_cmd(f"sudo cp -a {paths['rootfs_dir']}/. {mount_point}/")
    run_cmd(f"sudo umount {mount_point}")

    print("Rootfs image created successfully.")


def compile_kernel(arch):
    """Compile Linux kernel using LLVM."""
    config = ARCH_CONFIG[arch]

    print(f"\n=== Compiling Linux kernel for {config['welcome_msg']} ===\n")

    kernel_image = KERNEL_DIR / config["kernel_image"]
    if kernel_image.exists():
        print(f"Kernel {kernel_image} already exists, skipping compilation.")
        return

    if not KERNEL_DIR.exists():
        print(f"Error: Kernel source not found at {KERNEL_DIR}")
        sys.exit(1)

    # Set up LLVM cross-compilation environment
    env = os.environ.copy()
    env["ARCH"] = config["kernel_arch"]
    env["LLVM"] = "1"

    # Configure kernel with default config
    run_cmd(f"make {config['defconfig']}", cwd=KERNEL_DIR, env=env)

    # Enable virtio drivers for QEMU and debug options
    config_options = [
        "CONFIG_VIRTIO=y",
        "CONFIG_VIRTIO_PCI=y",
        "CONFIG_VIRTIO_BLK=y",
        "CONFIG_VIRTIO_NET=y",
        "CONFIG_VIRTIO_CONSOLE=y",
        "CONFIG_EXT4_FS=y",
        # Export kernel config via /proc/config.gz
        "CONFIG_IKCONFIG=y",
        "CONFIG_IKCONFIG_PROC=y",
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
    ] + config["extra_kernel_opts"]

    for opt in config_options:
        opt_name = opt.split('=')[0].replace('CONFIG_', '')
        # Use --set-val to force y even if already set to m
        run_cmd(f"./scripts/config --set-val {opt_name} y",
                cwd=KERNEL_DIR, env=env)

    run_cmd("make olddefconfig", cwd=KERNEL_DIR, env=env)

    # Compile kernel
    nproc = os.cpu_count() or 4
    target = "vmlinux" if arch == "ppc64el" else "Image"
    run_cmd(f"make -j{nproc} {target}", cwd=KERNEL_DIR, env=env)

    print("Kernel compiled successfully.")


def boot_qemu(arch):
    """Boot the kernel with QEMU using the rootfs."""
    config = ARCH_CONFIG[arch]
    paths = get_paths(arch)

    print("\n=== Booting with QEMU ===\n")

    kernel_image = KERNEL_DIR / config["kernel_image"]

    if not kernel_image.exists():
        print(f"Error: Kernel not found at {kernel_image}")
        sys.exit(1)

    if not paths["rootfs_image"].exists():
        print(f"Error: Rootfs image not found at {paths['rootfs_image']}")
        sys.exit(1)

    if arch == "ppc64el":
        qemu_cmd = [
            config["qemu_bin"],
            "-machine", config["qemu_machine"],
            "-cpu", config["qemu_cpu"],
            "-m", "2G",
            "-smp", "2",
            "-kernel", str(kernel_image),
            "-drive", f"file={paths['rootfs_image']},format=raw,if=virtio",
            "-append", f"root=/dev/vda rw console={config['qemu_console']} init=/init",
            "-nographic",
            "-serial", "mon:stdio",
        ]
    else:  # arm64
        qemu_cmd = [
            config["qemu_bin"],
            "-machine", "virt,gic-version=3",
            "-cpu", config["qemu_cpu"],
            "-m", "2G",
            "-smp", "2",
            "-kernel", str(kernel_image),
            "-drive", f"file={paths['rootfs_image']},format=raw,if=virtio",
            "-append", f"root=/dev/vda rw console={config['qemu_console']} init=/init",
            "-nographic",
        ]

    print(f"Running: {' '.join(qemu_cmd)}")
    subprocess.run(qemu_cmd)


def main():
    parser = argparse.ArgumentParser(
        description="Download Debian rootfs, compile kernel, and boot with QEMU"
    )
    parser.add_argument(
        "--arch", choices=["ppc64el", "arm64"], default="ppc64el",
        help="Target architecture (default: ppc64el)"
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

    paths = get_paths(args.arch)
    paths["work_dir"].mkdir(exist_ok=True)

    check_dependencies(args.arch)

    if not args.boot_only:
        if not args.skip_rootfs:
            download_rootfs(args.arch)
            create_rootfs_image(args.arch)

        if not args.skip_kernel:
            compile_kernel(args.arch)

    boot_qemu(args.arch)


if __name__ == "__main__":
    main()
