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

PACKAGE_CROSS = "use tools/toolchain.sh to build the cross-compiler toolchain"
PACKAGE_CLANG = "reasonably recent version of clang needs to be installed"

def read_config(fname, config):
	"Read HelenOS build configuration"

	inf = open(fname, 'r')

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

def get_target(config):
	platform = None
	target = None

	if (config['PLATFORM'] == "abs32le"):
		check_config(config, "CROSS_TARGET")
		platform = config['CROSS_TARGET']

		if (config['CROSS_TARGET'] == "arm32"):
			target = "arm-helenos"

		if (config['CROSS_TARGET'] == "ia32"):
			target = "i686-helenos"

		if (config['CROSS_TARGET'] == "mips32"):
			target = "mipsel-helenos"

	if (config['PLATFORM'] == "amd64"):
		platform = config['PLATFORM']
		target = "amd64-helenos"

	if (config['PLATFORM'] == "arm32"):
		platform = config['PLATFORM']
		target = "arm-helenos"

	if (config['PLATFORM'] == "arm64"):
		platform = config['PLATFORM']
		target = "aarch64-helenos"

	if (config['PLATFORM'] == "ia32"):
		platform = config['PLATFORM']
		target = "i686-helenos"

	if (config['PLATFORM'] == "ia64"):
		platform = config['PLATFORM']
		target = "ia64-helenos"

	if (config['PLATFORM'] == "mips32"):
		check_config(config, "MACHINE")

		if ((config['MACHINE'] == "msim") or (config['MACHINE'] == "lmalta")):
			platform = config['PLATFORM']
			target = "mipsel-helenos"

		if ((config['MACHINE'] == "bmalta")):
			platform = "mips32eb"
			target = "mips-helenos"

	if (config['PLATFORM'] == "mips64"):
		check_config(config, "MACHINE")

		if (config['MACHINE'] == "msim"):
			platform = config['PLATFORM']
			target = "mips64el-helenos"

	if (config['PLATFORM'] == "ppc32"):
		platform = config['PLATFORM']
		target = "ppc-helenos"

	if (config['PLATFORM'] == "riscv64"):
		platform = config['PLATFORM']
		target = "riscv64-helenos"

	if (config['PLATFORM'] == "sparc64"):
		platform = config['PLATFORM']
		target = "sparc64-helenos"

	return (platform, target)

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

def check_path_gcc(target):
	"Check whether GCC for a given target is present in $PATH."

	try:
		subprocess.Popen([ "%s-gcc" % target, "--version" ], stdout = subprocess.PIPE, stderr = subprocess.PIPE).wait()
		return True
	except:
		return False

def check_app_alternatives(alts, args, name, details):
	"Check whether an application can be executed (use several alternatives)"

	tried = []
	found = None

	for alt in alts:
		working = True
		cmdline = [alt] + args
		tried.append(" ".join(cmdline))

		try:
			sys.stderr.write("Checking for %s ... " % alt)
			subprocess.Popen(cmdline, stdout = subprocess.PIPE, stderr = subprocess.PIPE).wait()
		except:
			sys.stderr.write("failed\n")
			working = False

		if (working):
			sys.stderr.write("ok\n")
			found = alt
			break

	if (found is None):
		print_error(["%s is missing." % name,
		             "",
		             "Please make sure that it is installed in your",
		             "system (%s)." % details,
		             "",
		             "The following alternatives were tried:"] + tried)

	return found

def check_clang(path, prefix, common, details):
	"Check for clang"

	common['CLANG'] = "%sclang" % prefix
	common['CLANGXX'] = "%sclang++" % prefix

	if (not path is None):
		common['CLANG'] = "%s/%s" % (path, common['CLANG'])
		common['CLANGXX'] = "%s/%s" % (path, common['CLANGXX'])

	check_app([common['CLANG'], "--version"], "clang", details)

def check_gcc(path, prefix, common, details):
	"Check for GCC"

	common['GCC'] = "%sgcc" % prefix
	common['GXX'] = "%sg++" % prefix

	if (not path is None):
		common['GCC'] = "%s/%s" % (path, common['GCC'])
		common['GXX'] = "%s/%s" % (path, common['GXX'])

	check_app([common['GCC'], "--version"], "GNU GCC", details)

