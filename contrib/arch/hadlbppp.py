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
	
	return tokens

def split_tokens(string, delimiters, trim = False, separate = False):
	"Split string to tokens by delimiters, keep the delimiters"
	
	tokens = []
	last = 0
	i = 0
	
	while (i < len(string)):
		for delim in delimiters:
			if (len(delim) > 0):
				
				if (string[i:(i + len(delim))] == delim):
					if (separate):
						tokens = cond_append(tokens, string[last:i], trim)
						tokens = cond_append(tokens, delim, trim)
						last = i + len(delim)
					elif (i > 0):
						tokens = cond_append(tokens, string[last:i], trim)
						last = i
					
					i += len(delim) - 1
					break
		
		i += 1
	
	tokens = cond_append(tokens, string[last:len(string)], trim)
	
	return tokens

def preproc_bp(outname, tokens):
	"Preprocess tentative statements in Behavior Protocol"
	
	result = []
	i = 0
	
	while (i < len(tokens)):
		if (tokens[i] == "tentative"):
			if ((i + 1 < len(tokens)) and (tokens[i + 1] == "{")):
				i += 2
				start = i
				level = 1
				
				while ((i < len(tokens)) and (level > 0)):
					if (tokens[i] == "{"):
						level += 1
					elif (tokens[i] == "}"):
						level -= 1
					
					i += 1
				
				if (level == 0):
					result.append("(")
					result.extend(preproc_bp(outname, tokens[start:(i - 1)]))
					result.append(")")
					result.append("+")
					result.append("NULL")
				else:
					print "%s: Syntax error in tentative statement" % outname
			else:
				print "%s: Unexpected tentative statement" % outname
		else:
			result.append(tokens[i])
		
		i += 1
	
	return result

def parse_bp(base, root, inname, nested, outname, outf, indent):
	"Parse Behavior Protocol"
	
	if (nested):
		if (inname[0:1] == "/"):
			path = os.path.join(base, ".%s" % inname)
			nested_root = os.path.dirname(path)
		else:
			path = os.path.join(root, inname)
			nested_root = root
		
		if (not os.path.isfile(path)):
			print "%s: Unable to include file %s" % (outname, path)
			return True
		
		inf = file(path, "r")
	else:
		inf = file(inname, "r")
		nested_root = root
	
	tokens = preproc_bp(outname, split_tokens(inf.read(), ["\n", " ", "\t", "(", ")", "{", "}", "[", "]", "/*", "*/", "#", "*", ";", "+", "||", "|", "!", "?"], True, True))
	
	inc = False
	empty = True
	comment = False
	lcomment = False
	
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
		
		if (inc):
			outf.write("\n%s(" % tabs(indent))
			
			inc_empty = parse_bp(base, nested_root, token, True, outname, outf, indent + 1)
			if (inc_empty):
				outf.write("\n%sNULL" % tabs(indent + 1))
			
			outf.write("\n%s)" % tabs(indent))
			inc = False
			continue
		
		if ((token == ";") or (token == "+") or (token == "||") or (token == "|")):
			outf.write(" %s" % token)
		elif (token == "["):
			inc = True
		elif (token == "]"):
			inc = False
		elif (token == "("):
			outf.write("\n%s%s" % (tabs(indent), token))
			indent += 1
		elif (token == ")"):
			if (indent == 0):
				print "%s: Too many closing parentheses" % outname
			
			indent -= 1
			outf.write("\n%s%s" % (tabs(indent), token))
		elif (token == "{"):
			outf.write(" %s" % token)
			indent += 1
		elif (token == "}"):
			if (indent == 0):
				print "%s: Too many closing parentheses" % outname
			
			indent -= 1
			outf.write("\n%s%s" % (tabs(indent), token))
		elif (token == "*"):
			outf.write("%s" % token)
		elif ((token == "!") or (token == "?") or (token == "NULL")):
			outf.write("\n%s%s" % (tabs(indent), token))
		else:
			outf.write("%s" % token)
	
	inf.close()
	
	return empty

def parse_adl(base, root, inname, nested, outname, outf, indent):
	"Parse Architecture Description Language"
	
	if (nested):
		(infname, inarg) = inname.split("%")
		
		if (infname[0:1] == "/"):
			path = os.path.join(base, ".%s" % infname)
			nested_root = os.path.dirname(path)
		else:
			path = os.path.join(root, infname)
			nested_root = root
		
		if (not os.path.isfile(path)):
			print "%s: Unable to include file %s" % (outname, path)
			return True
		
		inf = file(path, "r")
	else:
		inarg = "%%"
		inf = file(inname, "r")
		nested_root = root
	
	tokens = split_tokens(inf.read(), ["\n", " ", "\t", "%%", "[", "]"], False, True)
	
	inc = False
	empty = True
	newline = True
	locindent = 0
	
	for token in tokens:
		if (empty):
			empty = False
		
		if (inc):
			if (token.find("%") != -1):
				parse_adl(base, nested_root, token, True, outname, outf, locindent)
			else:
				parse_bp(base, nested_root, token, True, outname, outf, locindent)
			
			inc = False
			continue
		
		if (token == "\n"):
			newline = True
			locindent = 0
			outf.write("\n%s" % tabs(indent))
		elif (token == "\t"):
			if (newline):
				locindent += 1
			outf.write("%s" % token)
		elif (token == "%%"):
			newline = False
			outf.write("%s" % inarg)
		elif (token == "["):
			newline = False
			inc = True
		elif (token == "]"):
			newline = False
			inc = False
		else:
			newline = False;
			outf.write("%s" % token)
	
	inf.close()
	
	return empty

def open_bp(base, root, inname, outname):
	"Open Behavior Protocol file"
	
	outf = file(outname, "w")
	
	outf.write("### %s\n" % inname)
	
	empty = parse_bp(base, root, inname, False, outname, outf, 0)
	if (empty):
		outf.write("NULL")
	
	outf.close()

def open_adl(base, root, inname, outname):
	"Open Architecture Description file"
	
	outf = file(outname, "w")
	
	empty = parse_adl(base, root, inname, False, outname, outf, 0)
	if (empty):
		outf.write("/* Empty */")
	
	outf.close()

def recursion(base, root, output, level):
	"Recursive directory walk"
	
	for name in os.listdir(root):
		canon = os.path.join(root, name)
		
		if (os.path.isfile(canon)):
			fcomp = split_tokens(canon, ["."])
			cname = canon.split("/")
			
			filtered = False
			while (not filtered):
				try:
					cname.remove(".")
				except (ValueError):
					filtered = True
			
			output_path = os.path.join(output, ".".join(cname))
			
			if (fcomp[-1] == ".bp"):
				open_bp(base, root, canon, output_path)
			elif (fcomp[-1] == ".adl"):
				open_adl(base, root, canon, output_path)
		
		if (os.path.isdir(canon)):
			recursion(base, canon, output, level + 1)

def main():
	if (len(sys.argv) < 2):
		usage(sys.argv[0])
		return
	
	path = os.path.abspath(sys.argv[1])
	if (not os.path.isdir(path)):
		print "Error: <OUTPUT> is not a directory"
		return
	
	recursion(".", ".", path, 0)

if __name__ == '__main__':
	main()
