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
	INST, VAR, FIN, BIND, TO, SUBSUME, DELEGATE, IFACE, PROTOTYPE, PAR_LEFT, \
	PAR_RIGHT, SIGNATURE, PROTOCOL, FRAME, PROVIDES, REQUIRES = range(25)

def usage(prname):
	"Print usage syntax"
	
	print "%s <OUTPUT>" % prname

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

def preproc_bp(name, tokens):
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
					result.extend(preproc_bp(name, tokens[start:(i - 1)]))
					result.append(")")
					result.append("+")
					result.append("NULL")
					if (i < len(tokens)):
						result.append(tokens[i])
				else:
					print "%s: Syntax error in tentative statement" % name
			else:
				print "%s: Unexpected token in tentative statement" % name
		else:
			result.append(tokens[i])
		
		i += 1
	
	return result

def split_bp(protocol):
	"Convert Behavior Protocol to tokens"
	
	return split_tokens(protocol, ["\n", " ", "\t", "(", ")", "{", "}", "*", ";", "+", "||", "|", "!", "?"], True, True)

def extend_bp(name, tokens, iface):
	"Add interface to incoming messages"
	
	result = []
	i = 0
	
	while (i < len(tokens)):
		result.append(tokens[i])
		
		if (tokens[i] == "?"):
			if (i + 1 < len(tokens)):
				i += 1
				parts = tokens[i].split(".")
				
				if (len(parts) == 1):
					result.append("%s.%s" % (iface, tokens[i]))
				else:
					result.append(tokens[i])
			else:
				print "%s: Unexpected end of protocol" % name
		
		i += 1
	
	return result

def merge_bp(protocols):
	"Merge several Behavior Protocols"
	
	if (len(protocols) > 1):
		result = []
		first = True
		
		for protocol in protocols:
			if (first):
				first = False
			else:
				result.append("|")
			
			result.append("(")
			result.extend(protocol)
			result.append(")")
		
		return result
	
	if (len(protocols) == 1):
		return protocols[0]
	
	return []

def parse_bp(name, tokens, base_indent):
	"Parse Behavior Protocol"
	
	tokens = preproc_bp(name, tokens)
	
	indent = base_indent
	output = ""
	
	for token in tokens:
		if (token == "\n"):
			continue
		
		if ((token == ";") or (token == "+") or (token == "||") or (token == "|")):
			output += " %s" % token
		elif (token == "("):
			output += "\n%s%s" % (tabs(indent), token)
			indent += 1
		elif (token == ")"):
			if (indent < base_indent):
				print "%s: Too many parentheses" % name
			
			indent -= 1
			output += "\n%s%s" % (tabs(indent), token)
		elif (token == "{"):
			output += " %s" % token
			indent += 1
		elif (token == "}"):
			if (indent < base_indent):
				print "%s: Too many parentheses" % name
			
			indent -= 1
			output += "\n%s%s" % (tabs(indent), token)
		elif (token == "*"):
			output += "%s" % token
		elif ((token == "!") or (token == "?") or (token == "NULL")):
			output += "\n%s%s" % (tabs(indent), token)
		else:
			output += "%s" % token
	
	if (indent > base_indent):
		print "%s: Missing parentheses" % name
	
	output = output.strip()
	if (output == ""):
		return "NULL"
	
	return output

def get_iface(name):
	"Get interface by name"
	
	global iface_properties
	
	if (name in iface_properties):
		return iface_properties[name]
	
	return None

def dump_frame(frame, outdir):
	"Dump Behavior Protocol of a given frame"
	
	outname = os.path.join(outdir, "%s.bp" % frame['name'])
	if (os.path.isfile(outname)):
		print "%s: File already exists, overwriting" % outname
	
	protocols = []
	if ('protocol' in frame):
		protocols.append(frame['protocol'])
	
	if ('provides' in frame):
		for provides in frame['provides']:
			iface = get_iface(provides['iface'])
			if (not iface is None):
				if ('protocol' in iface):
					protocols.append(extend_bp(outname, iface['protocol'], iface['name']))
			else:
				print "%s: Provided interface '%s' is undefined" % (frame['name'], provides['iface'])
	
	outf = file(outname, "w")
	outf.write(parse_bp(outname, merge_bp(protocols), 0))
	outf.close()

def get_system_arch():
	"Get system architecture"
	
	global arch_properties
	
	for arch, properties in arch_properties.items():
		if ('system' in properties):
			return properties
	
	return None

