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
xapic = bool(val & (1 << 11))


 # From table 2.1 
 # https://courses.cs.washington.edu/courses/cse451/24wi/documentation/x2apic.pdf
 # xAPIC | x2APIC    | Msg
 # 0     |   0       | local APIC is disabled
 # 0     |   1       | Invalid
 # 1     |   0       | local APIC is enabled in xAPIC mode
 # 1     |   1       | local APIC is enabled in x2APIC mode

if xapic and x2apic:
    msg = "local APIC is enabled in x2APIC mode"
elif xapic and not x2apic:
    msg =  "local APIC is enabled in xAPIC mode"
elif not xapic and x2apic:
    msg = "Invalid"
else:
    msg = "local APIC is disabled"

print(msg)
