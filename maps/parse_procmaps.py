#!/usr/bin/python3

import subprocess
import argparse
import sys

total_size = 0
entries = 0

def find_pid(name: str) -> int:
    command = ["ps", "aux"]

    proc = subprocess.Popen(command, stdout=subprocess.PIPE)
    for line_ in proc.stdout:
        line = line_.decode().strip()
        if name in line and "python" not in line:
            # print(f"Found process:\n   *  {line}", file=sys.stderr)
            ret = line.split()
            return ret[1]

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

def main(process: str, summary: bool) -> None:
    pid = find_pid(process)
    if not pid:
        print("Pid not found")
        return

    # print(f"Pid id {pid}", file=sys.stderr)
    read_proc_maps(pid, not summary)

    # Print summary
    if summary:
        global total_size
        global entries
        print(f"Size = {total_size/(1024*1024*1024)} and entries = {entries}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='parse /proc/pid/maps for a process name')
    parser.add_argument('name', help='process name')
    parser.add_argument('--sum', action='store_true', help='print summary')
    args = parser.parse_args()
    process_name = args.name

    main(process_name, args.sum)
