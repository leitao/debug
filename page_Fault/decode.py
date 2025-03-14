#!env python3

import sys

mymap = { "VM_NONE" : 0, "VM_READ" : 0x00000001,
         "VM_WRITE" : 0x00000002, "VM_EXEC" : 0x00000004,
         "VM_SHARED": 0x00000008, "VM_MAYREAD": 0x00000010,
         "VM_MAYWRITE": 0x00000020, "VM_MAYEXEC": 0x00000040,
         "VM_MAYSHARE": 0x00000080, "VM_ACCOUNT": 0x00100000,
         "VM_NORESERVE": 0x00200000, "VM_HUGETLB": 0x00400000}



if len(sys.argv) < 2:
    print(f"pass the value to convert: {sys.argv[0]} <vmflag in hex> ")
    exit(1)

myinput = int(sys.argv[1], base=16)

for k, v in mymap.items():
    if v & myinput:
        myinput = ((~v) & myinput);
        print(k)

if myinput:
    print(f"Not all bits decoded {myinput}")
