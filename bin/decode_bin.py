#!env python3

import argparse

def find_msb(n):
    """
    Using bit_length() - Most Pythonic and efficient
    """
    if n == 0:
        return -1  # No bits set
    return n.bit_length() - 1

def isset(num, i):
    bit = 1 << i
    return bool(num & bit)

def name(idx, description):
    try:
        return description[idx]
    except IndexError:
        return idx

def print_header(msb, description):
    for i in range(msb, -1, -1):
        p = name(i, description)
        print("{: >6}".format(p), end="")
        if not i % 4:
            print(" ", end="")
    print()
    print("="*msb*8)

def print_num(num, msb):
    for i in range(msb, -1, -1):
        s = isset(num, i)
        print("{: >6}".format(s), end="")
        if not i % 4:
            print(" ", end="")
    print(f"  ({num}) ({hex(num)})")



## start here
parser = argparse.ArgumentParser()
parser.add_argument('number', nargs="+")           # positional argument
parser.add_argument('--desc')
args = parser.parse_args()
nums = [int(i) for i in args.number]

description = []
if args.desc:
    description = args.desc.split(',')

msb = find_msb(max(nums))
print_header(msb, description)

for num in nums:
    print_num(num, msb)
