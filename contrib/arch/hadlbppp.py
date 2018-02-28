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
	INST, VAR, FIN, BIND, TO, SUBSUME, DELEGATE, IFACE, EXTENDS, PRE_BODY, \
	PROTOTYPE, PAR_LEFT, PAR_RIGHT, SIGNATURE, PROTOCOL, INITIALIZATION, \
	FINALIZATION, FRAME, PROVIDES, REQUIRES = range(29)

def usage(prname):
	"Print usage syntax"

	print("%s <--bp|--ebp|--adl|--dot|--nop>+ <OUTPUT>" % prname)
	print()
	print("--bp   Dump original Behavior Protocols (dChecker, BPSlicer)")
	print("--ebp  Dump Extended Behavior Protocols (bp2promela)")
	print("--adl  Dump Architecture Description Language (modified SOFA ADL/CDL)")
	print("--dot  Dump Dot architecture diagram (GraphViz)")
	print("--nop  Do not dump anything (just input files syntax check)")
	print()

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

def tentative_bp(name, tokens):
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
					result.extend(tentative_bp(name, tokens[start:(i - 1)]))
					result.append(")")
					result.append("+")
					result.append("NULL")
					if (i < len(tokens)):
						result.append(tokens[i])
				else:
					print("%s: Syntax error in tentative statement" % name)
			else:
				print("%s: Expected '{' for tentative statement" % name)
		else:
			result.append(tokens[i])

		i += 1

	return result

def alternative_bp(name, tokens):
	"Preprocess alternative statements in Behavior Protocol"

	result = []
	i = 0

	while (i < len(tokens)):
		if (tokens[i] == "alternative"):
			if ((i + 1 < len(tokens)) and (tokens[i + 1] == "(")):
				i += 2
				reps = []

				while ((i < len(tokens)) and (tokens[i] != ")")):
					reps.append(tokens[i])
					if ((i + 1 < len(tokens)) and (tokens[i + 1] == ";")):
						i += 2
					else:
						i += 1

				if (len(reps) >= 2):
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
							first = True

							for rep in reps[1:]:
								retokens = []
								for token in tokens[start:(i - 1)]:
									parts = token.split(".")
									if ((len(parts) == 2) and (parts[0] == reps[0])):
										retokens.append("%s.%s" % (rep, parts[1]))
									else:
										retokens.append(token)

								if (first):
									first = False
								else:
									result.append("+")

								result.append("(")
								result.extend(alternative_bp(name, retokens))
								result.append(")")

							if (i < len(tokens)):
								result.append(tokens[i])
						else:
							print("%s: Syntax error in alternative statement" % name)
					else:
						print("%s: Expected '{' for alternative statement body" % name)
				else:
					print("%s: At least one pattern and one replacement required for alternative statement" % name)
			else:
				print("%s: Expected '(' for alternative statement head" % name)
		else:
			result.append(tokens[i])

		i += 1

	return result

def split_bp(protocol):
	"Convert Behavior Protocol to tokens"

	return split_tokens(protocol, ["\n", " ", "\t", "(", ")", "{", "}", "*", ";", "+", "||", "|", "!", "?"], True, True)

def extend_bp(name, tokens, iface):
	"Convert interface Behavior Protocol to generic protocol"

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
				print("%s: Unexpected end of protocol" % name)

		i += 1

	return result

def merge_bp(initialization, finalization, protocols):
	"Merge several Behavior Protocols"

	indep = []

	if (len(protocols) > 1):
		first = True

		for protocol in protocols:
			if (first):
				first = False
			else:
				indep.append("|")

			indep.append("(")
			indep.extend(protocol)
			indep.append(")")
	elif (len(protocols) == 1):
		indep = protocols[0]

	inited = []

	if (initialization != None):
		if (len(indep) > 0):
			inited.append("(")
			inited.extend(initialization)
			inited.append(")")
			inited.append(";")
			inited.append("(")
			inited.extend(indep)
			inited.append(")")
		else:
			inited = initialization
	else:
		inited = indep

	finited = []

	if (finalization != None):
		if (len(inited) > 0):
			finited.append("(")
			finited.extend(inited)
			finited.append(")")
			finited.append(";")
			finited.append("(")
			finited.extend(finalization)
			finited.append(")")
		else:
			finited = finalization
	else:
		finited = inited

	return finited

