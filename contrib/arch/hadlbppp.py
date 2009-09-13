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
HelenOS Architecture Description Language and Behavior Protocols preprocessor
"""

import sys
import os

def usage(prname):
	"Print usage syntax"
	print prname + " <OUTPUT>"

def tabs(cnt):
	"Return given number of tabs"
	
	return ("\t" * cnt)

def cond_append(tokens, token, trim):
	"Conditionally append token to tokens with trim"
	
	if (trim):
		token = token.strip(" \t")
		if (token != ""):
			tokens.append(token)
	else:
		tokens.append(token)
	
	return tokens

def split_tokens(string, delimiters, trim = False):
	"Split string to tokens by delimiters, keep the delimiters"
	
	tokens = []
	last = 0
	i = 0
	
	while (i < len(string)):
		for delim in delimiters:
			if (len(delim) > 0):
				
				if ((string[i:(i + len(delim))] == delim) and (i > 0)):
					tokens = cond_append(tokens, string[last:i], trim)
					last = i
					i += len(delim) - 1
					break
		
		i += 1
	
	tokens = cond_append(tokens, string[last:len(string)], trim)
	
	return tokens

def parse(fname, outf):
	"Parse particular protocol"
	
	inf = file(fname, "r")
	outf.write("### %s\n\n" % fname)
	
	tokens = split_tokens(inf.read(), ["\n", " ", "\t", "(", ")", "{", "}", "[", "/*", "*/", "#", "*", ";", "+", "||", "|", "!", "?"], True)
	
	empty = True
	comment = False
	lcomment = False
	indent = 0
	
	for token in tokens:
		if (comment):
			if (token == "*/"):
				comment = False
			continue
		
		if ((not comment) and (token == "/*")):
			comment = True
			continue
		
		if (lcomment):
			if (token == "\n"):
				lcomment = False
			continue
		
		if ((not lcomment) and (token == "#")):
			lcomment = True
			continue
		
		if (token == "\n"):
			continue
		
		if (empty):
			empty = False
		
		if ((token == ";") or (token == "+") or (token == "||") or (token == "|")):
			outf.write(" %s\n" % token)
		elif (token == "("):
			outf.write("%s%s\n" % (tabs(indent), token))
			indent += 1
		elif (token == ")"):
			indent -= 1
			outf.write("\n%s%s" % (tabs(indent), token))
		elif (token == "{"):
			outf.write(" %s\n" % token)
			indent += 1
		elif (token == "}"):
			indent -= 1
			outf.write("\n%s%s" % (tabs(indent), token))
		elif (token == "*"):
			outf.write("%s" % token)
		else:
			outf.write("%s%s" % (tabs(indent), token))
	
	if (empty):
		outf.write("NULL")
	
	outf.write("\n\n\n")
	inf.close()

def recursion(root, output, level):
	"Recursive directory walk"
	
	for name in os.listdir(root):
		canon = os.path.join(root, name)
		
		if ((os.path.isfile(canon)) and (level > 0)):
			fcomp = split_tokens(canon, ["."])
			if (fcomp[-1] == ".bp"):
				parse(canon, outf)
		
		if (os.path.isdir(canon)):
			recursion(canon, outf, level + 1)

def main():
	if (len(sys.argv) < 2):
		usage(sys.argv[0])
		return
	
	path = os.path.abspath(sys.argv[1])
	if (not os.path.isdir(path)):
		print "<OUTPUT> is not a directory"
		return
	
	recursion(".", path, 0)
	
if __name__ == '__main__':
	main()
