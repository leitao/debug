#!/usr/bin/python3

import subprocess
import argparse
import sys
import math

total_size = 0
entries = 0

def find_pid(name: str) -> int:
    command = ["ps", "aux"]

    pids = []
    proc = subprocess.Popen(command, stdout=subprocess.PIPE)
    verbose_list = []
    for line_ in proc.stdout:
        line = line_.decode().strip()
        if name in line and "python" not in line and "procmaps" not in line:
            # print(f"Found process:\n   *  {line}", file=sys.stderr)
            ret = line.split()
            verbose_list.append(line)
            pids.append(ret[1])

    if len(pids) > 1:
        print(f"Multiple processes found for '{name}': pids = {pids}", file=sys.stderr)
        for line in verbose_list:
            print(f" * {line}", file=sys.stderr)

        sys.exit(-1)

    return pids[0]

def mem_size(line: str, verbose: bool):
    memrange= line.split(" ")[0]
    area = line.split(" ")[-1].strip()
    start, end = memrange.split("-")
    size = int(f"0x{end}", 16) - int(f"0x{start}", 16)
    # Global variables
    global total_size
    total_size += size
    global entries
    entries += 1

    if verbose:
        print(f"{size :<10} : {area}")


def read_proc_maps(pid: int, verbose: bool):
    with open(f"/proc/{pid}/maps") as f:
        for line in f.readlines():
            mem_size(line, verbose)

def read_proc_smaps(pid: int, verbose: bool):
    with open(f"/proc/{pid}/smaps") as f:
        for line in f.readlines():
            if line.startswith("Size:"):
                if line.split()[2] != "kB":
                    print(f"Warning. No 'kB' found in : {line}")
                    continue


                size = line.split()[1]
                if verbose:
                    print(f"{size :<10}")

                global total_size
                global entries
                entries += 1
                total_size += int(size)


def get_from_maps(pid: int, verbose: bool) -> None:
    read_proc_maps(pid, verbose)

def get_from_smaps(pid: int, verbose: bool) -> None:
    read_proc_smaps(pid, verbose)

def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='parse /proc/pid/maps for a process name')
    parser.add_argument('name', help='process name')
    parser.add_argument('--maps', action='store_true', help='Look at /proc/pid/maps')
    parser.add_argument('--smaps', action='store_true', help='Look at /prod/pid/smap instead of /proc/pid/maps')
    parser.add_argument('--verbose', action='store_true', help='verbose')

    return parser.parse_args()

if __name__ == "__main__":
    args = get_args()
    process_name = args.name

    pid = find_pid(process_name)
    if args.verbose:
        print(f"PID = {pid}", file=sys.stderr)

    if not pid:
        print("Pid not found", file=sys.stderr)
        sys.exit(-1)

    if args.smaps:
        get_from_smaps(pid, args.verbose)
        print(f"SMAPS: Size = {round(total_size/(1024*1024), 2)} Mb and entries = {entries}")

    if args.maps:
        total_size = 0
        entries = 0
        get_from_maps(pid, args.verbose)
        print(f"MAPS: Size = {round(total_size/(1024*1024*1024), 2)} Mb and entries = {entries}")
