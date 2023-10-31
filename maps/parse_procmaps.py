#!env python3.11

import subprocess
import argparse

def find_pid(name: str) -> int:
    command = ["ps", "a"]


    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    while True:
        line = process.stdout.readline().decode()
        if name in line:
            return line.split(" ")[0]

def mem_size(line: str):
    memrange= line.split(" ")[0]
    area = line.split(" ")[-1].strip()
    start, end = memrange.split("-")
    size = int(f"0x{end}", 16) - int(f"0x{start}", 16)
    print(f"{size :<10} : {area}")


def read_proc_maps(pid: int):
    with open(f"/proc/{pid}/maps") as f:
        for line in f.readlines():
            mem_size(line)

def main(process: str) -> None:
    pid = find_pid(process)
    print(f"Pid id {pid}")
    read_proc_maps(pid)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='parse /proc/pid/maps for a process name')
    parser.add_argument('name', help='process name')
    args = parser.parse_args()
    process_name = args.name
    main(process_name)
