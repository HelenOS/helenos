#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2010 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#
"""
Wrapper for Stanse static checker
"""

import sys
import os
import subprocess
import jobfile

jobs = [
	"kernel/kernel.job",
	"uspace/srv/clip/clip.job"
]

def usage(prname):
	"Print usage syntax"
	print(prname + " <ROOT>")

def stanse(root, job):
	"Run Stanse on a jobfile"

	# Convert generic jobfile to Stanse-specific jobfile format

	inname = os.path.join(root, job)
	outname = os.path.join(root, "_%s" % os.path.basename(job))

	if (not os.path.isfile(inname)):
		print("Unable to open %s" % inname)
		print("Did you run \"make precheck\" on the source tree?")
		return False

	inf = open(inname, "r")
	records = inf.read().splitlines()
	inf.close()

	output = []
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

		# Only C files are interesting for us
		if (tool != "cc"):
			continue

		output.append([srcfname, tgtfname, base, options])

	outf = open(outname, "w")
	for record in output:
		outf.write("{%s},{%s},{%s},{%s}\n" % (record[0], record[1], record[2], record[3]))
	outf.close()

	# Run Stanse

	retval = subprocess.Popen(['stanse', '--checker', 'ReachabilityChecker', '--jobfile', outname]).wait()

	# Cleanup

	os.remove(outname)
	for record in output:
		tmpfile = os.path.join(record[2], "%s.preproc" % record[1])
		if (os.path.isfile(tmpfile)):
			os.remove(tmpfile)

	if (retval == 0):
		return True

	return False

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
		if (not stanse(rootdir, job)):
			print()
			print("Failed job: %s" % job)
			return

	print
	print("All jobs passed")

if __name__ == '__main__':
	main()
