#!/usr/bin/env python

import sys
import struct
import re

symline = re.compile(r'(0x[a-f0-9]+)\s+([^\s]+)$')
fileline = re.compile(r'[^\s]+\s+0x[a-f0-9]+\s+0x[a-f0-9]+\s+([^\s]+\.o)$')

MAXSTRING=63
symtabfmt = "<Q%ds" % (MAXSTRING+1)

def read_symbols(inp):
    while 1:
        line = inp.readline()
        if not line:
            return
        if 'memory map' in line:
            break        

    symtable = {}
    filename = ''
    while 1:
        line = inp.readline()
        if not line.strip():
            continue
        if line[0] not in (' ','.'):
            break
        line = line.strip()
        # Search for file name
        res = fileline.match(line)
        if res:
            filename = res.group(1)
        # Search for symbols
        res = symline.match(line)
        if res:
            symtable[int(res.group(1),16)] = filename + ':' + res.group(2)
    return symtable
    
def generate(inp, out):
    symtab = read_symbols(inp)
    if not symtab:
        print "Bad kernel.map format, empty."
        sys.exit(1)
    addrs = symtab.keys()
    addrs.sort()
    for addr in addrs:
        # Do not write address 0, it indicates end of data
        if addr == 0:
            continue
        data = struct.pack(symtabfmt,addr,symtab[addr][:MAXSTRING])
        out.write(data)
    out.write(struct.pack(symtabfmt,0,''))

def main():
    if len(sys.argv) != 3:
        print "Usage: %s <kernel.map> <output.bin>" % sys.argv[0]
        sys.exit(1)

    inp = open(sys.argv[1],'r')
    out = open(sys.argv[2],'w')
    generate(inp,out)
    inp.close()
    out.close()

if __name__ == '__main__':
    main()
