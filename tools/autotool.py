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
import shutil
import re
import time
import subprocess

SANDBOX = 'autotool'
CONFIG = 'Makefile.config'
MAKEFILE = 'Makefile.common'
HEADER = 'common.h'
GUARD = 'AUTOTOOL_COMMON_H_'

PROBE_SOURCE = 'probe.c'
PROBE_OUTPUT = 'probe.s'

PACKAGE_BINUTILS = "usually part of binutils"
PACKAGE_GCC = "preferably version 4.4.3 or newer"
PACKAGE_CROSS = "use tools/toolchain.sh to build the cross-compiler toolchain"

COMPILER_FAIL = "The compiler is probably not capable to compile HelenOS."

PROBE_HEAD = """#define AUTOTOOL_DECLARE(category, subcategory, name, value) \\
	asm volatile ( \\
		"AUTOTOOL_DECLARE\\t" category "\\t" subcategory "\\t" name "\\t%[val]\\n" \\
		: \\
		: [val] "n" (value) \\
	)

#define DECLARE_INTSIZE(type) \\
	AUTOTOOL_DECLARE("intsize", "unsigned", #type, sizeof(unsigned type)); \\
	AUTOTOOL_DECLARE("intsize", "signed", #type, sizeof(signed type))

int main(int argc, char *argv[])
{
"""

PROBE_TAIL = """}
"""

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

def sandbox_enter():
	"Create a temporal sandbox directory for running tests"
	
	if (os.path.exists(SANDBOX)):
		if (os.path.isdir(SANDBOX)):
			try:
				shutil.rmtree(SANDBOX)
			except:
				print_error(["Unable to cleanup the directory \"%s\"." % SANDBOX])
		else:
			print_error(["Please inspect and remove unexpected directory,",
			             "entry \"%s\"." % SANDBOX])
	
	try:
		os.mkdir(SANDBOX)
	except:
		print_error(["Unable to create sandbox directory \"%s\"." % SANDBOX])
	
	owd = os.getcwd()
	os.chdir(SANDBOX)
	
	return owd

def sandbox_leave(owd):
	"Leave the temporal sandbox directory"
	
	os.chdir(owd)

def check_config(config, key):
	"Check whether the configuration key exists"
	
	if (not key in config):
		print_error(["Build configuration of HelenOS does not contain %s." % key,
		             "Try running \"make config\" again.",
		             "If the problem persists, please contact the developers of HelenOS."])

def check_common(common, key):
	"Check whether the common key exists"
	
	if (not key in common):
		print_error(["Failed to determine the value %s." % key,
		             "Please contact the developers of HelenOS."])

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