def parse_bp(name, tokens, base_indent):
	"Parse Behavior Protocol"

	tokens = tentative_bp(name, tokens)
	tokens = alternative_bp(name, tokens)

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
				print("%s: Too many parentheses" % name)

			indent -= 1
			output += "\n%s%s" % (tabs(indent), token)
		elif (token == "{"):
			output += " %s" % token
			indent += 1
		elif (token == "}"):
			if (indent < base_indent):
				print("%s: Too many parentheses" % name)

			indent -= 1
			output += "\n%s%s" % (tabs(indent), token)
		elif (token == "*"):
			output += "%s" % token
		elif ((token == "!") or (token == "?") or (token == "NULL")):
			output += "\n%s%s" % (tabs(indent), token)
		else:
			output += "%s" % token

	if (indent > base_indent):
		print("%s: Missing parentheses" % name)

	output = output.strip()
	if (output == ""):
		return "NULL"

	return output

def parse_ebp(component, name, tokens, base_indent):
	"Parse Behavior Protocol and generate Extended Behavior Protocol output"

	return "component %s {\n\tbehavior {\n\t\t%s\n\t}\n}" % (component, parse_bp(name, tokens, base_indent + 2))

def get_iface(name):
	"Get interface by name"

	global iface_properties

	if (name in iface_properties):
		return iface_properties[name]

	return None

def inherited_protocols(iface):
	"Get protocols inherited by an interface"

	result = []

	if ('extends' in iface):
		supiface = get_iface(iface['extends'])
		if (not supiface is None):
			if ('protocol' in supiface):
				result.append(supiface['protocol'])
			result.extend(inherited_protocols(supiface))
		else:
			print("%s: Extends unknown interface '%s'" % (iface['name'], iface['extends']))

	return result

def dump_frame(directed_binds, frame, outdir, var, archf):
	"Dump Behavior Protocol of a given frame"

	global opt_bp
	global opt_ebp

	if (opt_ebp):
		fname = "%s.ebp" % frame['name']
	else:
		fname = "%s.bp" % frame['name']

	if (archf != None):
		archf.write("instantiate %s from \"%s\"\n" % (var, fname))

	outname = os.path.join(outdir, fname)

	protocols = []
	if ('protocol' in frame):
		protocols.append(frame['protocol'])

	if ('initialization' in frame):
		initialization = frame['initialization']
	else:
		initialization = None

	if ('finalization' in frame):
		finalization = frame['finalization']
	else:
		finalization = None

	if ('provides' in frame):
		for provides in frame['provides']:
			iface = get_iface(provides['iface'])
			if (not iface is None):
				binds = directed_binds['%s.%s' % (var, provides['iface'])]
				if (not binds is None):
					cnt = len(binds)
				else:
					cnt = 1

				if ('protocol' in iface):
					proto = extend_bp(outname, iface['protocol'], iface['name'])
					for _ in range(0, cnt):
						protocols.append(proto)

				for protocol in inherited_protocols(iface):
					proto = extend_bp(outname, protocol, iface['name'])
					for _ in range(0, cnt):
						protocols.append(proto)
			else:
				print("%s: Provided interface '%s' is undefined" % (frame['name'], provides['iface']))

	if (opt_bp):
		outf = open(outname, "w")
		outf.write(parse_bp(outname, merge_bp(initialization, finalization, protocols), 0))
		outf.close()

	if (opt_ebp):
		outf = open(outname, "w")
		outf.write(parse_ebp(frame['name'], outname, merge_bp(initialization, finalization, protocols), 0))
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

def create_null_bp(fname, outdir, archf):
	"Create null frame protocol"

	global opt_bp
	global opt_ebp

	if (archf != None):
		archf.write("frame \"%s\"\n" % fname)

	outname = os.path.join(outdir, fname)

	if (opt_bp):
		outf = open(outname, "w")
		outf.write("NULL")
		outf.close()

	if (opt_ebp):
		outf = open(outname, "w")
		outf.write("component null {\n\tbehavior {\n\t\tNULL\n\t}\n}")
		outf.close()

