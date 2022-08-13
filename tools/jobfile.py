#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2009 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""
Add a source/object file pair to a checker jobfile
"""

import sys
import os
import fcntl

def usage(prname):
	"Print usage syntax"
	print(prname + " <JOBFILE> <CC> <INPUT> -o <OUTPUT> [CC_ARGUMENTS ...]")

def main():
	if (len(sys.argv) < 6):
		usage(sys.argv[0])
		return

	jobfname = sys.argv[1]
	ccname = sys.argv[2]
	srcfname = sys.argv[3]
	assert(not srcfname.startswith("-"))
	assert(sys.argv[4] == "-o")
	tgtfname = sys.argv[5]
	options = " ".join(sys.argv[6:])
	cwd = os.getcwd()

	if srcfname.endswith(".c"):
		toolname = "cc"
		category = "core"

	if srcfname.endswith(".s"):
		toolname = "as"
		category = "asm"

	if srcfname.endswith(".S"):
		toolname = "as"
		category = "asm/preproc"

	jobfile = open(jobfname, "a")
	fcntl.lockf(jobfile, fcntl.LOCK_EX)
	jobfile.write("{%s},{%s},{%s},{%s},{%s},{%s}\n" % (srcfname, tgtfname, toolname, category, cwd, options))
	fcntl.lockf(jobfile, fcntl.LOCK_UN)
	jobfile.close()

	# Run the compiler proper.
	os.execvp(ccname, sys.argv[2:])

if __name__ == '__main__':
	main()