def get_arch(name):
	"Get architecture by name"
	
	global arch_properties
	
	if (name in arch_properties):
		return arch_properties[name]
	
	return None

def get_frame(name):
	"Get frame by name"
	
	global frame_properties
	
	if (name in frame_properties):
		return frame_properties[name]
	
	return None

def create_null_bp(name, outdir):
	"Create null frame protocol"
	
	outname = os.path.join(outdir, name)
	if (not os.path.isfile(outname)):
		outf = file(outname, "w")
		outf.write("NULL")
		outf.close()

def dump_arch(arch, outdir):
	"Dump architecture Behavior Protocol"
	
	outname = os.path.join(outdir, "%s.archbp" % arch['name'])
	if (os.path.isfile(outname)):
		print "%s: File already exists, overwriting" % outname
	
	outf = file(outname, "w")
	
	create_null_bp("null.bp", outdir)
	outf.write("frame \"null.bp\"\n\n")
	
	if ('inst' in arch):
		for inst in arch['inst']:
			subarch = get_arch(inst['type'])
			if (not subarch is None):
				outf.write("instantiate %s from \"%s.archbp\"\n" % (inst['var'], inst['type']))
				dump_arch(subarch, outdir)
			else:
				subframe = get_frame(inst['type'])
				if (not subframe is None):
					outf.write("instantiate %s from \"%s.bp\"\n" % (inst['var'], inst['type']))
					dump_frame(subframe, outdir)
				else:
					print "%s: '%s' is neither architecture nor frame" % (arch['name'], inst['type'])
		
		outf.write("\n")
	
	if ('bind' in arch):
		for bind in arch['bind']:
			outf.write("bind %s.%s to %s.%s\n" % (bind['from'][0], bind['from'][1], bind['to'][0], bind['to'][1]))
	
	outf.close()

def preproc_adl(raw, inarg):
	"Preprocess %% statements in ADL"
	
	return raw.replace("%%", inarg)