def flatten_binds(binds, delegates, subsumes):
	"Remove bindings which are replaced by delegation or subsumption"

	result = []
	stable = True

	for bind in binds:
		keep = True

		for delegate in delegates:
			if (bind['to'] == delegate['to']):
				keep = False
				result.append({'from': bind['from'], 'to': delegate['rep']})

		for subsume in subsumes:
			if (bind['from'] == subsume['from']):
				keep = False
				result.append({'from': subsume['rep'], 'to': bind['to']})

		if (keep):
			result.append(bind)
		else:
			stable = False

	if (stable):
		return result
	else:
		return flatten_binds(result, delegates, subsumes)

def direct_binds(binds):
	"Convert bindings matrix to set of sources by destination"

	result = {}

	for bind in binds:
		if (not bind['to'] in result):
			result[bind['to']] = set()

		result[bind['to']].add(bind['from'])

	return result

def merge_arch(prefix, arch, outdir):
	"Merge subarchitecture into architecture"

	insts = []
	binds = []
	delegates = []
	subsumes = []

	if ('inst' in arch):
		for inst in arch['inst']:
			subarch = get_arch(inst['type'])
			if (not subarch is None):
				(subinsts, subbinds, subdelegates, subsubsumes) = merge_arch("%s_%s" % (prefix, subarch['name']), subarch, outdir)
				insts.extend(subinsts)
				binds.extend(subbinds)
				delegates.extend(subdelegates)
				subsumes.extend(subsubsumes)
			else:
				subframe = get_frame(inst['type'])
				if (not subframe is None):
					insts.append({'var': "%s_%s" % (prefix, inst['var']), 'frame': subframe})
				else:
					print("%s: '%s' is neither an architecture nor a frame" % (arch['name'], inst['type']))

	if ('bind' in arch):
		for bind in arch['bind']:
			binds.append({'from': "%s_%s.%s" % (prefix, bind['from'][0], bind['from'][1]), 'to': "%s_%s.%s" % (prefix, bind['to'][0], bind['to'][1])})

	if ('delegate' in arch):
		for delegate in arch['delegate']:
			delegates.append({'to': "%s.%s" % (prefix, delegate['from']), 'rep': "%s_%s.%s" % (prefix, delegate['to'][0], delegate['to'][1])})

	if ('subsume' in arch):
		for subsume in arch['subsume']:
			subsumes.append({'from': "%s.%s" % (prefix, subsume['to']), 'rep': "%s_%s.%s" % (prefix, subsume['from'][0], subsume['from'][1])})

	return (insts, binds, delegates, subsumes)

