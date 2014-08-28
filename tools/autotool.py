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
PACKAGE_GCC = "preferably version 4.7.0 or newer"
PACKAGE_CROSS = "use tools/toolchain.sh to build the cross-compiler toolchain"

COMPILER_FAIL = "The compiler is probably not capable to compile HelenOS."
COMPILER_WARNING = "The compilation of HelenOS might fail."

PROBE_HEAD = """#define AUTOTOOL_DECLARE(category, subcategory, tag, name, strc, conc, value) \\
	asm volatile ( \\
		"AUTOTOOL_DECLARE\\t" category "\\t" subcategory "\\t" tag "\\t" name "\\t" strc "\\t" conc "\\t%[val]\\n" \\
		: \\
		: [val] "n" (value) \\
	)

#define STRING(arg)      STRING_ARG(arg)
#define STRING_ARG(arg)  #arg

#define DECLARE_BUILTIN_TYPE(tag, type) \\
	AUTOTOOL_DECLARE("builtin_size", "", tag, STRING(type), "", "", sizeof(type)); \\
	AUTOTOOL_DECLARE("builtin_sign", "unsigned long long int", tag, STRING(type), "unsigned", "", __builtin_types_compatible_p(type, unsigned long long int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "unsigned long int", tag, STRING(type), "unsigned", "", __builtin_types_compatible_p(type, unsigned long int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "unsigned int", tag, STRING(type), "unsigned", "", __builtin_types_compatible_p(type, unsigned int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "unsigned short int", tag, STRING(type), "unsigned", "", __builtin_types_compatible_p(type, unsigned short int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "unsigned char", tag, STRING(type), "unsigned", "", __builtin_types_compatible_p(type, unsigned char)); \\
	AUTOTOOL_DECLARE("builtin_sign", "signed long long int", tag, STRING(type), "signed", "", __builtin_types_compatible_p(type, signed long long int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "signed long int", tag, STRING(type), "signed", "", __builtin_types_compatible_p(type, signed long int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "signed int", tag, STRING(type), "signed", "", __builtin_types_compatible_p(type, signed int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "signed short int", tag, STRING(type), "signed", "", __builtin_types_compatible_p(type, signed short int)); \\
	AUTOTOOL_DECLARE("builtin_sign", "signed char", tag, STRING(type), "signed", "", __builtin_types_compatible_p(type, signed char));

#define DECLARE_INTSIZE(tag, type, strc, conc) \\
	AUTOTOOL_DECLARE("intsize", "unsigned", tag, #type, strc, conc, sizeof(unsigned type)); \\
	AUTOTOOL_DECLARE("intsize", "signed", tag, #type, strc, conc, sizeof(signed type));

#define DECLARE_FLOATSIZE(tag, type) \\
	AUTOTOOL_DECLARE("floatsize", "", tag, #type, "", "", sizeof(type));

int main(int argc, char *argv[])
{
#ifdef __SIZE_TYPE__
	DECLARE_BUILTIN_TYPE("size", __SIZE_TYPE__);
#endif
#ifdef __WCHAR_TYPE__
	DECLARE_BUILTIN_TYPE("wchar", __WCHAR_TYPE__);
#endif
#ifdef __WINT_TYPE__
	DECLARE_BUILTIN_TYPE("wint", __WINT_TYPE__);
#endif
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
	target = None
	gnu_target = None
	clang_target = None
	helenos_target = None
	cc_args = []
	
	if (config['PLATFORM'] == "abs32le"):
		check_config(config, "CROSS_TARGET")
		target = config['CROSS_TARGET']
		
		if (config['CROSS_TARGET'] == "arm32"):
			gnu_target = "arm-linux-gnueabi"
			clang_target = "arm-unknown-linux"
			helenos_target = "arm-helenos-gnueabi"
		
		if (config['CROSS_TARGET'] == "ia32"):
			gnu_target = "i686-pc-linux-gnu"
			clang_target = "i386-unknown-linux"
			helenos_target = "i686-pc-helenos"
		
		if (config['CROSS_TARGET'] == "mips32"):
			gnu_target = "mipsel-linux-gnu"
			clang_target = "mipsel-unknown-linux"
			helenos_target = "mipsel-helenos"
			common['CC_ARGS'].append("-mabi=32")
	
	if (config['PLATFORM'] == "amd64"):
		target = config['PLATFORM']
		gnu_target = "amd64-linux-gnu"
		clang_target = "x86_64-unknown-linux"
		helenos_target = "amd64-helenos"
	
	if (config['PLATFORM'] == "arm32"):
		target = config['PLATFORM']
		gnu_target = "arm-linux-gnueabi"
		clang_target = "arm-unknown-linux"
		helenos_target = "arm-helenos-gnueabi"
	
	if (config['PLATFORM'] == "ia32"):
		target = config['PLATFORM']
		gnu_target = "i686-pc-linux-gnu"
		clang_target = "i386-unknown-linux"
		helenos_target = "i686-pc-helenos"
	
	if (config['PLATFORM'] == "ia64"):
		target = config['PLATFORM']
		gnu_target = "ia64-pc-linux-gnu"
		helenos_target = "ia64-pc-helenos"
	
	if (config['PLATFORM'] == "mips32"):
		check_config(config, "MACHINE")
		cc_args.append("-mabi=32")
		
		if ((config['MACHINE'] == "msim") or (config['MACHINE'] == "lmalta")):
			target = config['PLATFORM']
			gnu_target = "mipsel-linux-gnu"
			clang_target = "mipsel-unknown-linux"
			helenos_target = "mipsel-helenos"
		
		if ((config['MACHINE'] == "bmalta")):
			target = "mips32eb"
			gnu_target = "mips-linux-gnu"
			clang_target = "mips-unknown-linux"
			helenos_target = "mips-helenos"
	
	if (config['PLATFORM'] == "mips64"):
		check_config(config, "MACHINE")
		cc_args.append("-mabi=64")
		
		if (config['MACHINE'] == "msim"):
			target = config['PLATFORM']
			gnu_target = "mips64el-linux-gnu"
			clang_target = "mips64el-unknown-linux"
			helenos_target = "mips64el-helenos"
	
	if (config['PLATFORM'] == "ppc32"):
		target = config['PLATFORM']
		gnu_target = "ppc-linux-gnu"
		clang_target = "powerpc-unknown-linux"
		helenos_target = "ppc-helenos"
	
	if (config['PLATFORM'] == "sparc32"):
		target = config['PLATFORM'];
		gnu_target = "sparc-leon3-linux-gnu"
		helenos_target = "sparc-leon3-helenos"
	
	if (config['PLATFORM'] == "sparc64"):
		target = config['PLATFORM']
		gnu_target = "sparc64-linux-gnu"
		clang_target = "sparc-unknown-linux"
		helenos_target = "sparc64-helenos"
	
	return (target, cc_args, gnu_target, clang_target, helenos_target)

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

def probe_compiler(common, intsizes, floatsizes):
	"Generate, compile and parse probing source"
	
	check_common(common, "CC")
	
	outf = open(PROBE_SOURCE, 'w')
	outf.write(PROBE_HEAD)
	
	for typedef in intsizes:
		outf.write("\tDECLARE_INTSIZE(\"%s\", %s, %s, %s);\n" % (typedef['tag'], typedef['type'], typedef['strc'], typedef['conc']))
	
	for typedef in floatsizes:
		outf.write("\nDECLARE_FLOATSIZE(\"%s\", %s);\n" % (typedef['tag'], typedef['type']))
	
	outf.write(PROBE_TAIL)
	outf.close()
	
	args = [common['CC']]
	args.extend(common['CC_ARGS'])
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
	
	unsigned_sizes = {}
	signed_sizes = {}
	
	unsigned_tags = {}
	signed_tags = {}
	
	unsigned_strcs = {}
	signed_strcs = {}
	
	unsigned_concs = {}
	signed_concs = {}
	
	float_tags = {}
	
	builtin_sizes = {}
	builtin_signs = {}
	
	for j in range(len(lines)):
		tokens = lines[j].strip().split("\t")
		
		if (len(tokens) > 0):
			if (tokens[0] == "AUTOTOOL_DECLARE"):
				if (len(tokens) < 7):
					print_error(["Malformed declaration in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
				
				category = tokens[1]
				subcategory = tokens[2]
				tag = tokens[3]
				name = tokens[4]
				strc = tokens[5]
				conc = tokens[6]
				value = tokens[7]
				
				if (category == "intsize"):
					try:
						value_int = decode_value(value)
					except:
						print_error(["Integer value expected in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
					
					if (subcategory == "unsigned"):
						unsigned_sizes[value_int] = name
						unsigned_tags[tag] = value_int
						unsigned_strcs[value_int] = strc
						unsigned_concs[value_int] = conc
					elif (subcategory == "signed"):
						signed_sizes[value_int] = name
						signed_tags[tag] = value_int
						signed_strcs[value_int] = strc
						signed_concs[value_int] = conc
					else:
						print_error(["Unexpected keyword \"%s\" in \"%s\" on line %s." % (subcategory, PROBE_OUTPUT, j), COMPILER_FAIL])
				
				if (category == "floatsize"):
					try:
						value_int = decode_value(value)
					except:
						print_error(["Integer value expected in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
					
					float_tags[tag] = value_int
				
				if (category == "builtin_size"):
					try:
						value_int = decode_value(value)
					except:
						print_error(["Integer value expected in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
					
					builtin_sizes[tag] = {'name': name, 'value': value_int}
				
				if (category == "builtin_sign"):
					try:
						value_int = decode_value(value)
					except:
						print_error(["Integer value expected in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
					
					if (value_int == 1):
						if (not tag in builtin_signs):
							builtin_signs[tag] = strc;
						elif (builtin_signs[tag] != strc):
							print_error(["Inconsistent builtin type detection in \"%s\" on line %s." % (PROBE_OUTPUT, j), COMPILER_FAIL])
	
	return {'unsigned_sizes': unsigned_sizes, 'signed_sizes': signed_sizes, 'unsigned_tags': unsigned_tags, 'signed_tags': signed_tags, 'unsigned_strcs': unsigned_strcs, 'signed_strcs': signed_strcs, 'unsigned_concs': unsigned_concs, 'signed_concs': signed_concs, 'float_tags': float_tags, 'builtin_sizes': builtin_sizes, 'builtin_signs': builtin_signs}

def detect_sizes(probe, bytes, inttags, floattags):
	"Detect correct types for fixed-size types"
	
	macros = []
	typedefs = []
	
	for b in bytes:
		if (not b in probe['unsigned_sizes']):
			print_error(['Unable to find appropriate unsigned integer type for %u bytes.' % b,
			             COMPILER_FAIL])
		
		if (not b in probe['signed_sizes']):
			print_error(['Unable to find appropriate signed integer type for %u bytes.' % b,
			             COMPILER_FAIL])
		
		if (not b in probe['unsigned_strcs']):
			print_error(['Unable to find appropriate unsigned printf formatter for %u bytes.' % b,
			             COMPILER_FAIL])
		
		if (not b in probe['signed_strcs']):
			print_error(['Unable to find appropriate signed printf formatter for %u bytes.' % b,
			             COMPILER_FAIL])
		
		if (not b in probe['unsigned_concs']):
			print_error(['Unable to find appropriate unsigned literal macro for %u bytes.' % b,
			             COMPILER_FAIL])
		
		if (not b in probe['signed_concs']):
			print_error(['Unable to find appropriate signed literal macro for %u bytes.' % b,
			             COMPILER_FAIL])
		
		typedefs.append({'oldtype': "unsigned %s" % probe['unsigned_sizes'][b], 'newtype': "uint%u_t" % (b * 8)})
		typedefs.append({'oldtype': "signed %s" % probe['signed_sizes'][b], 'newtype': "int%u_t" % (b * 8)})
		
		macros.append({'oldmacro': "unsigned %s" % probe['unsigned_sizes'][b], 'newmacro': "UINT%u_T" % (b * 8)})
		macros.append({'oldmacro': "signed %s" % probe['signed_sizes'][b], 'newmacro': "INT%u_T" % (b * 8)})
		
		macros.append({'oldmacro': "\"%so\"" % probe['unsigned_strcs'][b], 'newmacro': "PRIo%u" % (b * 8)})
		macros.append({'oldmacro': "\"%su\"" % probe['unsigned_strcs'][b], 'newmacro': "PRIu%u" % (b * 8)})
		macros.append({'oldmacro': "\"%sx\"" % probe['unsigned_strcs'][b], 'newmacro': "PRIx%u" % (b * 8)})
		macros.append({'oldmacro': "\"%sX\"" % probe['unsigned_strcs'][b], 'newmacro': "PRIX%u" % (b * 8)})
		macros.append({'oldmacro': "\"%sd\"" % probe['signed_strcs'][b], 'newmacro': "PRId%u" % (b * 8)})
		
		name = probe['unsigned_concs'][b]
		if ((name.startswith('@')) or (name == "")):
			macros.append({'oldmacro': "c ## U", 'newmacro': "UINT%u_C(c)" % (b * 8)})
		else:
			macros.append({'oldmacro': "c ## U%s" % name, 'newmacro': "UINT%u_C(c)" % (b * 8)})
		
		name = probe['unsigned_concs'][b]
		if ((name.startswith('@')) or (name == "")):
			macros.append({'oldmacro': "c", 'newmacro': "INT%u_C(c)" % (b * 8)})
		else:
			macros.append({'oldmacro': "c ## %s" % name, 'newmacro': "INT%u_C(c)" % (b * 8)})
	
	for tag in inttags:
		newmacro = "U%s" % tag
		if (not tag in probe['unsigned_tags']):
			print_error(['Unable to find appropriate size macro for %s.' % newmacro,
			             COMPILER_FAIL])
		
		oldmacro = "UINT%s" % (probe['unsigned_tags'][tag] * 8)
		macros.append({'oldmacro': "%s_MIN" % oldmacro, 'newmacro': "%s_MIN" % newmacro})
		macros.append({'oldmacro': "%s_MAX" % oldmacro, 'newmacro': "%s_MAX" % newmacro})
		macros.append({'oldmacro': "1", 'newmacro': 'U%s_SIZE_%s' % (tag, probe['unsigned_tags'][tag] * 8)})
		
		newmacro = tag
		if (not tag in probe['signed_tags']):
			print_error(['Unable to find appropriate size macro for %s' % newmacro,
			             COMPILER_FAIL])
		
		oldmacro = "INT%s" % (probe['signed_tags'][tag] * 8)
		macros.append({'oldmacro': "%s_MIN" % oldmacro, 'newmacro': "%s_MIN" % newmacro})
		macros.append({'oldmacro': "%s_MAX" % oldmacro, 'newmacro': "%s_MAX" % newmacro})
		macros.append({'oldmacro': "1", 'newmacro': '%s_SIZE_%s' % (tag, probe['signed_tags'][tag] * 8)})
	
	for tag in floattags:
		if (not tag in probe['float_tags']):
			print_error(['Unable to find appropriate size macro for %s' % tag,
			             COMPILER_FAIL])
		
		macros.append({'oldmacro': "1", 'newmacro': '%s_SIZE_%s' % (tag, probe['float_tags'][tag] * 8)})
	
	if (not 'size' in probe['builtin_signs']):
		print_error(['Unable to determine whether size_t is signed or unsigned.',
		             COMPILER_FAIL])
	
	if (probe['builtin_signs']['size'] != 'unsigned'):
		print_error(['The type size_t is not unsigned.',
		             COMPILER_FAIL])
	
	fnd = True
	
	if (not 'wchar' in probe['builtin_sizes']):
		print_warning(['The compiler does not provide the macro __WCHAR_TYPE__',
		               'for defining the compiler-native type wchar_t. We are',
		               'forced to define wchar_t as a hardwired type int32_t.',
		               COMPILER_WARNING])
		fnd = False
	
	if (probe['builtin_sizes']['wchar']['value'] != 4):
		print_warning(['The compiler provided macro __WCHAR_TYPE__ for defining',
		               'the compiler-native type wchar_t is not compliant with',
		               'HelenOS. We are forced to define wchar_t as a hardwired',
		               'type int32_t.',
		               COMPILER_WARNING])
		fnd = False
	
	if (not fnd):
		macros.append({'oldmacro': "int32_t", 'newmacro': "wchar_t"})
	else:
		macros.append({'oldmacro': "__WCHAR_TYPE__", 'newmacro': "wchar_t"})
	
	if (not 'wchar' in probe['builtin_signs']):
		print_error(['Unable to determine whether wchar_t is signed or unsigned.',
		             COMPILER_FAIL])
	
	if (probe['builtin_signs']['wchar'] == 'unsigned'):
		macros.append({'oldmacro': "1", 'newmacro': 'WCHAR_IS_UNSIGNED'})
	if (probe['builtin_signs']['wchar'] == 'signed'):
		macros.append({'oldmacro': "1", 'newmacro': 'WCHAR_IS_SIGNED'})
	
	fnd = True
	
	if (not 'wint' in probe['builtin_sizes']):
		print_warning(['The compiler does not provide the macro __WINT_TYPE__',
		               'for defining the compiler-native type wint_t. We are',
		               'forced to define wint_t as a hardwired type int32_t.',
		               COMPILER_WARNING])
		fnd = False
	
	if (probe['builtin_sizes']['wint']['value'] != 4):
		print_warning(['The compiler provided macro __WINT_TYPE__ for defining',
		               'the compiler-native type wint_t is not compliant with',
		               'HelenOS. We are forced to define wint_t as a hardwired',
		               'type int32_t.',
		               COMPILER_WARNING])
		fnd = False
	
	if (not fnd):
		macros.append({'oldmacro': "int32_t", 'newmacro': "wint_t"})
	else:
		macros.append({'oldmacro': "__WINT_TYPE__", 'newmacro': "wint_t"})
	
	if (not 'wint' in probe['builtin_signs']):
		print_error(['Unable to determine whether wint_t is signed or unsigned.',
		             COMPILER_FAIL])
	
	if (probe['builtin_signs']['wint'] == 'unsigned'):
		macros.append({'oldmacro': "1", 'newmacro': 'WINT_IS_UNSIGNED'})
	if (probe['builtin_signs']['wint'] == 'signed'):
		macros.append({'oldmacro': "1", 'newmacro': 'WINT_IS_SIGNED'})
	
	return {'macros': macros, 'typedefs': typedefs}

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

def create_header(hdname, maps):
	"Create header output"
	
	outhd = open(hdname, 'w')
	
	outhd.write('/***************************************\n')
	outhd.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	outhd.write(' * Generated by: tools/autotool.py     *\n')
	outhd.write(' ***************************************/\n\n')
	
	outhd.write('#ifndef %s\n' % GUARD)
	outhd.write('#define %s\n\n' % GUARD)
	
	for macro in maps['macros']:
		outhd.write('#define %s  %s\n' % (macro['newmacro'], macro['oldmacro']))
	
	outhd.write('\n')
	
	for typedef in maps['typedefs']:
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
		check_app(["makedepend", "-f", "-"], "Makedepend utility", "usually part of imake or xutils")
		
		# Compiler
		common['CC_ARGS'] = []
		if (config['COMPILER'] == "gcc_cross"):
			target, cc_args, gnu_target, clang_target, helenos_target = get_target(config)
			
			if (target is None) or (gnu_target is None):
				print_error(["Unsupported compiler target for GNU GCC.",
				             "Please contact the developers of HelenOS."])
			
			path = "%s/%s/bin" % (cross_prefix, target)
			prefix = "%s-" % gnu_target
			
			check_gcc(path, prefix, common, PACKAGE_CROSS)
			check_binutils(path, prefix, common, PACKAGE_CROSS)
			
			check_common(common, "GCC")
			common['CC'] = common['GCC']
			common['CC_ARGS'].extend(cc_args)
		
		if (config['COMPILER'] == "gcc_helenos"):
			target, cc_args, gnu_target, clang_target, helenos_target = get_target(config)
			
			if (target is None) or (helenos_target is None):
				print_error(["Unsupported compiler target for GNU GCC.",
				             "Please contact the developers of HelenOS."])
			
			path = "%s/%s/bin" % (cross_helenos_prefix, target)
			prefix = "%s-" % helenos_target
			
			check_gcc(path, prefix, common, PACKAGE_CROSS)
			check_binutils(path, prefix, common, PACKAGE_CROSS)
			
			check_common(common, "GCC")
			common['CC'] = common['GCC']
			common['CC_ARGS'].extend(cc_args)
		
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
		
		if (config['COMPILER'] == "clang"):
			target, cc_args, gnu_target, clang_target, helenos_target = get_target(config)
			
			if (target is None) or (gnu_target is None) or (clang_target is None):
				print_error(["Unsupported compiler target for clang.",
				             "Please contact the developers of HelenOS."])
			
			path = "%s/%s/bin" % (cross_prefix, target)
			prefix = "%s-" % gnu_target
			
			check_app(["clang", "--version"], "clang compiler", "preferably version 1.0 or newer")
			check_gcc(path, prefix, common, PACKAGE_GCC)
			check_binutils(path, prefix, common, PACKAGE_BINUTILS)
			
			check_common(common, "GCC")
			common['CC'] = "clang"
			common['CC_ARGS'].extend(cc_args)
			common['CC_ARGS'].append("-target")
			common['CC_ARGS'].append(clang_target)
			common['CLANG_TARGET'] = clang_target
		
		check_python()
		
		# Platform-specific utilities
		if ((config['BARCH'] == "amd64") or (config['BARCH'] == "ia32") or (config['BARCH'] == "ppc32") or (config['BARCH'] == "sparc64")):
			common['GENISOIMAGE'] = check_app_alternatives(["mkisofs", "genisoimage"], ["--version"], "ISO 9660 creation utility", "usually part of genisoimage")
		
		probe = probe_compiler(common,
			[
				{'type': 'long long int', 'tag': 'LLONG', 'strc': '"ll"', 'conc': '"LL"'},
				{'type': 'long int', 'tag': 'LONG', 'strc': '"l"', 'conc': '"L"'},
				{'type': 'int', 'tag': 'INT', 'strc': '""', 'conc': '""'},
				{'type': 'short int', 'tag': 'SHORT', 'strc': '"h"', 'conc': '"@"'},
				{'type': 'char', 'tag': 'CHAR', 'strc': '"hh"', 'conc': '"@@"'}
			],
			[
				{'type': 'long double', 'tag': 'LONG_DOUBLE'},
				{'type': 'double', 'tag': 'DOUBLE'},
				{'type': 'float', 'tag': 'FLOAT'}
			]
		)
		
		maps = detect_sizes(probe, [1, 2, 4, 8], ['CHAR', 'SHORT', 'INT', 'LONG', 'LLONG'], ['LONG_DOUBLE', 'DOUBLE', 'FLOAT'])
		
	finally:
		sandbox_leave(owd)
	
	common['AUTOGEN'] = "%s/autogen.py" % os.path.dirname(os.path.abspath(sys.argv[0]))

	create_makefile(MAKEFILE, common)
	create_header(HEADER, maps)
	
	return 0

if __name__ == '__main__':
	sys.exit(main())
