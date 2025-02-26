#!env python3

import os, glob
from typing import Final
import logging

TIMERS: Final[str] = ["CLOCK_REALTIME", "CLOCK_MONOTONIC", "CLOCK_PROCESS_CPUTIME_ID", "CLOCK_THREAD_CPUTIME_ID", "CLOCK_MONOTONIC_RAW",  "CLOCK_REALTIME_COARSE", "CLOCK_MONOTONIC_COARSE", "CLOCK_BOOTTIME"]
PATH: Final[str] = os.getcwd() + "/results/"

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



print('name, ', end="")
for col in columns:
    col = col.split('_')[1]
    print(f"{col}, ", end="")

print("")

for timer, values in d.items():
    print(f"{timer} ", end='')
    for col in columns:
        print(f"{values[col]}, ", end = '')

    print()
