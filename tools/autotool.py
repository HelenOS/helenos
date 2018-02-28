#!/usr/bin/env python2
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
HEADER = 'common.h.new'
GUARD = '_AUTOTOOL_COMMON_H_'

PROBE_SOURCE = 'probe.c'
PROBE_OUTPUT = 'probe.s'

PACKAGE_BINUTILS = "usually part of binutils"
PACKAGE_GCC = "preferably version 4.7.0 or newer"
PACKAGE_CROSS = "use tools/toolchain.sh to build the cross-compiler toolchain"
PACKAGE_CLANG = "reasonably recent version of clang needs to be installed"

COMPILER_FAIL = "The compiler is probably not capable to compile HelenOS."
COMPILER_WARNING = "The compilation of HelenOS might fail."

PROBE_HEAD = """#define AUTOTOOL_DECLARE(category, tag, name, signedness, base, size, compatible) \\
	asm volatile ( \\
		"AUTOTOOL_DECLARE\\t" category "\\t" tag "\\t" name "\\t" signedness "\\t" base "\\t%[size_val]\\t%[cmp_val]\\n" \\
		: \\
		: [size_val] "n" (size), [cmp_val] "n" (compatible) \\
	)

#define STRING(arg)      STRING_ARG(arg)
#define STRING_ARG(arg)  #arg

#define DECLARE_BUILTIN_TYPE(tag, type) \\
	AUTOTOOL_DECLARE("unsigned long long int", tag, STRING(type), "unsigned", "long long", sizeof(type), __builtin_types_compatible_p(type, unsigned long long int)); \\
	AUTOTOOL_DECLARE("unsigned long int", tag, STRING(type), "unsigned", "long", sizeof(type), __builtin_types_compatible_p(type, unsigned long int)); \\
	AUTOTOOL_DECLARE("unsigned int", tag, STRING(type), "unsigned", "int", sizeof(type), __builtin_types_compatible_p(type, unsigned int)); \\
	AUTOTOOL_DECLARE("unsigned short int", tag, STRING(type), "unsigned", "short", sizeof(type), __builtin_types_compatible_p(type, unsigned short int)); \\
	AUTOTOOL_DECLARE("unsigned char", tag, STRING(type), "unsigned", "char", sizeof(type), __builtin_types_compatible_p(type, unsigned char)); \\
	AUTOTOOL_DECLARE("signed long long int", tag, STRING(type), "signed", "long long", sizeof(type), __builtin_types_compatible_p(type, signed long long int)); \\
	AUTOTOOL_DECLARE("signed long int", tag, STRING(type), "signed", "long", sizeof(type), __builtin_types_compatible_p(type, signed long int)); \\
	AUTOTOOL_DECLARE("signed int", tag, STRING(type), "signed", "int", sizeof(type), __builtin_types_compatible_p(type, signed int)); \\
	AUTOTOOL_DECLARE("signed short int", tag, STRING(type), "signed", "short", sizeof(type), __builtin_types_compatible_p(type, signed short int)); \\
	AUTOTOOL_DECLARE("signed char", tag, STRING(type), "signed", "char", sizeof(type), __builtin_types_compatible_p(type, signed char)); \\
	AUTOTOOL_DECLARE("pointer", tag, STRING(type), "N/A", "pointer", sizeof(type), __builtin_types_compatible_p(type, void*)); \\
	AUTOTOOL_DECLARE("long double", tag, STRING(type), "signed", "long double", sizeof(type), __builtin_types_compatible_p(type, long double)); \\
	AUTOTOOL_DECLARE("double", tag, STRING(type), "signed", "double", sizeof(type), __builtin_types_compatible_p(type, double)); \\
	AUTOTOOL_DECLARE("float", tag, STRING(type), "signed", "float", sizeof(type), __builtin_types_compatible_p(type, float));

extern int main(int, char *[]);

int main(int argc, char *argv[])
{
"""

PROBE_TAIL = """}
"""

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

