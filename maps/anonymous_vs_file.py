#!/usr/bin/python3

from pprint import pprint
import subprocess
import argparse
import sys
from typing import List, Dict, Any
import math

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
        sys.exit(-1)

    return pids[0]

def find_entry_size(lines: List[str]) -> int:
    for i, line in enumerate(lines):
        if "VmFlags:" in line:
            return i + 1

    raise Exception(f"Could not find VmFlags in smaps for PID {pid}")

def read_proc_smaps(pid: int, verbose: bool) -> dict:
    with open(f"/proc/{pid}/smaps") as f:
        lines = f.readlines()

    entry_size = find_entry_size(lines)
    if verbose:
        print(f"Entry size : {entry_size}", file=sys.stderr)

    if len(lines) % entry_size > 0:
        print(f"Error: Unexpected number of lines in smaps for PID {pid}", file=sys.stderr)
        sys.exit(-1)

    entries = []
    for i in range(0, len(lines), entry_size):
        entries.append(process_entry(lines[i : i + entry_size], verbose))

    return entries


def process_entry(line: List[str], verbose: bool) -> Dict[str, str]:
    d = {}
    entries = line[0].split()
    d["name"] = "Anonymous"

    if len(entries) > 5:
        d["name"] = entries[5]

    for i in range(1, len(line)):
        key, value  = line[i].split(":")
        value = value.strip()

        if key == "VmFlags":
            d[key.strip()] = value
            continue

        if "kB" in value:
            new_value, _ = value.split()
            value = int(new_value) * 1024;

        d[key.strip()] = int(value)

    return d

def get_from_smaps(pid: int, verbose: bool) -> List[Dict[str, Any]]:
    return read_proc_smaps(pid, verbose)


def parse_memory(entries: List[Dict[str, Any]]) -> Dict[str, Dict[str, int]]:
    ret = {}

    anonymous = ["Anonymous", "stack", "heap", "vvar"]
    ret = {
        "file": {
            "Rss": 0,
            "VmSize": 0,
        },
        "anon": {
            "Rss": 0,
            "VmSize": 0,
        },

    }
    for entry in entries:
        k = "file"
        if entry["name"] in anonymous:
            k = "anon"

        ret[k]["VmSize"] += entry["Size"]
        ret[k]["Rss"] += entry["Rss"]

    return ret

def get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description='parse /proc/pid/smaps for a process name')
    parser.add_argument('--name', action="store", help='process name')
    parser.add_argument('--verbose', action='store_true', help='verbose')

    return parser.parse_args()

if __name__ == "__main__":
    args = get_args()
    process_name = args.name

    pid = "self"
    if args.name:
        pid = find_pid(process_name)
        if args.verbose:
            print(f"PID = {pid}", file=sys.stderr)

    if not pid:
        print("Pid not found", file=sys.stderr)
        sys.exit(-1)

    entries = get_from_smaps(pid, args.verbose)
    output = parse_memory(entries)
    for k in output.keys():
        ratio = -1
        if output[k]['Rss']:
            ratio = math.ceil(output[k]['VmSize'] / output[k]['Rss'])

        print(f"{k}, {output[k]['Rss']}, {output[k]['VmSize']}, {ratio}")

    if args.verbose:
        pprint(output)