def check_libgcc(common):
	sys.stderr.write("Checking for libgcc.a ... ")
	libgcc_path = None
	proc = subprocess.Popen([ common['GCC'], "-print-search-dirs" ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
	for line in proc.stdout:
		line = line.decode('utf-8').strip('\n')
		parts = line.split()
		if parts[0] == "install:":
			p = parts[1] + "libgcc.a"
			if os.path.isfile(p):
				libgcc_path = p
	proc.wait()

	if libgcc_path is None:
		sys.stderr.write("failed\n")
		print_error(["Unable to find gcc library (libgcc.a).",
					"",
					"Please ensure that you have installed the",
					"toolchain properly."])

	sys.stderr.write("ok\n")
	common['LIBGCC_PATH'] = libgcc_path


def check_binutils(path, prefix, common, details):
	"Check for binutils toolchain"

	common['AS'] = "%sas" % prefix
	common['LD'] = "%sld" % prefix
	common['AR'] = "%sar" % prefix
	common['OBJCOPY'] = "%sobjcopy" % prefix
	common['OBJDUMP'] = "%sobjdump" % prefix
	common['STRIP'] = "%sstrip" % prefix

	if (not path is None):
		for key in ["AS", "LD", "AR", "OBJCOPY", "OBJDUMP", "STRIP"]:
			common[key] = "%s/%s" % (path, common[key])

	check_app([common['AS'], "--version"], "GNU Assembler", details)
	check_app([common['LD'], "--version"], "GNU Linker", details)
	check_app([common['AR'], "--version"], "GNU Archiver", details)
	check_app([common['OBJCOPY'], "--version"], "GNU Objcopy utility", details)
	check_app([common['OBJDUMP'], "--version"], "GNU Objdump utility", details)
	check_app([common['STRIP'], "--version"], "GNU strip", details)

def create_makefile(mkname, common):
	"Create makefile output"

	outmk = open(mkname, 'w')

	outmk.write('#########################################\n')
	outmk.write('## AUTO-GENERATED FILE, DO NOT EDIT!!! ##\n')
	outmk.write('## Generated by: tools/autotool.py     ##\n')
	outmk.write('#########################################\n\n')

	for key, value in common.items():
		if (type(value) is list):
			outmk.write('%s = %s\n' % (key, " ".join(value)))
		else:
			outmk.write('%s = %s\n' % (key, value))

	outmk.close()

def create_header(hdname, macros):
	"Create header output"

	outhd = open(hdname, 'w')

	outhd.write('/***************************************\n')
	outhd.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	outhd.write(' * Generated by: tools/autotool.py     *\n')
	outhd.write(' ***************************************/\n\n')

	outhd.write('#ifndef %s\n' % GUARD)
	outhd.write('#define %s\n\n' % GUARD)

	for macro in sorted(macros):
		outhd.write('#ifndef %s\n' % macro)
		outhd.write('#define %s  %s\n' % (macro, macros[macro]))
		outhd.write('#endif\n\n')

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
		cross_prefix = "/usr/local/cross"

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
		check_app(["unzip"], "unzip utility", "usually part of zip/unzip utilities")
		check_app(["tar", "--version"], "tar utility", "usually part of tar")

		platform, target = get_target(config)

		if (platform is None) or (target is None):
			print_error(["Unsupported compiler target.",
				     "Please contact the developers of HelenOS."])

		path = None

		if not check_path_gcc(target):
			path = "%s/bin" % cross_prefix

		common['TARGET'] = target
		prefix = "%s-" % target

		cc_autogen = None

		# We always need to check for GCC as we
		# need libgcc
		check_gcc(path, prefix, common, PACKAGE_CROSS)

		# Compiler
		if (config['COMPILER'] == "gcc_cross"):
			check_binutils(path, prefix, common, PACKAGE_CROSS)

			check_common(common, "GCC")
			common['CC'] = common['GCC']
			cc_autogen = common['CC']

			check_common(common, "GXX")
			common['CXX'] = common['GXX']

		if (config['COMPILER'] == "clang"):
			check_binutils(path, prefix, common, PACKAGE_CROSS)
			check_clang(path, prefix, common, PACKAGE_CLANG)

			check_common(common, "CLANG")
			common['CC'] = common['CLANG']
			common['CXX'] = common['CLANGXX']
			cc_autogen = common['CC'] + " -no-integrated-as"

			if (config['INTEGRATED_AS'] == "yes"):
				common['CC'] += " -integrated-as"
				common['CXX'] += " -integrated-as"

			if (config['INTEGRATED_AS'] == "no"):
				common['CC'] += " -no-integrated-as"
				common['CXX'] += " -no-integrated-as"

		# Find full path to libgcc
		check_libgcc(common)

		# Platform-specific utilities
		if (config['BARCH'] in ('amd64', 'arm64', 'ia32', 'ppc32', 'sparc64')):
			common['GENISOIMAGE'] = check_app_alternatives(["genisoimage", "mkisofs", "xorriso"], ["--version"], "ISO 9660 creation utility", "usually part of genisoimage")
			if common['GENISOIMAGE'] == 'xorriso':
				common['GENISOIMAGE'] += ' -as genisoimage'

	finally:
		sandbox_leave(owd)

	create_makefile(MAKEFILE, common)

	return 0

if __name__ == '__main__':
	sys.exit(main())