def print_warning(msg):
	"Print a bold error message"

	sys.stderr.write("\n")
	sys.stderr.write("######################################################################\n")
	sys.stderr.write("HelenOS build sanity check warning:\n")
	sys.stderr.write("\n")
	sys.stderr.write("%s\n" % "\n".join(msg))
	sys.stderr.write("######################################################################\n")
	sys.stderr.write("\n")

	time.sleep(5)

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
	gnu_target = None
	helenos_target = None
	target = None
	cc_args = []

	if (config['PLATFORM'] == "abs32le"):
		check_config(config, "CROSS_TARGET")
		platform = config['CROSS_TARGET']

		if (config['CROSS_TARGET'] == "arm32"):
			gnu_target = "arm-linux-gnueabi"
			helenos_target = "arm-helenos-gnueabi"

		if (config['CROSS_TARGET'] == "ia32"):
			gnu_target = "i686-pc-linux-gnu"
			helenos_target = "i686-pc-helenos"

		if (config['CROSS_TARGET'] == "mips32"):
			cc_args.append("-mabi=32")
			gnu_target = "mipsel-linux-gnu"
			helenos_target = "mipsel-helenos"

	if (config['PLATFORM'] == "amd64"):
		platform = config['PLATFORM']
		gnu_target = "amd64-unknown-elf"
		helenos_target = "amd64-helenos"

	if (config['PLATFORM'] == "arm32"):
		platform = config['PLATFORM']
		gnu_target = "arm-linux-gnueabi"
		helenos_target = "arm-helenos-gnueabi"

	if (config['PLATFORM'] == "ia32"):
		platform = config['PLATFORM']
		gnu_target = "i686-pc-linux-gnu"
		helenos_target = "i686-pc-helenos"

	if (config['PLATFORM'] == "ia64"):
		platform = config['PLATFORM']
		gnu_target = "ia64-pc-linux-gnu"
		helenos_target = "ia64-pc-helenos"

	if (config['PLATFORM'] == "mips32"):
		check_config(config, "MACHINE")
		cc_args.append("-mabi=32")

		if ((config['MACHINE'] == "msim") or (config['MACHINE'] == "lmalta")):
			platform = config['PLATFORM']
			gnu_target = "mipsel-linux-gnu"
			helenos_target = "mipsel-helenos"

		if ((config['MACHINE'] == "bmalta")):
			platform = "mips32eb"
			gnu_target = "mips-linux-gnu"
			helenos_target = "mips-helenos"

	if (config['PLATFORM'] == "mips64"):
		check_config(config, "MACHINE")
		cc_args.append("-mabi=64")

		if (config['MACHINE'] == "msim"):
			platform = config['PLATFORM']
			gnu_target = "mips64el-linux-gnu"
			helenos_target = "mips64el-helenos"

	if (config['PLATFORM'] == "ppc32"):
		platform = config['PLATFORM']
		gnu_target = "ppc-linux-gnu"
		helenos_target = "ppc-helenos"

	if (config['PLATFORM'] == "riscv64"):
		platform = config['PLATFORM']
		gnu_target = "riscv64-unknown-linux-gnu"
		helenos_target = "riscv64-helenos"

	if (config['PLATFORM'] == "sparc64"):
		platform = config['PLATFORM']
		gnu_target = "sparc64-linux-gnu"
		helenos_target = "sparc64-helenos"

	if (config['COMPILER'] == "gcc_helenos"):
		target = helenos_target
	else:
		target = gnu_target

	return (platform, cc_args, target)

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

	if (not path is None):
		common['CLANG'] = "%s/%s" % (path, common['CLANG'])

	check_app([common['CLANG'], "--version"], "clang", details)

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

def check_python():
	"Check for Python dependencies"

	try:
		sys.stderr.write("Checking for PyYAML ... ")
		import yaml
	except ImportError:
		print_error(["PyYAML is missing.",
		             "",
		             "Please make sure that it is installed in your",
		             "system (usually part of PyYAML package)."])

	sys.stderr.write("ok\n")

def decode_value(value):
	"Decode integer value"

	base = 10

	if ((value.startswith('$')) or (value.startswith('#'))):
		value = value[1:]

	if (value.startswith('0x')):
		value = value[2:]
		base = 16

	return int(value, base)

