#!/usr/bin/env python
#
# Copyright (c) 2010 Martin Decky
# Copyright (c) 2010 Ondrej Sery
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
Wrapper for Vcc checker
"""

import sys
import os
import subprocess
import jobfile
import re

jobs = [
	"kernel/kernel.job"
]

re_attribute = re.compile("__attribute__\s*\(\(.*\)\)")
re_va_list = re.compile("__builtin_va_list")

specification = ""

def usage(prname):
	"Print usage syntax"
	print(prname + " <ROOT> [VCC_PATH]")

def cygpath(upath):
	"Convert Unix (Cygwin) path to Windows path"

	return subprocess.Popen(['cygpath', '--windows', '--absolute', upath], stdout = subprocess.PIPE).communicate()[0].strip()

def preprocess(srcfname, tmpfname, base, options):
	"Preprocess source using GCC preprocessor and compatibility tweaks"

	global specification

	args = ['gcc', '-E']
	args.extend(options.split())
	args.extend(['-DCONFIG_VERIFY_VCC=1', srcfname])

	# Change working directory

	cwd = os.getcwd()
	os.chdir(base)

	preproc = subprocess.Popen(args, stdout = subprocess.PIPE).communicate()[0]

	tmpf = open(tmpfname, "w")
	tmpf.write(specification)

	for line in preproc.splitlines():

		# Ignore preprocessor directives

		if (line.startswith('#')):
			continue

		# Remove __attribute__((.*)) GCC extension

		line = re.sub(re_attribute, "", line)

		# Ignore unsupported __builtin_va_list type
		# (a better solution replacing __builrin_va_list with
		# an emulated implementation is needed)

		line = re.sub(re_va_list, "void *", line)

		tmpf.write("%s\n" % line)

	tmpf.close()

	os.chdir(cwd)

	return True

def vcc(vcc_path, root, job):
	"Run Vcc on a jobfile"

	# Parse jobfile

	inname = os.path.join(root, job)

	if (not os.path.isfile(inname)):
		print("Unable to open %s" % inname)
		print("Did you run \"make precheck\" on the source tree?")
		return False

	inf = open(inname, "r")
	records = inf.read().splitlines()
	inf.close()

	for record in records:
		arg = jobfile.parse_arg(record)
		if (not arg):
			return False

		if (len(arg) < 6):
			print("Not enough jobfile record arguments")
			return False

		srcfname = arg[0]
		tgtfname = arg[1]
		tool = arg[2]
		category = arg[3]
		base = arg[4]
		options = arg[5]

		srcfqname = os.path.join(base, srcfname)
		if (not os.path.isfile(srcfqname)):
			print("Source %s not found" % srcfqname)
			return False

		tmpfname = "%s.preproc" % srcfname
		tmpfqname = os.path.join(base, tmpfname)

		vccfname = "%s.i" % srcfname
		vccfqname = os.path.join(base, vccfname);

		# Only C files are interesting for us
		if (tool != "cc"):
			continue

		# Preprocess sources

		if (not preprocess(srcfname, tmpfname, base, options)):
			return False

		# Run Vcc
		print(" -- %s --" % srcfname)
		retval = subprocess.Popen([vcc_path, '/pointersize:32', '/newsyntax', cygpath(tmpfqname)]).wait()

		if (retval != 0):
			return False

		# Cleanup, but only if verification was successful
		# (to be able to examine the preprocessed file)

		if (os.path.isfile(tmpfqname)):
			os.remove(tmpfqname)
			os.remove(vccfqname)

	return True

def main():
	global specification

	if (len(sys.argv) < 2):
		usage(sys.argv[0])
		return

	rootdir = os.path.abspath(sys.argv[1])
	if (len(sys.argv) > 2):
		vcc_path = sys.argv[2]
	else:
		vcc_path = "/cygdrive/c/Program Files (x86)/Microsoft Research/Vcc/Binaries/vcc"

	if (not os.path.isfile(vcc_path)):
		print("%s is not a binary." % vcc_path)
		print("Please supply the full Cygwin path to Vcc as the second argument.")
		return

	config = os.path.join(rootdir, "HelenOS.config")

	if (not os.path.isfile(config)):
		print("%s not found." % config)
		print("Please specify the path to HelenOS build tree root as the first argument.")
		return

	specpath = os.path.join(rootdir, "tools/checkers/vcc.h")
	if (not os.path.isfile(specpath)):
		print("%s not found." % config)
		return

	specfile = file(specpath, "r")
	specification = specfile.read()
	specfile.close()

	for job in jobs:
		if (not vcc(vcc_path, rootdir, job)):
			print()
			print("Failed job: %s" % job)
			return

	print()
	print("All jobs passed")

if __name__ == '__main__':
	main()
