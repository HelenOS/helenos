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
Detect important prerequisites and parameters for building HelenOS
"""

import sys
import os
import re
import time
import subprocess

MAKEFILE = 'Makefile.config'
COMMON = 'Makefile.common'

PACKAGE_BINUTILS = "usually part of binutils"
PACKAGE_GCC = "preferably version 4.4.3 or newer"
PACKAGE_CROSS = "use tools/toolchain.sh to build the cross-compiler toolchain"

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

def check_config(config, key):
	"Check whether the configuration key exists"
	
	if (not key in config):
		print_error(["Build configuration of HelenOS does not contain %s." % key,
		             "Try running \"make config\" again.",
		             "If the problem persists, please contact the developers of HelenOS."])

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

def check_gcc(path, prefix, common, details):
	"Check for GCC"
	
	common['GCC'] = "%sgcc" % prefix
	
	if (not path is None):
		common['GCC'] = "%s/%s" % (path, common['GCC'])
	
	check_app([common['GCC'], "--version"], "GNU GCC", details)

def check_binutils(path, prefix, common, details):
	"Check for binutils toolchain"
	
	common['AS'] = "%sas" % prefix
	common['LD'] = "%sld" % prefix
	common['AR'] = "%sar" % prefix
	common['OBJCOPY'] = "%sobjcopy" % prefix
	common['OBJDUMP'] = "%sobjdump" % prefix
	
	if (not path is None):
		for key in ["AS", "LD", "AR", "OBJCOPY", "OBJDUMP"]:
			common[key] = "%s/%s" % (path, common[key])
	
	check_app([common['AS'], "--version"], "GNU Assembler", details)
	check_app([common['LD'], "--version"], "GNU Linker", details)
	check_app([common['AR'], "--version"], "GNU Archiver", details)
	check_app([common['OBJCOPY'], "--version"], "GNU Objcopy utility", details)
	check_app([common['OBJDUMP'], "--version"], "GNU Objdump utility", details)

def create_output(cmname, common):
	"Create common parameters output"
	
	outcm = file(cmname, 'w')
	
	outcm.write('#########################################\n')
	outcm.write('## AUTO-GENERATED FILE, DO NOT EDIT!!! ##\n')
	outcm.write('#########################################\n\n')
	
	for key, value in common.items():
		outcm.write('%s = %s\n' % (key, value))
	
	outcm.close()

def main():
	config = {}
	common = {}
	
	# Read and check configuration
	if os.path.exists(MAKEFILE):
		read_config(MAKEFILE, config)
	else:
		print_error(["Configuration file %s not found! Make sure that the" % MAKEFILE,
		             "configuration phase of HelenOS build went OK. Try running",
		             "\"make config\" again."])
	
	check_config(config, "PLATFORM")
	check_config(config, "COMPILER")
	check_config(config, "BARCH")
	
	# Cross-compiler prefix
	if ('CROSS_PREFIX' in os.environ):
		cross_prefix = os.environ['CROSS_PREFIX']
	else:
		cross_prefix = "/usr/local"
	
	# Prefix binutils tools on Solaris
	if (os.uname()[0] == "SunOS"):
		binutils_prefix = "g"
	else:
		binutils_prefix = ""
	
	# Common utilities
	check_app(["ln", "--version"], "Symlink utility", "usually part of coreutils")
	check_app(["rm", "--version"], "File remove utility", "usually part of coreutils")
	check_app(["mkdir", "--version"], "Directory creation utility", "usually part of coreutils")
	check_app(["cp", "--version"], "Copy utility", "usually part of coreutils")
	check_app(["find", "--version"], "Find utility", "usually part of findutils")
	check_app(["diff", "--version"], "Diff utility", "usually part of diffutils")
	check_app(["make", "--version"], "Make utility", "preferably GNU Make")
	check_app(["makedepend", "-f", "-"], "Makedepend utility", "usually part of imake or xutils")
	
	# Compiler
	if (config['COMPILER'] == "gcc_cross"):
		if (config['PLATFORM'] == "abs32le"):
			check_config(config, "CROSS_TARGET")
			target = config['CROSS_TARGET']
			
			if (config['CROSS_TARGET'] == "arm32"):
				gnu_target = "arm-linux-gnu"
			
			if (config['CROSS_TARGET'] == "ia32"):
				gnu_target = "i686-pc-linux-gnu"
			
			if (config['CROSS_TARGET'] == "mips32"):
				gnu_target = "mipsel-linux-gnu"
		
		if (config['PLATFORM'] == "amd64"):
			target = config['PLATFORM']
			gnu_target = "amd64-linux-gnu"
		
		if (config['PLATFORM'] == "arm32"):
			target = config['PLATFORM']
			gnu_target = "arm-linux-gnu"
		
		if (config['PLATFORM'] == "ia32"):
			target = config['PLATFORM']
			gnu_target = "i686-pc-linux-gnu"
		
		if (config['PLATFORM'] == "ia64"):
			target = config['PLATFORM']
			gnu_target = "ia64-pc-linux-gnu"
		
		if (config['PLATFORM'] == "mips32"):
			check_config(config, "MACHINE")
			
			if ((config['MACHINE'] == "lgxemul") or (config['MACHINE'] == "msim")):
				target = config['PLATFORM']
				gnu_target = "mipsel-linux-gnu"
			
			if (config['MACHINE'] == "bgxemul"):
				target = "mips32eb"
				gnu_target = "mips-linux-gnu"
		
		if (config['PLATFORM'] == "ppc32"):
			target = config['PLATFORM']
			gnu_target = "ppc-linux-gnu"
		
		if (config['PLATFORM'] == "sparc64"):
			target = config['PLATFORM']
			gnu_target = "sparc64-linux-gnu"
		
		path = "%s/%s/bin" % (cross_prefix, target)
		prefix = "%s-" % gnu_target
		
		check_gcc(path, prefix, common, PACKAGE_CROSS)
		check_binutils(path, prefix, common, PACKAGE_CROSS)
		common['CC'] = common['GCC']
	
	if (config['COMPILER'] == "gcc_native"):
		check_gcc(None, "", common, PACKAGE_GCC)
		check_binutils(None, binutils_prefix, common, PACKAGE_BINUTILS)
		common['CC'] = common['GCC']
	
	if (config['COMPILER'] == "icc"):
		common['CC'] = "icc"
		check_app([common['CC'], "-V"], "Intel C++ Compiler", "support is experimental")
		check_gcc(None, "", common, PACKAGE_GCC)
		check_binutils(None, binutils_prefix, common, PACKAGE_BINUTILS)
	
	if (config['COMPILER'] == "suncc"):
		common['CC'] = "suncc"
		check_app([common['CC'], "-V"], "Sun Studio Compiler", "support is experimental")
		check_gcc(None, "", common, PACKAGE_GCC)
		check_binutils(None, binutils_prefix, common, PACKAGE_BINUTILS)
	
	if (config['COMPILER'] == "clang"):
		common['CC'] = "clang"
		check_app([common['CC'], "--version"], "Clang compiler", "preferably version 1.0 or newer")
		check_gcc(None, "", common, PACKAGE_GCC)
		check_binutils(None, binutils_prefix, common, PACKAGE_BINUTILS)
	
	# Platform-specific utilities
	if ((config['BARCH'] == "amd64") or (config['BARCH'] == "ia32") or (config['BARCH'] == "ppc32") or (config['BARCH'] == "sparc64")):
		check_app(["mkisofs", "--version"], "ISO 9660 creation utility", "usually part of genisoimage")
	
	create_output(COMMON, common)
	
	return 0

if __name__ == '__main__':
	sys.exit(main())
