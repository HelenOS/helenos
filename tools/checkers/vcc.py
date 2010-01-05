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
Wrapper for Vcc checker
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
	print prname + " <ROOT>"

def cygpath(upath):
	"Convert Unix (Cygwin) path to Windows path"
	
	return subprocess.Popen(['cygpath', '--windows', '--absolute', upath], stdout = subprocess.PIPE).communicate()[0].strip()

def preprocess(srcfname, tmpfname, base, options):
	"Preprocess source using GCC preprocessor"
	
	args = ['gcc', '-E']
	args.extend(options.split())
	args.extend(['-o', tmpfname, srcfname])
	
	cwd = os.getcwd()
	os.chdir(base)
	retval = subprocess.Popen(args).wait()
	os.chdir(cwd)
	
	if (retval != 0):
		return False
	
	return True

def vcc(root, job):
	"Run Vcc on a jobfile"
	
	# Parse jobfile
	
	inname = os.path.join(root, job)
	
	if (not os.path.isfile(inname)):
		print "Unable to open %s" % inname
		print "Did you run \"make precheck\" on the source tree?"
		return False
	
	inf = file(inname, "r")
	records = inf.read().splitlines()
	inf.close()
	
	for record in records:
		arg = jobfile.parse_arg(record)
		if (not arg):
			return False
		
		if (len(arg) < 6):
			print "Not enought jobfile record arguments"
			return False
		
		srcfname = arg[0]
		tgtfname = arg[1]
		base = arg[4]
		options = arg[5]
		
		srcfqname = os.path.join(base, srcfname)
		if (not os.path.isfile(srcfqname)):
			print "Source %s not found" % srcfqname
			return False
		
		tmpfname = "%s.preproc" % srcfname
		tmpfqname = os.path.join(base, tmpfname)
		
		# Only C files are interesting for us
		if (arg[2] != "cc"):
			continue
		
		# Preprocess sources
		
		if (not preprocess(srcfname, tmpfname, base, options)):
			return False
		
		# Run Vcc
		
		retval = subprocess.Popen(['vcc', cygpath(tmpfqname)]).wait()
		
		# Cleanup
		
		if (os.path.isfile(tmpfqname)):
			os.remove(tmpfqname)
		
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
		print "%s not found." % config
		print "Please specify the path to HelenOS build tree root as the first argument."
		return
	
	for job in jobs:
		if (not vcc(rootdir, job)):
			print
			print "Failed job: %s" % job
			return
	
	print
	print "All jobs passed"

if __name__ == '__main__':
	main()
