#!/usr/bin/env python
#
# Copyright (c) 2008 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
"""
Binary package creator
"""

import sys
import os
import subprocess

def usage(prname):
	"Print usage syntax"
	print prname + " <OBJCOPY> <FORMAT> <ARCH> <ALIGNMENT> <UINTPTR>"

def main():
	if (len(sys.argv) < 6):
		usage(sys.argv[0])
		return
	
	if (not sys.argv[4].isdigit()):
		print "<ALIGNMENT> must be a number"
		return
	
	objcopy = sys.argv[1]
	format = sys.argv[2]
	arch = sys.argv[3]
	align = int(sys.argv[4], 0)
	uintptr = sys.argv[5]
	
	workdir = os.getcwd()
	
	header = file("_components.h", "w")
	data = file("_components.c", "w")
	
	header.write("#ifndef ___COMPONENTS_H__\n")
	header.write("#define ___COMPONENTS_H__\n\n")
	
	data.write("#include \"_components.h\"\n\n")
	data.write("void init_components(component_t *components)\n")
	data.write("{\n")
	
	cnt = 0
	link = ""
	for task in sys.argv[6:]:
		basename = os.path.basename(task)
		plainname = os.path.splitext(basename)[0]
		path = os.path.dirname(task)
		object = plainname + ".co"
		symbol = "_binary_" + basename.replace(".", "_")
		macro = plainname.upper()
		
		print task + " -> " + object
		
		if (align > 1):
			link += "\t\t. = ALIGN(" + ("%d" % align) + ");\n"
		
		link += "\t\t*(." + plainname + "_image);\n"
		
		header.write("extern int " + symbol + "_start;\n")
		header.write("extern int " + symbol + "_end;\n\n")
		header.write("#define " + macro + "_START ((void *) &" + symbol + "_start)\n")
		header.write("#define " + macro + "_END ((void *) &" + symbol + "_end)\n")
		header.write("#define " + macro + "_SIZE ((" + uintptr + ") " + macro + "_END - (" + uintptr + ") " + macro + "_START)\n\n")
		
		data.write("\tcomponents[" + ("%d" % cnt) + "].name = \"" + plainname + "\";\n")
		data.write("\tcomponents[" + ("%d" % cnt) + "].start = " + macro + "_START;\n")
		data.write("\tcomponents[" + ("%d" % cnt) + "].end = " + macro + "_END;\n")
		data.write("\tcomponents[" + ("%d" % cnt) + "].size = " + macro + "_SIZE;\n\n")
		
		os.chdir(path)
		subprocess.call([objcopy,
			"-I", "binary",
			"-O", format,
			"-B", arch,
			"--rename-section", ".data=." + plainname + "_image",
			basename, os.path.join(workdir, object)])
		os.chdir(workdir)
		
		cnt += 1
		
	header.write("#define COMPONENTS " + ("%d" % cnt) + "\n\n")
	header.write("typedef struct {\n")
	header.write("\tchar *name;\n\n")
	header.write("\tvoid *start;\n")
	header.write("\tvoid *end;\n")
	header.write("\t" + uintptr + " size;\n")
	header.write("} component_t;\n\n")
	header.write("extern void init_components(component_t *components);\n\n")
	header.write("#endif\n")
	
	data.write("}\n")
	
	header.close()
	data.close()
	
	linkname = "_link.ld"
	link_in = file(linkname + ".in", "r")
	template = link_in.read(os.path.getsize(linkname + ".in"))
	link_in.close()
	
	link_out = file(linkname, "w")
	link_out.write(template.replace("[[COMPONENTS]]", link))
	link_out.close()

if __name__ == '__main__':
	main()
