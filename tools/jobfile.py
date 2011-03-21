#!/usr/bin/env python
#
# Copyright (c) 2009 Martin Decky
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
Add a source/object file pair to a checker jobfile
"""

import sys
import os
import fcntl

def usage(prname):
	"Print usage syntax"
	print(prname + " <JOBFILE> <SOURCE> <TARGET> <TOOL> <CATEGORY> [OPTIONS ...]")

def main():
	if (len(sys.argv) < 6):
		usage(sys.argv[0])
		return
	
	jobfname = sys.argv[1]
	srcfname = sys.argv[2]
	tgtfname = sys.argv[3]
	toolname = sys.argv[4]
	category = sys.argv[5]
	cwd = os.getcwd()
	options = " ".join(sys.argv[6:])
	
	jobfile = open(jobfname, "a")
	fcntl.lockf(jobfile, fcntl.LOCK_EX)
	jobfile.write("{%s},{%s},{%s},{%s},{%s},{%s}\n" % (srcfname, tgtfname, toolname, category, cwd, options))
	fcntl.lockf(jobfile, fcntl.LOCK_UN)
	jobfile.close()

if __name__ == '__main__':
	main()
