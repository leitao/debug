#!/usr/bin/python3

import sys
import subprocess

process = subprocess.Popen(['rdmsr', '0x1b'],
                     stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE)
stdout, stderr = process.communicate()

if stderr:
    print(stderr)
    print("Not able to execure rdmsr. Are you root?")
    sys.exit(1)


val =  int(stdout.decode().strip(), 16)
x2apic = bool(val & (1 << 10))

print(f"APIC enabled: {x2apic}")

