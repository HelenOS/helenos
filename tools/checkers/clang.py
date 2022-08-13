#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2010 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#
"""
Wrapper for Clang static analyzer
"""

import sys
import os
import subprocess
import jobfile

jobs = [
	"kernel/kernel.job"
]

def usage(prname):
	"Print usage syntax"
	print(prname + " <ROOT>")

def clang(root, job):
	"Run Clang on a jobfile"

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
		options = arg[5].split()

		srcfqname = os.path.join(base, srcfname)
		if (not os.path.isfile(srcfqname)):
			print("Source %s not found" % srcfqname)
			return False

		# Only C files are interesting for us
		if (tool != "cc"):
			continue

		args = ['clang', '-Qunused-arguments', '--analyze',
			'-Xanalyzer', '-analyzer-opt-analyze-headers',
			'-Xanalyzer', '-checker-cfref']
		args.extend(options)
		args.extend(['-o', tgtfname, srcfname])

		cwd = os.getcwd()
		os.chdir(base)
		retval = subprocess.Popen(args).wait()
		os.chdir(cwd)

		if (retval != 0):
			return False

	return True

def main():
	if (len(sys.argv) < 2):
		usage(sys.argv[0])
		return

	rootdir = os.path.abspath(sys.argv[1])
	config = os.path.join(rootdir, "HelenOS.config")

	if (not os.path.isfile(config)):
		print("%s not found." % config)
		print("Please specify the path to HelenOS build tree root as the first argument.")
		return

	for job in jobs:
		if (not clang(rootdir, job)):
			print()
			print("Failed job: %s" % job)
			return

	print
	print("All jobs passed")

if __name__ == '__main__':
	main()
