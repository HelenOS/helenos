#!/usr/bin/env python

import sys
import struct
import re

MAXSTRING=63
symtabfmt = "<Q%ds" % (MAXSTRING+1)


funcline = re.compile(r'([0-9a-f]+)\s+[lg]\s+F\s+\.text\s+([0-9a-f]+)\s+(.*)$')
bssline = re.compile(r'([0-9a-f]+)\s+[lg]\s+[a-zA-Z]\s+\.bss\s+([0-9a-f]+)\s+(.*)$')
dataline = re.compile(r'([0-9a-f]+)\s+[lg]\s+[a-zA-Z]\s+\.data\s+([0-9a-f]+)\s+(.*)$')
fileexp = re.compile(r'([^\s]+):\s+file format')
def read_obdump(inp):
    funcs = {}
    data = {}
    bss ={}
    fname = ''
    for line in inp:
        line = line.strip()
        res = funcline.match(line)
        if res:
            funcs.setdefault(fname,[]).append((int(res.group(1),16),
                                               res.group(3)))
            continue
        res = bssline.match(line)
        if res:
            start = int(res.group(1),16)
            end = int(res.group(2),16)
            if end:
                bss.setdefault(fname,[]).append((start,res.group(3)))
        res = dataline.match(line)
        if res:
            start = int(res.group(1),16)
            end = int(res.group(2),16)
            if end:
                data.setdefault(fname,[]).append((start,res.group(3)))
        res = fileexp.match(line)
        if res:
            fname = res.group(1)
            continue

    return {
        'text' : funcs,
        'bss' : bss,
        'data' : data
        }

startfile = re.compile(r'\.(text|bss|data)\s+(0x[0-9a-f]+)\s+0x[0-9a-f]+\s+(.*)$')
def generate(kmapf, obmapf, out):
    obdump = read_obdump(obmapf)    

    def sorter(x,y):
        return cmp(x[0],y[0])

    for line in kmapf:
        line = line.strip()
        res = startfile.match(line)

        if res and obdump[res.group(1)].has_key(res.group(3)):
            offset = int(res.group(2),16)
            fname = res.group(3)
            symbols = obdump[res.group(1)][fname]
            symbols.sort(sorter)
            for addr,symbol in symbols:                
                value = fname + ':' + symbol
                data = struct.pack(symtabfmt,addr+offset,value[:MAXSTRING])
                out.write(data)
                
    out.write(struct.pack(symtabfmt,0,''))

def main():
    if len(sys.argv) != 4:
        print "Usage: %s <kernel.map> <nm dump> <output.bin>" % sys.argv[0]
        sys.exit(1)

    kmapf = open(sys.argv[1],'r')
    obmapf = open(sys.argv[2],'r')
    out = open(sys.argv[3],'w')
    generate(kmapf,obmapf,out)
    kmapf.close()
    obmapf.close()
    out.close()

if __name__ == '__main__':
    main()
