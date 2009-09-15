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

INC, POST_INC, BLOCK_COMMENT, LINE_COMMENT, SYSTEM, ARCH, HEAD, BODY, NULL, \
	INST, VAR, FIN, BIND, TO, SEEN_NL, IFACE, PROTOTYPE, PAR_LEFT, PAR_RIGHT, SIGNATURE, PROTOCOL = range(21)

context = set()
interface = None
architecture = None
protocol = None

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

def preproc_adl(raw, inarg):
	"Preprocess %% statements in ADL"
	
	return raw.replace("%%", inarg)

def identifier(token):
	"Check whether the token is an identifier"
	
	if (len(token) == 0):
		return False
	
	for i, char in enumerate(token):
		if (i == 0):
			if ((not char.isalpha()) and (char != "_")):
				return False
		else:
			if ((not char.isalnum()) and (char != "_")):
				return False
	
	return True

def descriptor(token):
	"Check whether the token is an interface descriptor"
	
	parts = token.split(":")
	if (len(parts) != 2):
		return False
	
	return (identifier(parts[0]) and identifier(parts[1]))

def word(token):
	"Check whether the token is a word"
	
	if (len(token) == 0):
		return False
	
	for i, char in enumerate(token):
		if ((not char.isalnum()) and (char != "_") and (char != ".")):
			return False
	
	return True

def parse_bp(base, root, inname, nested, outname, indent):
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
			return ""
	else:
		path = inname
		nested_root = root
	
	inf = file(path, "r")
	tokens = preproc_bp(outname, split_tokens(inf.read(), ["\n", " ", "\t", "(", ")", "{", "}", "[", "]", "/*", "*/", "#", "*", ";", "+", "||", "|", "!", "?"], True, True))
	
	output = ""
	inc = False
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
		
		if (inc):
			output += "\n%s(" % tabs(indent)
			
			bp = parse_bp(base, nested_root, token, True, outname, indent + 1)
			if (bp.strip() == ""):
				output += "\n%sNULL" % tabs(indent + 1)
			else:
				output += bp
			
			output += "\n%s)" % tabs(indent)
			inc = False
			continue
		
		if ((token == ";") or (token == "+") or (token == "||") or (token == "|")):
			output += " %s" % token
		elif (token == "["):
			inc = True
		elif (token == "]"):
			inc = False
		elif (token == "("):
			output += "\n%s%s" % (tabs(indent), token)
			indent += 1
		elif (token == ")"):
			if (indent <= 0):
				print "%s: Wrong number of parentheses" % outname
			
			indent -= 1
			output += "\n%s%s" % (tabs(indent), token)
		elif (token == "{"):
			output += " %s" % token
			indent += 1
		elif (token == "}"):
			if (indent <= 0):
				print "%s: Wrong number of parentheses" % outname
			
			indent -= 1
			output += "\n%s%s" % (tabs(indent), token)
		elif (token == "*"):
			output += "%s" % token
		elif ((token == "!") or (token == "?") or (token == "NULL")):
			output += "\n%s%s" % (tabs(indent), token)
		else:
			output += "%s" % token
	
	inf.close()
	
	return output