def parse_adl(base, root, inname, nested, indent):
	"Parse Architecture Description Language"
	
	global output
	global context
	global architecture
	global interface
	global frame
	global protocol
	
	global iface_properties
	global frame_properties
	global arch_properties
	
	global arg0
	
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
	
	for token in tokens:
		
		# Includes
		
		if (INC in context):
			context.remove(INC)
			parse_adl(base, nested_root, token, True, indent)
			context.add(POST_INC)
			continue
		
		if (POST_INC in context):
			if (token != "]"):
				print "%s: Expected ]" % inname
			
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
		
		if (token == "\n"):
			continue
		
		# "frame"
		
		if (FRAME in context):
			if (NULL in context):
				if (token != ";"):
					print "%s: Expected ';' in frame '%s'" % (inname, frame)
				else:
					output += "%s\n" % token
				
				context.remove(NULL)
				context.remove(FRAME)
				frame = None
				continue
			
			if (BODY in context):
				if (PROTOCOL in context):
					if (token == "{"):
						indent += 1
					elif (token == "}"):
						indent -= 1
					
					if (indent == -1):
						bp = split_bp(protocol)
						protocol = None
						
						if (not frame in frame_properties):
							frame_properties[frame] = {}
						
						if ('protocol' in frame_properties[frame]):
							print "%s: Protocol for frame '%s' already defined" % (inname, frame)
						else:
							frame_properties[frame]['protocol'] = bp
						
						output += "\n%s" % tabs(2)
						output += parse_bp(inname, bp, 2)
						output += "\n%s" % token
						indent = 0
						
						context.remove(PROTOCOL)
						context.remove(BODY)
						context.add(NULL)
					else:
						protocol += token
					
					continue
				
				if (REQUIRES in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ';' in frame '%s'" % (inname, frame)
						else:
							output += "%s" % token
						
						context.remove(FIN)
						continue
					
					if (VAR in context):
						if (not identifier(token)):
							print "%s: Variable name expected in frame '%s'" % (inname, frame)
						else:
							if (not frame in frame_properties):
								frame_properties[frame] = {}
							
							if (not 'requires' in frame_properties[frame]):
								frame_properties[frame]['requires'] = []
							
							frame_properties[frame]['requires'].append({'iface': arg0, 'var': token})
							arg0 = None
							
							output += "%s" % token
						
						context.remove(VAR)
						context.add(FIN)
						continue
					
					if ((token == "}") or (token[-1] == ":")):
						context.remove(REQUIRES)
					else:
						if (not identifier(token)):
							print "%s: Interface name expected in frame '%s'" % (inname, frame)
						else:
							arg0 = token
							output += "\n%s%s " % (tabs(indent), token)
						
						context.add(VAR)
						continue
				
				if (PROVIDES in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ';' in frame '%s'" % (inname, frame)
						else:
							output += "%s" % token
						
						context.remove(FIN)
						continue
					
					if (VAR in context):
						if (not identifier(token)):
							print "%s: Variable name expected in frame '%s'" % (inname, frame)
						else:
							if (not frame in frame_properties):
								frame_properties[frame] = {}
							
							if (not 'provides' in frame_properties[frame]):
								frame_properties[frame]['provides'] = []
							
							frame_properties[frame]['provides'].append({'iface': arg0, 'var': token})
							arg0 = None
							
							output += "%s" % token
						
						context.remove(VAR)
						context.add(FIN)
						continue
					
					if ((token == "}") or (token[-1] == ":")):
						context.remove(PROVIDES)
					else:
						if (not identifier(token)):
							print "%s: Interface name expected in frame '%s'" % (inname, frame)
						else:
							arg0 = token
							output += "\n%s%s " % (tabs(indent), token)
						
						context.add(VAR)
						continue
				
				if (token == "}"):
					if (indent != 2):
						print "%s: Wrong number of parentheses in frame '%s'" % (inname, frame)
					else:
						indent = 0
						output += "\n%s" % token
					
					context.remove(BODY)
					context.add(NULL)
					continue
				
				if (token == "provides:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					context.add(PROVIDES)
					protocol = ""
					continue
				
				if (token == "requires:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					context.add(REQUIRES)
					protocol = ""
					continue
				
				if (token == "protocol:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					indent = 0
					context.add(PROTOCOL)
					protocol = ""
					continue
				
				print "%s: Unknown token '%s' in frame '%s'" % (inname, token, frame)
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
					context.remove(FRAME)
					continue
				
				print "%s: Unknown token '%s' in frame head '%s'" % (inname, token, frame)
				
				continue
			
			if (not identifier(token)):
				print "%s: Expected frame name" % inname
			else:
				frame = token
				output += "%s " % token
				
				if (not frame in frame_properties):
					frame_properties[frame] = {}
				
				frame_properties[frame]['name'] = frame
			
			context.add(HEAD)
			continue
		
		# "interface"
		
		if (IFACE in context):
			if (NULL in context):
				if (token != ";"):
					print "%s: Expected ';' in interface '%s'" % (inname, interface)
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
					
					if (indent == -1):
						bp = split_bp(protocol)
						protocol = None
						
						if (not interface in iface_properties):
							iface_properties[interface] = {}
						
						if ('protocol' in iface_properties[interface]):
							print "%s: Protocol for interface '%s' already defined" % (inname, interface)
						else:
							iface_properties[interface]['protocol'] = bp
						
						output += "\n%s" % tabs(2)
						output += parse_bp(inname, bp, 2)
						output += "\n%s" % token
						indent = 0
						
						context.remove(PROTOCOL)
						context.remove(BODY)
						context.add(NULL)
					else:
						protocol += token
					
					continue
				
				if (PROTOTYPE in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ';' in interface '%s'" % (inname, interface)
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
							print "%s: Expected '(' in interface '%s'" % (inname, interface)
						else:
							output += "%s" % token
						
						context.remove(PAR_LEFT)
						context.add(SIGNATURE)
						continue
					
					if (not identifier(token)):
						print "%s: Method identifier expected in interface '%s'" % (inname, interface)
					else:
						output += "%s" % token
					
					context.add(PAR_LEFT)
					continue
				
				if (token == "}"):
					if (indent != 2):
						print "%s: Wrong number of parentheses in interface '%s'" % (inname, interface)
					else:
						indent = 0
						output += "\n%s" % token
					
					context.remove(BODY)
					context.add(NULL)
					continue
				
				if ((token == "ipcarg_t") or (token == "unative_t")):
					output += "\n%s%s " % (tabs(indent), token)
					context.add(PROTOTYPE)
					continue
				
				if (token == "protocol:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					indent = 0
					context.add(PROTOCOL)
					protocol = ""
					continue
				
				print "%s: Unknown token '%s' in interface '%s'" % (inname, token, interface)
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
					continue
				
				if (not word(token)):
					print "%s: Expected word in interface head '%s'" % (inname, interface)
				else:
					output += "%s " % token
				
				continue
			
			if (not identifier(token)):
				print "%s: Expected interface name" % inname
			else:
				interface = token
				output += "%s " % token
				
				if (not interface in iface_properties):
					iface_properties[interface] = {}
				
				iface_properties[interface]['name'] = interface
			
			context.add(HEAD)
			continue
		
		# "architecture"
		
		if (ARCH in context):
			if (NULL in context):
				if (token != ";"):
					print "%s: Expected ';' in architecture '%s'" % (inname, architecture)
				else:
					output += "%s\n" % token
				
				context.remove(NULL)
				context.remove(ARCH)
				context.discard(SYSTEM)
				architecture = None
				continue
			
			if (BODY in context):
				if (DELEGATE in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ';' in architecture '%s'" % (inname, architecture)
						else:
							output += "%s" % token
						
						context.remove(FIN)
						context.remove(DELEGATE)
						continue
					
					if (VAR in context):
						if (not descriptor(token)):
							print "%s: Expected interface descriptor in architecture '%s'" % (inname, architecture)
						else:
							if (not architecture in arch_properties):
								arch_properties[architecture] = {}
							
							if (not 'delegate' in arch_properties[architecture]):
								arch_properties[architecture]['delegate'] = []
							
							arch_properties[architecture]['delegate'].append({'from': arg0, 'to': token.split(":")})
							arg0 = None
							
							output += "%s" % token
						
						context.add(FIN)
						context.remove(VAR)
						continue
					
					if (TO in context):
						if (token != "to"):
							print "%s: Expected 'to' in architecture '%s'" % (inname, architecture)
						else:
							output += "%s " % token
						
						context.add(VAR)
						context.remove(TO)
						continue
					
					if (not identifier(token)):
						print "%s: Expected interface name in architecture '%s'" % (inname, architecture)
					else:
						output += "%s " % token
						arg0 = token
					
					context.add(TO)
					continue
				
				if (SUBSUME in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ';' in architecture '%s'" % (inname, architecture)
						else:
							output += "%s" % token
						
						context.remove(FIN)
						context.remove(SUBSUME)
						continue
					
					if (VAR in context):
						if (not identifier(token)):
							print "%s: Expected interface name in architecture '%s'" % (inname, architecture)
						else:
							if (not architecture in arch_properties):
								arch_properties[architecture] = {}
							
							if (not 'subsume' in arch_properties[architecture]):
								arch_properties[architecture]['subsume'] = []
							
							arch_properties[architecture]['subsume'].append({'from': arg0.split(":"), 'to': token})
							arg0 = None
							
							output += "%s" % token
						
						context.add(FIN)
						context.remove(VAR)
						continue
					
					if (TO in context):
						if (token != "to"):
							print "%s: Expected 'to' in architecture '%s'" % (inname, architecture)
						else:
							output += "%s " % token
						
						context.add(VAR)
						context.remove(TO)
						continue
					
					if (not descriptor(token)):
						print "%s: Expected interface descriptor in architecture '%s'" % (inname, architecture)
					else:
						output += "%s " % token
						arg0 = token
					
					context.add(TO)
					continue
				
				if (BIND in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ';' in architecture '%s'" % (inname, architecture)
						else:
							output += "%s" % token
						
						context.remove(FIN)
						context.remove(BIND)
						continue
					
					if (VAR in context):
						if (not descriptor(token)):
							print "%s: Expected second interface descriptor in architecture '%s'" % (inname, architecture)
						else:
							if (not architecture in arch_properties):
								arch_properties[architecture] = {}
							
							if (not 'bind' in arch_properties[architecture]):
								arch_properties[architecture]['bind'] = []
							
							arch_properties[architecture]['bind'].append({'from': arg0.split(":"), 'to': token.split(":")})
							arg0 = None
							
							output += "%s" % token
						
						context.add(FIN)
						context.remove(VAR)
						continue
					
					if (TO in context):
						if (token != "to"):
							print "%s: Expected 'to' in architecture '%s'" % (inname, architecture)
						else:
							output += "%s " % token
						
						context.add(VAR)
						context.remove(TO)
						continue
					
					if (not descriptor(token)):
						print "%s: Expected interface descriptor in architecture '%s'" % (inname, architecture)
					else:
						output += "%s " % token
						arg0 = token
					
					context.add(TO)
					continue
				
				if (INST in context):
					if (FIN in context):
						if (token != ";"):
							print "%s: Expected ';' in architecture '%s'" % (inname, architecture)
						else:
							output += "%s" % token
						
						context.remove(FIN)
						context.remove(INST)
						continue
					
					if (VAR in context):
						if (not identifier(token)):
							print "%s: Expected instance name in architecture '%s'" % (inname, architecture)
						else:
							if (not architecture in arch_properties):
								arch_properties[architecture] = {}
							
							if (not 'inst' in arch_properties[architecture]):
								arch_properties[architecture]['inst'] = []
							
							arch_properties[architecture]['inst'].append({'type': arg0, 'var': token})
							arg0 = None
							
							output += "%s" % token
						
						context.add(FIN)
						context.remove(VAR)
						continue
					
					if (not identifier(token)):
						print "%s: Expected frame/architecture type in architecture '%s'" % (inname, architecture)
					else:
						output += "%s " % token
						arg0 = token
					
					context.add(VAR)
					continue
				
				if (token == "}"):
					if (indent != 1):
						print "%s: Wrong number of parentheses in architecture '%s'" % (inname, architecture)
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
				
				if (token == "subsume"):
					output += "\n%s%s " % (tabs(indent), token)
					context.add(SUBSUME)
					continue
				
				if (token == "delegate"):
					output += "\n%s%s " % (tabs(indent), token)
					context.add(DELEGATE)
					continue
				
				print "%s: Unknown token '%s' in architecture '%s'" % (inname, token, architecture)
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
					print "%s: Expected word in architecture head '%s'" % (inname, architecture)
				else:
					output += "%s " % token
				
				continue
			
			if (not identifier(token)):
				print "%s: Expected architecture name" % inname
			else:
				architecture = token
				output += "%s " % token
				
				if (not architecture in arch_properties):
					arch_properties[architecture] = {}
				
				arch_properties[architecture]['name'] = architecture
				
				if (SYSTEM in context):
					arch_properties[architecture]['system'] = True
			
			context.add(HEAD)
			continue
		
		# "system architecture"
		
		if (SYSTEM in context):
			if (token != "architecture"):
				print "%s: Expected 'architecture'" % inname
			else:
				output += "%s " % token
			
			context.add(ARCH)
			continue
		
		if (token == "frame"):
			output += "\n%s " % token
			context.add(FRAME)
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
		
		print "%s: Unknown token '%s'" % (inname, token)
	
	inf.close()

def open_adl(base, root, inname, outdir, outname):
	"Open Architecture Description file"
	
	global output
	global context
	global architecture
	global interface
	global frame
	global protocol
	
	global arg0
	
	output = ""
	context = set()
	architecture = None
	interface = None
	frame = None
	protocol = None
	arg0 = None
	
	parse_adl(base, root, inname, False, 0)
	output = output.strip()
	
	if (output != ""):
		if (os.path.isfile(outname)):
			print "%s: File already exists, overwriting" % outname
		
		outf = file(outname, "w")
		outf.write(output)
		outf.close()
	else:
		if (os.path.isfile(outname)):
			print "%s: File already exists, but should be empty" % outname

def recursion(base, root, output, level):
	"Recursive directory walk"
	
	for name in os.listdir(root):
		canon = os.path.join(root, name)
		
		if (os.path.isfile(canon)):
			fcomp = split_tokens(canon, ["."])
			cname = canon.split("/")
			
			if (fcomp[-1] == ".adl"):
				output_path = os.path.join(output, cname[-1])
				open_adl(base, root, canon, output, output_path)
		
		if (os.path.isdir(canon)):
			recursion(base, canon, output, level + 1)

def main():
	global iface_properties
	global frame_properties
	global arch_properties
	
	if (len(sys.argv) < 2):
		usage(sys.argv[0])
		return
	
	path = os.path.abspath(sys.argv[1])
	if (not os.path.isdir(path)):
		print "Error: <OUTPUT> is not a directory"
		return
	
	iface_properties = {}
	frame_properties = {}
	arch_properties = {}
	
	recursion(".", ".", path, 0)
	
	system_arch = get_system_arch()
	
	if (not system_arch is None):
		dump_arch(system_arch, path)

if __name__ == '__main__':
	main()
