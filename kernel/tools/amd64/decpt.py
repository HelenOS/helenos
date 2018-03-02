#!/usr/bin/env python
"""
Decode 64-bit address into components
"""
import sys

def main():
    if len(sys.argv) != 2 or not sys.argv[1].startswith('0x'):
        print("%s 0x..." % sys.argv[0])
        sys.exit(1)

    address = int(sys.argv[1],16)
    offset = address & 0xfff
    ptl3 = (address >> 12) & 0x1ff
    ptl2 = (address >> 21) & 0x1ff
    ptl1 = (address >> 30) & 0x1ff
    ptl0 = (address >> 39) & 0x1ff
    print("Ptl0:   %3d" % ptl0)
    print("Ptl1:   %3d" % ptl1)
    print("Ptl2:   %3d" % ptl2)
    print("Ptl3:   %3d" % ptl3)
    print("Offset: 0x%x" % offset)

if __name__ == '__main__':
    main()
