#!/usr/bin/env python
#
# Copyright (c) 2010 Martin Decky
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
Check whether several important prerequisites for building HelenOS
are present in the system
"""
import sys
import os
import re
import subprocess

MAKEFILE = 'Makefile.config'

def read_config(fname, config):
	"Read HelenOS build configuration"
	
	inf = file(fname, 'r')
	
	for line in inf:
		res = re.match(r'^(?:#!# )?([^#]\w*)\s*=\s*(.*?)\s*$', line)
		if (res):
			config[res.group(1)] = res.group(2)
	
	inf.close()

def print_error(msg):
	"Print a bold error message"
	
	sys.stderr.write("\n")
	sys.stderr.write("######################################################################\n")
	sys.stderr.write("HelenOS build sanity check error:\n")
	sys.stderr.write("\n")
	sys.stderr.write("%s\n" % "\n".join(msg))
	sys.stderr.write("######################################################################\n")
	sys.stderr.write("\n")
	
	sys.exit(1)

def check_app(args, name, details):
	"Check whether an application can be executed"
	
	try:
		sys.stderr.write("Checking for %s ... " % args[0])
		subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.PIPE).wait()
	except:
		sys.stderr.write("failed\n")
		print_error(["%s is missing." % name,
		             "",
		             "Execution of \"%s\" has failed. Please make sure that it" % " ".join(args),
		             "is installed in your system (%s)." % details])
	
	sys.stderr.write("ok\n")

def check_binutils():
	"Check for binutils toolchain"
	
	check_app(["as", "--version"], "GNU Assembler", "usually part of binutils")
	check_app(["ld", "--version"], "GNU Linker", "usually part of binutils")
	check_app(["ar", "--version"], "GNU Archiver", "usually part of binutils")
	check_app(["objcopy", "--version"], "Objcopy utility", "usually part of binutils")
	check_app(["objdump", "--version"], "Objdump utility", "usually part of binutils")

def main():
	config = {}
	
	# Read configuration
	if os.path.exists(MAKEFILE):
		read_config(MAKEFILE, config)
	else:
		print_error(["Configuration file %s not found! Make sure that the" % MAKEFILE,
		             "configuration phase of HelenOS build went OK. Try running",
		             "\"make config\" again."])
	
	if ('CROSS_PREFIX' in os.environ):
		cross_prefix = os.environ['CROSS_PREFIX']
	else:
		cross_prefix = "/usr/local"
	
	# Common utilities
	check_app(["ln", "--version"], "Symlink utility", "usually part of coreutils")
	check_app(["rm", "--version"], "File remove utility", "usually part of coreutils")
	check_app(["mkdir", "--version"], "Directory creation utility", "usually part of coreutils")
	check_app(["cp", "--version"], "Copy utility", "usually part of coreutils")
	check_app(["uname", "--version"], "System information utility", "usually part of coreutils")
	check_app(["find", "--version"], "Find utility", "usually part of findutils")
	check_app(["diff", "--version"], "Diff utility", "usually part of diffutils")
	check_app(["make", "--version"], "Make utility", "preferably GNU Make")
	check_app(["makedepend", "-f", "-"], "Makedepend utility", "usually part of imake or xutils")
	
	try:
		if (config['COMPILER'] == "gcc_native"):
			check_app(["gcc", "--version"], "GNU GCC", "preferably version 4.4 or newer")
			check_binutils()
		
		if (config['COMPILER'] == "icc"):
			check_app(["icc", "-V"], "Intel C++ Compiler", "support is experimental")
			check_binutils()
		
		if (config['COMPILER'] == "suncc"):
			check_app(["suncc", "-V"], "Sun Studio Compiler", "support is experimental")
			check_binutils()
		
		if (config['COMPILER'] == "clang"):
			check_app(["clang", "--version"], "Clang compiler", "preferably version 1.0 or newer")
			check_binutils()
		
		if ((config['UARCH'] == "amd64") or (config['UARCH'] == "ia32") or (config['UARCH'] == "ppc32") or (config['UARCH'] == "sparc64")):
			check_app(["mkisofs", "--version"], "ISO 9660 creation utility", "usually part of genisoimage")
		
	except KeyError:
		print_error(["Build configuration of HelenOS is corrupted or the sanity checks",
		             "are out-of-sync. Try running \"make config\" again. ",
		             "If the problem persists, please contact the developers of HelenOS."])
	
	return 0

if __name__ == '__main__':
	sys.exit(main())
