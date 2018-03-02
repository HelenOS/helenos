#!/usr/bin/env python
"""
Decode 32-bit address into PTE components
"""
import sys

def main():
    if len(sys.argv) != 2 or not sys.argv[1].startswith('0x'):
        print("%s 0x..." % sys.argv[0])
        sys.exit(1)

    address = int(sys.argv[1],16)
    offset = address & 0xfff
    ptl1 = (address >> 12) & 0x3ff
    ptl0 = (address >> 22) & 0x3ff
    print("Ptl0:   %3d" % ptl0)
    print("Ptl1:   %3d" % ptl1)
    print("Offset: 0x%x" % offset)

if __name__ == '__main__':
    main()