def probe_compiler(common, sizes):
	"Generate, compile and parse probing source"
	
	check_common(common, "CC")
	
	outf = file(PROBE_SOURCE, 'w')
	outf.write(PROBE_HEAD)
	
	for typedef in sizes:
		outf.write("\tDECLARE_INTSIZE(%s);\n" % typedef)
	
	outf.write(PROBE_TAIL)
	outf.close()
	
	args = [common['CC'], "-S", "-o", PROBE_OUTPUT, PROBE_SOURCE]
	
	try:
		sys.stderr.write("Checking compiler properties ... ")
		output = subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.PIPE).communicate()
	except:
		sys.stderr.write("failed\n")
		print_error(["Error executing \"%s\"." % " ".join(args),
		             "Make sure that the compiler works properly."])
	
	if (not os.path.isfile(PROBE_OUTPUT)):
		sys.stderr.write("failed\n")
		print output[1]
		print_error(["Error executing \"%s\"." % " ".join(args),
		             "The compiler did not produce the output file \"%s\"." % PROBE_OUTPUT,
		             "",
		             output[0],
		             output[1]])
	
	sys.stderr.write("ok\n")
	
	inf = file(PROBE_OUTPUT, 'r')
	lines = inf.readlines()
	inf.close()
	
	unsigned_sizes = {}
	signed_sizes = {}
	
	for j in range(len(lines)):
		tokens = lines[j].strip().split("\t")
		
		if (len(tokens) > 0):
			if (tokens[0] == "AUTOTOOL_DECLARE"):
				if (len(tokens) < 5):
					print_error(["Malformed declaration in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
				
				category = tokens[1]
				subcategory = tokens[2]
				name = tokens[3]
				value = tokens[4]
				
				if (category == "intsize"):
					base = 10
					
					if ((value.startswith('$')) or (value.startswith('#'))):
						value = value[1:]
					
					if (value.startswith('0x')):
						value = value[2:]
						base = 16
					
					try:
						value_int = int(value, base)
					except:
						print_error(["Integer value expected in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
					
					if (subcategory == "unsigned"):
						unsigned_sizes[name] = value_int
					elif (subcategory == "signed"):
						signed_sizes[name] = value_int
					else:
						print_error(["Unexpected keyword \"%s\" in \"%s\" on line %s." % (subcategory, PROBE_OUTPUT, j), COMPILER_FAIL])
	
	return {'unsigned_sizes' : unsigned_sizes, 'signed_sizes' : signed_sizes}

def detect_uints(unsigned_sizes, signed_sizes, bytes):
	"Detect correct types for fixed-size integer types"
	
	typedefs = []
	
	for b in bytes:
		fnd = False
		newtype = "uint%s_t" % (b * 8)
		
		for name, value in unsigned_sizes.items():
			if (value == b):
				oldtype = "unsigned %s" % name
				typedefs.append({'oldtype' : oldtype, 'newtype' : newtype})
				fnd = True
				break
		
		if (not fnd):
			print_error(['Unable to find appropriate integer type for %s' % newtype,
			             COMPILER_FAIL])
		
		
		fnd = False
		newtype = "int%s_t" % (b * 8)
		
		for name, value in signed_sizes.items():
			if (value == b):
				oldtype = "signed %s" % name
				typedefs.append({'oldtype' : oldtype, 'newtype' : newtype})
				fnd = True
				break
		
		if (not fnd):
			print_error(['Unable to find appropriate integer type for %s' % newtype,
			             COMPILER_FAIL])
	
	return typedefs

def create_makefile(mkname, common):
	"Create makefile output"
	
	outmk = file(mkname, 'w')
	
	outmk.write('#########################################\n')
	outmk.write('## AUTO-GENERATED FILE, DO NOT EDIT!!! ##\n')
	outmk.write('#########################################\n\n')
	
	for key, value in common.items():
		outmk.write('%s = %s\n' % (key, value))
	
	outmk.close()

def create_header(hdname, typedefs):
	"Create header output"
	
	outhd = file(hdname, 'w')
	
	outhd.write('/***************************************\n')
	outhd.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	outhd.write(' ***************************************/\n\n')
	
	outhd.write('#ifndef %s\n' % GUARD)
	outhd.write('#define %s\n\n' % GUARD)
	
	for typedef in typedefs:
		outhd.write('typedef %s %s;\n' % (typedef['oldtype'], typedef['newtype']))
	
	outhd.write('\n#endif\n')
	outhd.close()

def main():
	config = {}
	common = {}
	
	# Read and check configuration
	if os.path.exists(CONFIG):
		read_config(CONFIG, config)
	else:
		print_error(["Configuration file %s not found! Make sure that the" % CONFIG,
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
	
	owd = sandbox_enter()
	
	try:
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
			
			check_common(common, "GCC")
			common['CC'] = common['GCC']
		
		if (config['COMPILER'] == "gcc_native"):
			check_gcc(None, "", common, PACKAGE_GCC)
			check_binutils(None, binutils_prefix, common, PACKAGE_BINUTILS)
			
			check_common(common, "GCC")
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
		
		probe = probe_compiler(common,
			[
				"char",
				"short int",
				"int",
				"long int",
				"long long int",
			]
		)
		
		typedefs = detect_uints(probe['unsigned_sizes'], probe['signed_sizes'], [1, 2, 4, 8])
		
	finally:
		sandbox_leave(owd)
	
	create_makefile(MAKEFILE, common)
	create_header(HEADER, typedefs)
	
	return 0

if __name__ == '__main__':
	sys.exit(main())
