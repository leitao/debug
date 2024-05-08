#!/bin/python3

import subprocess
import random
import argparse


from typing import List

BROKEN_EVENTS = ["rNNN", "mem:<addr>[/len][:access]", "cpu/t1=v1[t2=v2t3...]/modifier"]

def run_perf_stat(events: List[str], time: int, verbose: bool) -> None:
    cmd = ["perf", "stat"]
    for event in events:
        cmd.append("-e")
        cmd.append(event)

    cmd.append("sleep")
    cmd.append(str(time))
    if verbose:
        print(cmd)
    # cmd = ["perf", "stat", "sleep", "1"]
    process = subprocess.run(cmd, capture_output=True, text=True)
    if process.returncode:
        print("\n==== Failure===")
        print(f"Cmd: {cmd}")
        print(f"Returned: {process.returncode}")
        print(process.stdout)

    if not verbose:
        print(".", end="", flush=True)


def get_events() -> List[str]:
    process = subprocess.run(["perf", "list", "--json"], capture_output=True, text=True)
    stdout = process.stdout

    events = []
    for line in stdout.splitlines():
        if "EventName" in line:
            event_dirty = line.split("EventName\":")[1]
            event_clean = event_dirty.replace(" ", "").replace("\"", "").replace(",", "")
            if event_clean not in BROKEN_EVENTS:
                events.append(event_clean)

    return events


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--time", help="time (seconds) to run each test",
                        default="1")
    parser.add_argument("-x", "--iterations", help="total number of iterations",
                        default="100")
    parser.add_argument("--forever", help="run forever", action="store_true")
    parser.add_argument("--verbose", help="verbose", action="store_true")
    args = parser.parse_args()

    events = get_events()

    # One at a time
    for event in events:
        run_perf_stat([event], args.time)

    # Parallel
    z = 0
    while z < int(args.iterations):
        events_in_parallel = random.randint(0,10)
        pevents = []
        for i in range(0, events_in_parallel):
            pevents.append(events[random.randint(0, len(events) - 1)])

        run_perf_stat(pevents, args.time, args.verbose)
        if not args.forever:
            z += 1

    print("")

if __name__ == "__main__":
    main()