def dump_archbp(outdir):
	"Dump system architecture Behavior Protocol"

	global opt_bp
	global opt_ebp

	arch = get_system_arch()

	if (arch is None):
		print("Unable to find system architecture")
		return

	insts = []
	binds = []
	delegates = []
	subsumes = []

	if ('inst' in arch):
		for inst in arch['inst']:
			subarch = get_arch(inst['type'])
			if (not subarch is None):
				(subinsts, subbinds, subdelegates, subsubsumes) = merge_arch(subarch['name'], subarch, outdir)
				insts.extend(subinsts)
				binds.extend(subbinds)
				delegates.extend(subdelegates)
				subsumes.extend(subsubsumes)
			else:
				subframe = get_frame(inst['type'])
				if (not subframe is None):
					insts.append({'var': inst['var'], 'frame': subframe})
				else:
					print("%s: '%s' is neither an architecture nor a frame" % (arch['name'], inst['type']))

	if ('bind' in arch):
		for bind in arch['bind']:
			binds.append({'from': "%s.%s" % (bind['from'][0], bind['from'][1]), 'to': "%s.%s" % (bind['to'][0], bind['to'][1])})

	if ('delegate' in arch):
		for delegate in arch['delegate']:
			print("Unable to delegate interface in system architecture")
			break

	if ('subsume' in arch):
		for subsume in arch['subsume']:
			print("Unable to subsume interface in system architecture")
			break

	directed_binds = direct_binds(flatten_binds(binds, delegates, subsumes))

	outname = os.path.join(outdir, "%s.archbp" % arch['name'])
	if ((opt_bp) or (opt_ebp)):
		outf = open(outname, "w")
	else:
		outf = None

	create_null_bp("null.bp", outdir, outf)

	for inst in insts:
		dump_frame(directed_binds, inst['frame'], outdir, inst['var'], outf)

	for dst, src in directed_binds.items():
		if (outf != None):
			outf.write("bind %s to %s\n" % (", ".join(src), dst))

	if (outf != None):
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
	global initialization
	global finalization

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
			print("%s: Unable to include file %s" % (inname, path))
			return ""
	else:
		inarg = "%%"
		path = inname
		nested_root = root

	inf = open(path, "r")

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
				print("%s: Expected ]" % inname)

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
					print("%s: Expected ';' in frame '%s'" % (inname, frame))
				else:
					output += "%s\n" % token

				context.remove(NULL)
				context.remove(FRAME)
				frame = None
				continue

			if (BODY in context):
				if (FINALIZATION in context):
					if (token == "{"):
						indent += 1
					elif (token == "}"):
						indent -= 1

					if (((token[-1] == ":") and (indent == 0)) or (indent == -1)):
						bp = split_bp(finalization)
						finalization = None

						if (not frame in frame_properties):
							frame_properties[frame] = {}

						if ('finalization' in frame_properties[frame]):
							print("%s: Finalization protocol for frame '%s' already defined" % (inname, frame))
						else:
							frame_properties[frame]['finalization'] = bp

						output += "\n%s" % tabs(2)
						output += parse_bp(inname, bp, 2)

						context.remove(FINALIZATION)
						if (indent == -1):
							output += "\n%s" % token
							context.remove(BODY)
							context.add(NULL)
							indent = 0
							continue
						else:
							indent = 2
					else:
						finalization += token
						continue

				if (INITIALIZATION in context):
					if (token == "{"):
						indent += 1
					elif (token == "}"):
						indent -= 1

					if (((token[-1] == ":") and (indent == 0)) or (indent == -1)):
						bp = split_bp(initialization)
						initialization = None

						if (not frame in frame_properties):
							frame_properties[frame] = {}

						if ('initialization' in frame_properties[frame]):
							print("%s: Initialization protocol for frame '%s' already defined" % (inname, frame))
						else:
							frame_properties[frame]['initialization'] = bp

						output += "\n%s" % tabs(2)
						output += parse_bp(inname, bp, 2)

						context.remove(INITIALIZATION)
						if (indent == -1):
							output += "\n%s" % token
							context.remove(BODY)
							context.add(NULL)
							indent = 0
							continue
						else:
							indent = 2
					else:
						initialization += token
						continue

				if (PROTOCOL in context):
					if (token == "{"):
						indent += 1
					elif (token == "}"):
						indent -= 1

					if (((token[-1] == ":") and (indent == 0)) or (indent == -1)):
						bp = split_bp(protocol)
						protocol = None

						if (not frame in frame_properties):
							frame_properties[frame] = {}

						if ('protocol' in frame_properties[frame]):
							print("%s: Protocol for frame '%s' already defined" % (inname, frame))
						else:
							frame_properties[frame]['protocol'] = bp

						output += "\n%s" % tabs(2)
						output += parse_bp(inname, bp, 2)

						context.remove(PROTOCOL)
						if (indent == -1):
							output += "\n%s" % token
							context.remove(BODY)
							context.add(NULL)
							indent = 0
							continue
						else:
							indent = 2
					else:
						protocol += token
						continue

				if (REQUIRES in context):
					if (FIN in context):
						if (token != ";"):
							print("%s: Expected ';' in frame '%s'" % (inname, frame))
						else:
							output += "%s" % token

						context.remove(FIN)
						continue

					if (VAR in context):
						if (not identifier(token)):
							print("%s: Variable name expected in frame '%s'" % (inname, frame))
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
							print("%s: Interface name expected in frame '%s'" % (inname, frame))
						else:
							arg0 = token
							output += "\n%s%s " % (tabs(indent), token)

						context.add(VAR)
						continue

				if (PROVIDES in context):
					if (FIN in context):
						if (token != ";"):
							print("%s: Expected ';' in frame '%s'" % (inname, frame))
						else:
							output += "%s" % token

						context.remove(FIN)
						continue

					if (VAR in context):
						if (not identifier(token)):
							print("%s: Variable name expected in frame '%s'" % (inname, frame))
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
							print("%s: Interface name expected in frame '%s'" % (inname, frame))
						else:
							arg0 = token
							output += "\n%s%s " % (tabs(indent), token)

						context.add(VAR)
						continue

				if (token == "}"):
					if (indent != 2):
						print("%s: Wrong number of parentheses in frame '%s'" % (inname, frame))
					else:
						indent = 0
						output += "\n%s" % token

					context.remove(BODY)
					context.add(NULL)
					continue

				if (token == "provides:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					context.add(PROVIDES)
					continue

				if (token == "requires:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					context.add(REQUIRES)
					continue

				if (token == "initialization:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					indent = 0
					context.add(INITIALIZATION)
					initialization = ""
					continue

				if (token == "finalization:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					indent = 0
					context.add(FINALIZATION)
					finalization = ""
					continue

				if (token == "protocol:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					indent = 0
					context.add(PROTOCOL)
					protocol = ""
					continue

				print("%s: Unknown token '%s' in frame '%s'" % (inname, token, frame))
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

				print("%s: Unknown token '%s' in frame head '%s'" % (inname, token, frame))

				continue

			if (not identifier(token)):
				print("%s: Expected frame name" % inname)
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
					print("%s: Expected ';' in interface '%s'" % (inname, interface))
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
							print("%s: Protocol for interface '%s' already defined" % (inname, interface))
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
							print("%s: Expected ';' in interface '%s'" % (inname, interface))
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
							print("%s: Expected '(' in interface '%s'" % (inname, interface))
						else:
							output += "%s" % token

						context.remove(PAR_LEFT)
						context.add(SIGNATURE)
						continue

					if (not identifier(token)):
						print("%s: Method identifier expected in interface '%s'" % (inname, interface))
					else:
						output += "%s" % token

					context.add(PAR_LEFT)
					continue

				if (token == "}"):
					if (indent != 2):
						print("%s: Wrong number of parentheses in interface '%s'" % (inname, interface))
					else:
						indent = 0
						output += "\n%s" % token

					context.remove(BODY)
					context.add(NULL)
					continue

				if (token == "sysarg_t"):
					output += "\n%s%s " % (tabs(indent), token)
					context.add(PROTOTYPE)
					continue

				if (token == "protocol:"):
					output += "\n%s%s" % (tabs(indent - 1), token)
					indent = 0
					context.add(PROTOCOL)
					protocol = ""
					continue

				print("%s: Unknown token '%s' in interface '%s'" % (inname, token, interface))
				continue

			if (HEAD in context):
				if (PRE_BODY in context):
					if (token == "{"):
						output += "%s" % token
						indent += 2
						context.remove(PRE_BODY)
						context.remove(HEAD)
						context.add(BODY)
						continue

					if (token == ";"):
						output += "%s\n" % token
						context.remove(PRE_BODY)
						context.remove(HEAD)
						context.remove(IFACE)
						continue

					print("%s: Expected '{' or ';' in interface head '%s'" % (inname, interface))
					continue

				if (EXTENDS in context):
					if (not identifier(token)):
						print("%s: Expected inherited interface name in interface head '%s'" % (inname, interface))
					else:
						output += "%s " % token
						if (not interface in iface_properties):
							iface_properties[interface] = {}

						iface_properties[interface]['extends'] = token

					context.remove(EXTENDS)
					context.add(PRE_BODY)
					continue

				if (token == "extends"):
					output += "%s " % token
					context.add(EXTENDS)
					continue

				if (token == "{"):
					output += "%s" % token
					indent += 2
					context.remove(HEAD)
					context.add(BODY)
					continue

				if (token == ";"):
					output += "%s\n" % token
					context.remove(HEAD)
					context.remove(IFACE)
					continue

				print("%s: Expected 'extends', '{' or ';' in interface head '%s'" % (inname, interface))
				continue

			if (not identifier(token)):
				print("%s: Expected interface name" % inname)
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
					print("%s: Expected ';' in architecture '%s'" % (inname, architecture))
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
							print("%s: Expected ';' in architecture '%s'" % (inname, architecture))
						else:
							output += "%s" % token

						context.remove(FIN)
						context.remove(DELEGATE)
						continue

					if (VAR in context):
						if (not descriptor(token)):
							print("%s: Expected interface descriptor in architecture '%s'" % (inname, architecture))
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
							print("%s: Expected 'to' in architecture '%s'" % (inname, architecture))
						else:
							output += "%s " % token

						context.add(VAR)
						context.remove(TO)
						continue

					if (not identifier(token)):
						print("%s: Expected interface name in architecture '%s'" % (inname, architecture))
					else:
						output += "%s " % token
						arg0 = token

					context.add(TO)
					continue

				if (SUBSUME in context):
					if (FIN in context):
						if (token != ";"):
							print("%s: Expected ';' in architecture '%s'" % (inname, architecture))
						else:
							output += "%s" % token

						context.remove(FIN)
						context.remove(SUBSUME)
						continue

					if (VAR in context):
						if (not identifier(token)):
							print("%s: Expected interface name in architecture '%s'" % (inname, architecture))
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
							print("%s: Expected 'to' in architecture '%s'" % (inname, architecture))
						else:
							output += "%s " % token

						context.add(VAR)
						context.remove(TO)
						continue

					if (not descriptor(token)):
						print("%s: Expected interface descriptor in architecture '%s'" % (inname, architecture))
					else:
						output += "%s " % token
						arg0 = token

					context.add(TO)
					continue

				if (BIND in context):
					if (FIN in context):
						if (token != ";"):
							print("%s: Expected ';' in architecture '%s'" % (inname, architecture))
						else:
							output += "%s" % token

						context.remove(FIN)
						context.remove(BIND)
						continue

					if (VAR in context):
						if (not descriptor(token)):
							print("%s: Expected second interface descriptor in architecture '%s'" % (inname, architecture))
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
							print("%s: Expected 'to' in architecture '%s'" % (inname, architecture))
						else:
							output += "%s " % token

						context.add(VAR)
						context.remove(TO)
						continue

					if (not descriptor(token)):
						print("%s: Expected interface descriptor in architecture '%s'" % (inname, architecture))
					else:
						output += "%s " % token
						arg0 = token

					context.add(TO)
					continue

				if (INST in context):
					if (FIN in context):
						if (token != ";"):
							print("%s: Expected ';' in architecture '%s'" % (inname, architecture))
						else:
							output += "%s" % token

						context.remove(FIN)
						context.remove(INST)
						continue

					if (VAR in context):
						if (not identifier(token)):
							print("%s: Expected instance name in architecture '%s'" % (inname, architecture))
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
						print("%s: Expected frame/architecture type in architecture '%s'" % (inname, architecture))
					else:
						output += "%s " % token
						arg0 = token

					context.add(VAR)
					continue

				if (token == "}"):
					if (indent != 1):
						print("%s: Wrong number of parentheses in architecture '%s'" % (inname, architecture))
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

				print("%s: Unknown token '%s' in architecture '%s'" % (inname, token, architecture))
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
					print("%s: Expected word in architecture head '%s'" % (inname, architecture))
				else:
					output += "%s " % token

				continue

			if (not identifier(token)):
				print("%s: Expected architecture name" % inname)
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
				print("%s: Expected 'architecture'" % inname)
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

		print("%s: Unknown token '%s'" % (inname, token))

	inf.close()

def open_adl(base, root, inname, outdir, outname):
	"Open Architecture Description file"

	global output
	global context
	global architecture
	global interface
	global frame
	global protocol
	global initialization
	global finalization

	global arg0

	global opt_adl

	output = ""
	context = set()
	architecture = None
	interface = None
	frame = None
	protocol = None
	initialization = None
	finalization = None
	arg0 = None

	parse_adl(base, root, inname, False, 0)
	output = output.strip()

	if ((output != "") and (opt_adl)):
		outf = open(outname, "w")
		outf.write(output)
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
				open_adl(base, root, canon, output, output_path)

		if (os.path.isdir(canon)):
			recursion(base, canon, output, level + 1)

def merge_dot_frame(prefix, name, frame, outf, indent):
	"Dump Dot frame"

	outf.write("%ssubgraph cluster_%s {\n" % (tabs(indent), prefix))
	outf.write("%s\tlabel=\"%s\";\n" % (tabs(indent), name))
	outf.write("%s\tstyle=filled;\n" % tabs(indent))
	outf.write("%s\tcolor=red;\n" % tabs(indent))
	outf.write("%s\tfillcolor=yellow;\n" % tabs(indent))
	outf.write("%s\t\n" % tabs(indent))

	if ('provides' in frame):
		outf.write("%s\t%s__provides [label=\"\", shape=doublecircle, style=filled, color=green, fillcolor=yellow];\n" % (tabs(indent), prefix))

	if ('requires' in frame):
		outf.write("%s\t%s__requires [label=\"\", shape=circle, style=filled, color=red, fillcolor=yellow];\n" % (tabs(indent), prefix))

	outf.write("%s}\n" % tabs(indent))
	outf.write("%s\n" % tabs(indent))

def merge_dot_arch(prefix, name, arch, outf, indent):
	"Dump Dot subarchitecture"

	outf.write("%ssubgraph cluster_%s {\n" % (tabs(indent), prefix))
	outf.write("%s\tlabel=\"%s\";\n" % (tabs(indent), name))
	outf.write("%s\tcolor=red;\n" % tabs(indent))
	outf.write("%s\t\n" % tabs(indent))

	if ('inst' in arch):
		for inst in arch['inst']:
			subarch = get_arch(inst['type'])
			if (not subarch is None):
				merge_dot_arch("%s_%s" % (prefix, inst['var']), inst['var'], subarch, outf, indent + 1)
			else:
				subframe = get_frame(inst['type'])
				if (not subframe is None):
					merge_dot_frame("%s_%s" % (prefix, inst['var']), inst['var'], subframe, outf, indent + 1)
				else:
					print("%s: '%s' is neither an architecture nor a frame" % (arch['name'], inst['type']))

	if ('bind' in arch):
		labels = {}
		for bind in arch['bind']:
			if (bind['from'][1] != bind['to'][1]):
				label = "%s:%s" % (bind['from'][1], bind['to'][1])
			else:
				label = bind['from'][1]

			if (not (bind['from'][0], bind['to'][0]) in labels):
				labels[(bind['from'][0], bind['to'][0])] = []

			labels[(bind['from'][0], bind['to'][0])].append(label)

		for bind in arch['bind']:
			if (not (bind['from'][0], bind['to'][0]) in labels):
				continue

			attrs = []

			if (bind['from'][0] != bind['to'][0]):
				attrs.append("ltail=cluster_%s_%s" % (prefix, bind['from'][0]))
				attrs.append("lhead=cluster_%s_%s" % (prefix, bind['to'][0]))

			attrs.append("label=\"%s\"" % "\\n".join(labels[(bind['from'][0], bind['to'][0])]))
			del labels[(bind['from'][0], bind['to'][0])]

			outf.write("%s\t%s_%s__requires -> %s_%s__provides [%s];\n" % (tabs(indent), prefix, bind['from'][0], prefix, bind['to'][0], ", ".join(attrs)))

	if ('delegate' in arch):
		outf.write("%s\t%s__provides [label=\"\", shape=doublecircle, color=green];\n" % (tabs(indent), prefix))

		labels = {}
		for delegate in arch['delegate']:
			if (delegate['from'] != delegate['to'][1]):
				label = "%s:%s" % (delegate['from'], delegate['to'][1])
			else:
				label = delegate['from']

			if (not delegate['to'][0] in labels):
				labels[delegate['to'][0]] = []

			labels[delegate['to'][0]].append(label)

		for delegate in arch['delegate']:
			if (not delegate['to'][0] in labels):
				continue

			attrs = []
			attrs.append("color=gray")
			attrs.append("lhead=cluster_%s_%s" % (prefix, delegate['to'][0]))
			attrs.append("label=\"%s\"" % "\\n".join(labels[delegate['to'][0]]))
			del labels[delegate['to'][0]]

			outf.write("%s\t%s__provides -> %s_%s__provides [%s];\n" % (tabs(indent), prefix, prefix, delegate['to'][0], ", ".join(attrs)))

	if ('subsume' in arch):
		outf.write("%s\t%s__requires [label=\"\", shape=circle, color=red];\n" % (tabs(indent), prefix))

		labels = {}
		for subsume in arch['subsume']:
			if (subsume['from'][1] != subsume['to']):
				label = "%s:%s" % (subsume['from'][1], subsume['to'])
			else:
				label = subsume['to']

			if (not subsume['from'][0] in labels):
				labels[subsume['from'][0]] = []

			labels[subsume['from'][0]].append(label)

		for subsume in arch['subsume']:
			if (not subsume['from'][0] in labels):
				continue

			attrs = []
			attrs.append("color=gray")
			attrs.append("ltail=cluster_%s_%s" % (prefix, subsume['from'][0]))
			attrs.append("label=\"%s\"" % "\\n".join(labels[subsume['from'][0]]))
			del labels[subsume['from'][0]]

			outf.write("%s\t%s_%s__requires -> %s__requires [%s];\n" % (tabs(indent), prefix, subsume['from'][0], prefix, ", ".join(attrs)))

	outf.write("%s}\n" % tabs(indent))
	outf.write("%s\n" % tabs(indent))

def dump_dot(outdir):
	"Dump Dot architecture"

	global opt_dot

	arch = get_system_arch()

	if (arch is None):
		print("Unable to find system architecture")
		return

	if (opt_dot):
		outname = os.path.join(outdir, "%s.dot" % arch['name'])
		outf = open(outname, "w")

		outf.write("digraph {\n")
		outf.write("\tlabel=\"%s\";\n" % arch['name'])
		outf.write("\tcompound=true;\n")
		outf.write("\tsplines=\"polyline\";\n")
		outf.write("\tedge [fontsize=8];\n")
		outf.write("\t\n")

		if ('inst' in arch):
			for inst in arch['inst']:
				subarch = get_arch(inst['type'])
				if (not subarch is None):
					merge_dot_arch(inst['var'], inst['var'], subarch, outf, 1)
				else:
					subframe = get_frame(inst['type'])
					if (not subframe is None):
						merge_dot_frame("%s" % inst['var'], inst['var'], subframe, outf, 1)
					else:
						print("%s: '%s' is neither an architecture nor a frame" % (arch['name'], inst['type']))

		if ('bind' in arch):
			labels = {}
			for bind in arch['bind']:
				if (bind['from'][1] != bind['to'][1]):
					label = "%s:%s" % (bind['from'][1], bind['to'][1])
				else:
					label = bind['from'][1]

				if (not (bind['from'][0], bind['to'][0]) in labels):
					labels[(bind['from'][0], bind['to'][0])] = []

				labels[(bind['from'][0], bind['to'][0])].append(label)

			for bind in arch['bind']:
				if (not (bind['from'][0], bind['to'][0]) in labels):
					continue

				attrs = []

				if (bind['from'][0] != bind['to'][0]):
					attrs.append("ltail=cluster_%s" % bind['from'][0])
					attrs.append("lhead=cluster_%s" % bind['to'][0])

				attrs.append("label=\"%s\"" % "\\n".join(labels[(bind['from'][0], bind['to'][0])]))
				del labels[(bind['from'][0], bind['to'][0])]

				outf.write("\t%s__requires -> %s__provides [%s];\n" % (bind['from'][0], bind['to'][0], ", ".join(attrs)))

		if ('delegate' in arch):
			for delegate in arch['delegate']:
				print("Unable to delegate interface in system architecture")
				break

		if ('subsume' in arch):
			for subsume in arch['subsume']:
				print("Unable to subsume interface in system architecture")
				break

		outf.write("}\n")

		outf.close()

def main():
	global iface_properties
	global frame_properties
	global arch_properties
	global opt_bp
	global opt_ebp
	global opt_adl
	global opt_dot

	if (len(sys.argv) < 3):
		usage(sys.argv[0])
		return

	opt_bp = False
	opt_ebp = False
	opt_adl = False
	opt_dot = False

	for arg in sys.argv[1:(len(sys.argv) - 1)]:
		if (arg == "--bp"):
			opt_bp = True
		elif (arg == "--ebp"):
			opt_ebp = True
		elif (arg == "--adl"):
			opt_adl = True
		elif (arg == "--dot"):
			opt_dot = True
		elif (arg == "--nop"):
			pass
		else:
			print("Error: Unknown command line option '%s'" % arg)
			return

	if ((opt_bp) and (opt_ebp)):
		print("Error: Cannot dump both original Behavior Protocols and Extended Behavior Protocols")
		return

	path = os.path.abspath(sys.argv[-1])
	if (not os.path.isdir(path)):
		print("Error: <OUTPUT> is not a directory")
		return

	iface_properties = {}
	frame_properties = {}
	arch_properties = {}

	recursion(".", ".", path, 0)
	dump_archbp(path)
	dump_dot(path)

if __name__ == '__main__':
	main()