def probe_compiler(common, typesizes):
	"Generate, compile and parse probing source"

	check_common(common, "CC")

	outf = open(PROBE_SOURCE, 'w')
	outf.write(PROBE_HEAD)

	for typedef in typesizes:
		if 'def' in typedef:
			outf.write("#ifdef %s\n" % typedef['def'])
		outf.write("\tDECLARE_BUILTIN_TYPE(\"%s\", %s);\n" % (typedef['tag'], typedef['type']))
		if 'def' in typedef:
			outf.write("#endif\n")

	outf.write(PROBE_TAIL)
	outf.close()

	args = common['CC_AUTOGEN'].split(' ')
	args.extend(["-S", "-o", PROBE_OUTPUT, PROBE_SOURCE])

	try:
		sys.stderr.write("Checking compiler properties ... ")
		output = subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.PIPE).communicate()
	except:
		sys.stderr.write("failed\n")
		print_error(["Error executing \"%s\"." % " ".join(args),
		             "Make sure that the compiler works properly."])

	if (not os.path.isfile(PROBE_OUTPUT)):
		sys.stderr.write("failed\n")
		print(output[1])
		print_error(["Error executing \"%s\"." % " ".join(args),
		             "The compiler did not produce the output file \"%s\"." % PROBE_OUTPUT,
		             "",
		             output[0],
		             output[1]])

	sys.stderr.write("ok\n")

	inf = open(PROBE_OUTPUT, 'r')
	lines = inf.readlines()
	inf.close()

	builtins = {}

	for j in range(len(lines)):
		tokens = lines[j].strip().split("\t")

		if (len(tokens) > 0):
			if (tokens[0] == "AUTOTOOL_DECLARE"):
				if (len(tokens) < 8):
					print_error(["Malformed declaration in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])

				category = tokens[1]
				tag = tokens[2]
				name = tokens[3]
				signedness = tokens[4]
				base = tokens[5]
				size = tokens[6]
				compatible = tokens[7]

				try:
					compatible_int = decode_value(compatible)
					size_int = decode_value(size)
				except:
					print_error(["Integer value expected in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])

				if (compatible_int == 1):
					builtins[tag] = {
						'tag': tag,
						'name': name,
						'sign': signedness,
						'base': base,
						'size': size_int,
					}

	for typedef in typesizes:
		if not typedef['tag'] in builtins:
			print_error(['Unable to determine the properties of type %s.' % typedef['tag'],
		             COMPILER_FAIL])
		if 'sname' in typedef:
			builtins[typedef['tag']]['sname'] = typedef['sname']

	return builtins

def get_suffix(type):
	if type['sign'] == 'unsigned':
		return {
			"char": "",
			"short": "",
			"int": "U",
			"long": "UL",
			"long long": "ULL",
		}[type['base']]
	else:
		return {
			"char": "",
			"short": "",
			"int": "",
			"long": "L",
			"long long": "LL",
		}[type['base']]

def get_max(type):
	val = (1 << (type['size']*8 - 1))
	if type['sign'] == 'unsigned':
		val *= 2
	return val - 1

def detect_sizes(probe):
	"Detect properties of builtin types"

	macros = {}

	for type in probe.values():
		macros['__SIZEOF_%s__' % type['tag']] = type['size']

		if ('sname' in type):
			macros['__%s_TYPE__'  % type['sname']] = type['name']
			macros['__%s_WIDTH__' % type['sname']] = type['size']*8
			macros['__%s_%s__' % (type['sname'], type['sign'].upper())] = "1"
			macros['__%s_C_SUFFIX__' % type['sname']] = get_suffix(type)
			macros['__%s_MAX__' % type['sname']] = "%d%s" % (get_max(type), get_suffix(type))

	if (probe['SIZE_T']['sign'] != 'unsigned'):
		print_error(['The type size_t is not unsigned.', COMPILER_FAIL])

	return macros

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

	# HelenOS cross-compiler prefix
	if ('CROSS_HELENOS_PREFIX' in os.environ):
		cross_helenos_prefix = os.environ['CROSS_HELENOS_PREFIX']
	else:
		cross_helenos_prefix = "/usr/local/cross-helenos"

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
		check_app(["unzip"], "unzip utility", "usually part of zip/unzip utilities")

		platform, cc_args, target = get_target(config)

		if (platform is None) or (target is None):
			print_error(["Unsupported compiler target.",
				     "Please contact the developers of HelenOS."])

		path = "%s/%s/bin" % (cross_prefix, target)

		# Compatibility with earlier toolchain paths.
		if not os.path.exists(path):
			if (config['COMPILER'] == "gcc_helenos"):
				check_path = "%s/%s/%s" % (cross_helenos_prefix, platform, target)
				if not os.path.exists(check_path):
					print_error(["Toolchain for target is not installed, or CROSS_PREFIX is not set correctly."])
				path = "%s/%s/bin" % (cross_helenos_prefix, platform)
			else:
				check_path = "%s/%s/%s" % (cross_prefix, platform, target)
				if not os.path.exists(check_path):
					print_error(["Toolchain for target is not installed, or CROSS_PREFIX is not set correctly."])
				path = "%s/%s/bin" % (cross_prefix, platform)

		common['TARGET'] = target
		prefix = "%s-" % target

		# Compiler
		if (config['COMPILER'] == "gcc_cross" or config['COMPILER'] == "gcc_helenos"):
			check_gcc(path, prefix, common, PACKAGE_CROSS)
			check_binutils(path, prefix, common, PACKAGE_CROSS)

			check_common(common, "GCC")
			common['CC'] = " ".join([common['GCC']] + cc_args)
			common['CC_AUTOGEN'] = common['CC']

		if (config['COMPILER'] == "gcc_native"):
			check_gcc(None, "", common, PACKAGE_GCC)
			check_binutils(None, binutils_prefix, common, PACKAGE_BINUTILS)

			check_common(common, "GCC")
			common['CC'] = common['GCC']
			common['CC_AUTOGEN'] = common['CC']

		if (config['COMPILER'] == "clang"):
			check_binutils(path, prefix, common, PACKAGE_CROSS)
			check_clang(path, prefix, common, PACKAGE_CLANG)

			check_common(common, "CLANG")
			common['CC'] = " ".join([common['CLANG']] + cc_args)
			common['CC_AUTOGEN'] = common['CC'] + " -no-integrated-as"

			if (config['INTEGRATED_AS'] == "yes"):
				common['CC'] += " -integrated-as"

			if (config['INTEGRATED_AS'] == "no"):
				common['CC'] += " -no-integrated-as"

		check_python()

		# Platform-specific utilities
		if ((config['BARCH'] == "amd64") or (config['BARCH'] == "ia32") or (config['BARCH'] == "ppc32") or (config['BARCH'] == "sparc64")):
			common['GENISOIMAGE'] = check_app_alternatives(["genisoimage", "mkisofs", "xorriso"], ["--version"], "ISO 9660 creation utility", "usually part of genisoimage")
			if common['GENISOIMAGE'] == 'xorriso':
				common['GENISOIMAGE'] += ' -as genisoimage'

		probe = probe_compiler(common,
			[
				{'type': 'long long int', 'tag': 'LONG_LONG', 'sname': 'LLONG' },
				{'type': 'long int', 'tag': 'LONG', 'sname': 'LONG' },
				{'type': 'int', 'tag': 'INT', 'sname': 'INT' },
				{'type': 'short int', 'tag': 'SHORT', 'sname': 'SHRT'},
				{'type': 'void*', 'tag': 'POINTER'},
				{'type': 'long double', 'tag': 'LONG_DOUBLE'},
				{'type': 'double', 'tag': 'DOUBLE'},
				{'type': 'float', 'tag': 'FLOAT'},
				{'type': '__SIZE_TYPE__', 'tag': 'SIZE_T', 'def': '__SIZE_TYPE__', 'sname': 'SIZE' },
				{'type': '__PTRDIFF_TYPE__', 'tag': 'PTRDIFF_T', 'def': '__PTRDIFF_TYPE__', 'sname': 'PTRDIFF' },
				{'type': '__WINT_TYPE__', 'tag': 'WINT_T', 'def': '__WINT_TYPE__', 'sname': 'WINT' },
				{'type': '__WCHAR_TYPE__', 'tag': 'WCHAR_T', 'def': '__WCHAR_TYPE__', 'sname': 'WCHAR' },
				{'type': '__INTMAX_TYPE__', 'tag': 'INTMAX_T', 'def': '__INTMAX_TYPE__', 'sname': 'INTMAX' },
				{'type': 'unsigned __INTMAX_TYPE__', 'tag': 'UINTMAX_T', 'def': '__INTMAX_TYPE__', 'sname': 'UINTMAX' },
			]
		)

		macros = detect_sizes(probe)

	finally:
		sandbox_leave(owd)

	common['AUTOGEN'] = "%s/autogen.py" % os.path.dirname(os.path.abspath(sys.argv[0]))

	create_makefile(MAKEFILE, common)
	create_header(HEADER, macros)

	return 0

if __name__ == '__main__':
	sys.exit(main())