def parse_adl(base, root, inname, nested, indent):
	"Parse Architecture Description Language"
	
	if (nested):
		parts = inname.split("%")
		
		if (len(parts) > 1):
			inarg = parts[1]
		else:
			inarg = "%%"
		
		if (parts[0][0:1] == "/"):
			path = os.path.join(base, ".%s" % parts[0])
			nested_root = os.path.dirname(path)
		else:
			path = os.path.join(root, parts[0])
			nested_root = root
		
		if (not os.path.isfile(path)):
			print "%s: Unable to include file %s" % (inname, path)
			return ""
	else:
		inarg = "%%"
		path = inname
		nested_root = root
	
	inf = file(path, "r")
	
	raw = preproc_adl(inf.read(), inarg)
	tokens = split_tokens(raw, ["\n", " ", "\t", "(", ")", "{", "}", "[", "]", "/*", "*/", "#", ";"], True, True)
	output = ""
	
	for token in tokens:
		
		# Includes
		
		if (INC in context):
			context.remove(INC)
			
			if (PROTOCOL in context):
				protocol += parse_bp(base, nested_root, token, True, "xxx", indent).strip()
			else:
				output += "\n%s" % tabs(indent)
				output += parse_adl(base, nested_root, token, True, indent).strip()
			
			context.add(POST_INC)
			continue
		
		if (POST_INC in context):
			if (token != "]"):
				print "%s: Expected ]" % inname
			
			context.add(SEEN_NL)
			context.remove(POST_INC)
			continue
		
		# Comments and newlines
		
		if (BLOCK_COMMENT in context):
			if (token == "*/"):
				context.remove(BLOCK_COMMENT)
			
			continue
		
		if (LINE_COMMENT in context):
			if (token == "\n"):
				context.remove(LINE_COMMENT)
			
			continue
		
		# Any context
		
		if (token == "/*"):
			context.add(BLOCK_COMMENT)
			continue
		
		if (token == "#"):
			context.add(LINE_COMMENT)
			continue
		
		if (token == "["):
			context.add(INC)
			continue
		
		# Seen newline
		
		if (SEEN_NL in context):
			context.remove(SEEN_NL)
			if (token == "\n"):
				output += "\n%s" % tabs(indent)
				continue
		else:
			if (token == "\n"):
				continue
		
		# "interface"
		
		if (IFACE in context):
			if (NULL in context):
				if (token != ";"):
					print "%s: Expected ;" % inname
				else:
					output += "%s\n" % token
				
				context.remove(NULL)
				context.remove(IFACE)
				interface = None
				continue
			
			if (BODY in context):
				if (PROTOCOL in context):
					if (token == "{"):
						indent += 1
					elif (token == "}"):
						indent -= 1
					
					if (indent == 1):
						output += protocol.strip()
						protocol = None
						
						output += "\n%s" % token
						
						context.remove(PROTOCOL)
						context.remove(BODY)
						context.add(NULL)
					else:
						protocol += token
					
					continue
				
				if (PROTOTYPE in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ;" % inname
						else:
							output += "%s" % token
						
						context.remove(FIN)
						context.remove(PROTOTYPE)
						continue
					
					if (PAR_RIGHT in context):
						if (token == ")"):
							output += "%s" % token
							context.remove(PAR_RIGHT)
							context.add(FIN)
						else:
							output += " %s" % token
						
						continue
					
					if (SIGNATURE in context):
						output += "%s" % token
						if (token == ")"):
							context.remove(SIGNATURE)
							context.add(FIN)
						
						context.remove(SIGNATURE)
						context.add(PAR_RIGHT)
						continue
					
					if (PAR_LEFT in context):
						if (token != "("):
							print "%s: Expected (" % inname
						else:
							output += "%s" % token
						
						context.remove(PAR_LEFT)
						context.add(SIGNATURE)
						continue
					
					if (not identifier(token)):
						print "%s: Method identifier expected" % inname
					else:
						output += "%s" % token
					
					context.add(PAR_LEFT)
					continue
				
				if (token == "}"):
					if (indent != 1):
						print "%s: Wrong number of parentheses" % inname
					else:
						indent = 0
						output += "\n%s" % token
					
					context.remove(BODY)
					context.add(NULL)
					continue
				
				if (token == "ipcarg_t"):
					output += "\n%s%s " % (tabs(indent), token)
					context.add(PROTOTYPE)
					continue
				
				if (token == "protocol:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					context.add(PROTOCOL)
					protocol = ""
					continue
				
				print "%s: Unknown token %s in interface" % (inname, token)
				continue
			
			if (HEAD in context):
				if (token == "{"):
					output += "%s" % token
					indent += 2
					context.remove(HEAD)
					context.add(BODY)
					continue
				
				if (token == ";"):
					output += "%s\n" % token
					context.remove(HEAD)
					context.remove(ARCH)
					context.discard(SYSTEM)
					continue
				
				if (not word(token)):
					print "%s: Expected word" % inname
				else:
					output += "%s " % token
				
				continue
			
			if (not identifier(token)):
				print "%s: Expected interface name" % inname
			else:
				interface = token
				output += "%s " % token
			
			context.add(HEAD)
			continue
		
		# "architecture"
		
		if (ARCH in context):
			if (NULL in context):
				if (token != ";"):
					print "%s: Expected ;" % inname
				else:
					output += "%s\n" % token
				
				context.remove(NULL)
				context.remove(ARCH)
				context.discard(SYSTEM)
				architecture = None
				continue
			
			if (BODY in context):
				if (BIND in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ;" % inname
						else:
							output += "%s" % token
						
						context.remove(FIN)
						context.remove(BIND)
						continue
					
					if (VAR in context):
						if (not descriptor(token)):
							print "%s: Expected second interface descriptor" % inname
						else:
							output += "%s" % token
						
						context.add(FIN)
						context.remove(VAR)
						continue
					
					if (TO in context):
						if (token != "to"):
							print "%s: Expected to" % inname
						else:
							output += "%s " % token
						
						context.add(VAR)
						context.remove(TO)
						continue
					
					if (not descriptor(token)):
						print "%s: Expected interface descriptor" % inname
					else:
						output += "%s " % token
					
					context.add(TO)
					continue
				
				if (INST in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ;" % inname
						else:
							output += "%s" % token
						
						context.remove(FIN)
						context.remove(INST)
						continue
					
					if (VAR in context):
						if (not identifier(token)):
							print "%s: Expected instance name" % inname
						else:
							output += "%s" % token
						
						context.add(FIN)
						context.remove(VAR)
						continue
					
					if (not identifier(token)):
						print "%s: Expected frame/architecture type" % inname
					else:
						output += "%s " % token
					
					context.add(VAR)
					continue
				
				if (token == "}"):
					if (indent != 1):
						print "%s: Wrong number of parentheses" % inname
					else:
						indent -= 1
						output += "\n%s" % token
					
					context.remove(BODY)
					context.add(NULL)
					continue
				
				if (token == "inst"):
					output += "\n%s%s " % (tabs(indent), token)
					context.add(INST)
					continue
				
				if (token == "bind"):
					output += "\n%s%s " % (tabs(indent), token)
					context.add(BIND)
					continue
				
				print "%s: Unknown token %s in architecture" % (inname, token)
				continue
			
			if (HEAD in context):
				if (token == "{"):
					output += "%s" % token
					indent += 1
					context.remove(HEAD)
					context.add(BODY)
					continue
				
				if (token == ";"):
					output += "%s\n" % token
					context.remove(HEAD)
					context.remove(ARCH)
					context.discard(SYSTEM)
					continue
				
				if (not word(token)):
					print "%s: Expected word" % inname
				else:
					output += "%s " % token
				
				continue
			
			if (not identifier(token)):
				print "%s: Expected architecture name" % inname
			else:
				architecture = token
				output += "%s " % token
			
			context.add(HEAD)
			continue
		
		# "system architecture"
		
		if (SYSTEM in context):
			if (token != "architecture"):
				print "%s: Expected architecture" % inname
			else:
				output += "%s " % token
			
			context.add(ARCH)
			continue
		
		if (token == "interface"):
			output += "\n%s " % token
			context.add(IFACE)
			continue
		
		if (token == "system"):
			output += "\n%s " % token
			context.add(SYSTEM)
			continue
		
		if (token == "architecture"):
			output += "\n%s " % token
			context.add(ARCH)
			continue
		
		print "%s: Unknown token %s" % (inname, token)
	
	inf.close()
	
	return output

def open_adl(base, root, inname, outname):
	"Open Architecture Description file"
	
	context.clear()
	interface = None
	architecture = None
	protocol = None
	
	adl = parse_adl(base, root, inname, False, 0)
	if (adl.strip() == ""):
		adl = "/* Empty */\n"
	
	if (os.path.isfile(outname)):
		print "%s: File already exists, overwriting" % outname
	
	outf = file(outname, "w")
	outf.write(adl.strip())
	outf.close()

def recursion(base, root, output, level):
	"Recursive directory walk"
	
	for name in os.listdir(root):
		canon = os.path.join(root, name)
		
		if (os.path.isfile(canon)):
			fcomp = split_tokens(canon, ["."])
			cname = canon.split("/")
			
			if (fcomp[-1] == ".adl"):
				output_path = os.path.join(output, cname[-1])
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
