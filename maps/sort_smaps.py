#!/usr/bin/python3

import subprocess
import argparse
import sys
import math
from typing import List, Dict
import re
import json

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
        # for line in verbose_list:
        #     print(f" * {line}", file=sys.stderr)

        sys.exit(-1)

    return pids[0]


def parse_memory_map(file_content: str) -> List[Dict]:
    blocks = re.split(r'\n(?=[\da-f]+-[\da-f]+)', file_content.strip())
    parsed_blocks = []

    for block in blocks:
        lines = block.strip().split('\n')
        parsed_block = {}

        # Parse the first line
        match = re.match(r'([\da-f]+)-([\da-f]+)\s+(\S+)\s+(\S+)\s+(\S+):(\S+)\s+(\d+)\s+(.+)?', lines[0])
        if match:
            parsed_block['start_address'] = match.group(1)
            parsed_block['end_address'] = match.group(2)
            parsed_block['permissions'] = match.group(3)
            parsed_block['offset'] = match.group(4)
            parsed_block['device'] = f"{match.group(5)}:{match.group(6)}"
            parsed_block['inode'] = match.group(7)
            parsed_block['pathname'] = match.group(8) if match.group(8) else ''

        # Parse the remaining lines
        for line in lines[1:]:
            key, value = line.split(':', 1)
            key = key.strip()
            value = value.strip()
            
            # Convert numeric values to integers
            if value.endswith(' kB'):
                value = int(value[:-3])
            elif value.isdigit():
                value = int(value)
            
            parsed_block[key] = value

        parsed_blocks.append(parsed_block)

    return parsed_blocks


def read_proc_smaps(pid: int, verbose: bool) -> List[str]:
    with open(f"/proc/{pid}/smaps") as f:
        file_content = f.read()
        blocks = parse_memory_map(file_content)

    sorted_data = sorted(blocks, key=lambda x: x.get('Size', 0), reverse=True)
    return sorted_data


def read_rss_from_smaps(pid: int, verbose: bool) -> None:
    with open(f"/proc/{pid}/smaps_rollup") as f:
        for line in f.readlines():
            if line.startswith("Rss:"):
                if line.split()[2] != "kB":
                    print(f"Warning. No 'kB' found in : {line}")
                    continue

                rss = line.split()[1]
                return int(rss)

    print(f"Error reading RSS from smaps_rollup for PID={pid}", file=sys.stderr)
    sys.exit(-1)

def get_from_maps(pid: int, verbose: bool) -> None:
    read_proc_maps(pid, verbose)
    rss = read_rss_from_status(pid, verbose)
    # Getting the numbers in Kb to compare with the other method
    global total_size
    total_size = int(total_size/1024)
    rss = rss

    if verbose:
        print(f"MAPS : Size = {total_size}, RSS = {rss}, VMA entries = {entries}")
    else:
        print(f"MAPS,{total_size},{rss},{entries}")

def get_from_smaps(pid: int, verbose: bool) -> None:
    blocks = read_proc_smaps(pid, verbose)
    for block in blocks:
       json_string = json.dumps(block, indent=4)
       print(json_string)




def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='parse /proc/pid/maps for a process name')
    parser.add_argument('name', help='process name')
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

    get_from_smaps(pid, args.verbose)
