#!/usr/bin/env python

import sys
import struct
import re

MAXSTRING=63
symtabfmt = "<Q%ds" % (MAXSTRING+1)


objline = re.compile(r'([0-9a-f]+)\s+[lg]\s+F\s+\.text\s+[0-9a-f]+\s+(.*)$')
fileexp = re.compile(r'([^\s]+):\s+file format')
def read_obdump(inp):
    result = {}
    fname = ''
    for line in inp:
        line = line.strip()
        res = objline.match(line)
        if res:
            result.setdefault(fname,[]).append((int(res.group(1),16),
                                                 res.group(2)))
        res = fileexp.match(line)
        if res:
            fname = res.group(1)
            continue
    
    return result

startfile = re.compile(r'\.text\s+(0x[0-9a-f]+)\s+0x[0-9a-f]+\s+(.*)$')
def generate(kmapf, obmapf, out):
    obdump = read_obdump(obmapf)

    def sorter(x,y):
        return cmp(x[0],y[0])

    for line in kmapf:
        line = line.strip()
        res = startfile.match(line)
        if res and obdump.has_key(res.group(2)):
            offset = int(res.group(1),16)
            fname = res.group(2)
            symbols = obdump[fname]
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
