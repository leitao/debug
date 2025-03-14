#!env python3
  ##define VM_NONE         0x00000000
  ##define VM_READ         0x00000001      /* currently active flags */
  ##define VM_WRITE        0x00000002
  ##define VM_EXEC         0x00000004
  ##define VM_SHARED       0x00000008
  #/* mprotect() hardcodes VM_MAYREAD >> 4 == VM_READ, and so for r/w/x bits. */
  ##define VM_MAYREAD      0x00000010      /* limits for mprotect() etc */
  ##define VM_MAYWRITE     0x00000020
  ##define VM_MAYEXEC      0x00000040
  ##define VM_MAYSHARE     0x00000080
mymap = { "VM_NONE" : 0, "VM_READ" : 0x00000001,
         "VM_WRITE" : 0x00000002, "VM_EXEC" : 0x00000004,
         "VM_SHARED": 0x00000008, "VM_MAYREAD": 0x00000010,
         "VM_MAYWRITE": 0x00000020, "VM_MAYEXEC": 0x00000040,
         "VM_MAYSHARE": 0x00000080, "VM_ACCOUNT": 0x00100000 }
foo = input()
myinput = int(foo, base=16)
#
for k, v in mymap.items():
    if v & myinput:
        myinput = ((~v) & myinput);
        print(k)
print(hex(myinput)) # needs to zero
