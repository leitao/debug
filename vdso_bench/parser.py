#!env python3

import os, glob
from typing import Final
import logging

TIMERS: Final[str] = ["getpid", "CLOCK_REALTIME", "CLOCK_MONOTONIC", "CLOCK_PROCESS_CPUTIME_ID", "CLOCK_THREAD_CPUTIME_ID", "CLOCK_MONOTONIC_RAW",  "CLOCK_REALTIME_COARSE", "CLOCK_MONOTONIC_COARSE", "CLOCK_BOOTTIME"]
PATH: Final[str] = os.getcwd() + "/merged/"

# k =  timername, v is {run: time}#
# d : dict[dict[str: str]] = {}
d = {}

columns = []
for filename in os.listdir(PATH):
    # print(filename)
    columns.append(filename)
    with open(os.path.join(PATH, filename)) as file:
        while line := file.readline():
            for timer in TIMERS:
                if timer in line:
                    # Get the proper name from the line
                    timer = line.split(' ')[4]
                    # Number of calls to CLOCK_REALTIME : 34.09 M/s per thread
                    value = line.split(' ')[6]
                    if timer not in d:
                        d[timer] = {}
                    # print(f"+Adding: {timer=} {filename=} = {value=}")
                    d[timer][filename] = value
        # break

columns.sort()

print('name, ', end="")
for col in columns:
    col = col.split('_')[1] + "_" + col.split('_')[3]
    print(f"{col}, ", end="")

print("")

for timer in sorted(d.keys()):
    values = d[timer]
    print(f"{timer}, ", end='')
    for col in columns:
        if not col in values:
            print(",", end='')
            continue
        print(f"{values[col]}, ", end = '')

    print()
