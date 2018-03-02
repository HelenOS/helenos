#
# Copyright (c) 2008 Martin Decky
# Copyright (c) 2011 Martin Sucha
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
Convert descriptive structure definitions to structure object
"""

import struct
import sys
import types

# Handle long integer conversions nicely in both Python 2 and Python 3
integer_types = (int, long) if sys.version < '3' else (int,)

# Ensure that 's' format for struct receives correct data type depending
# on Python version (needed due to different way to encode into bytes)
ensure_string = \
	(lambda value: value if type(value) is str else bytes(value)) \
		if sys.version < '3' else \
	(lambda value: bytes(value, 'ascii') if type(value) is str else value)

ranges = {
	'B': (integer_types, 0x00, 0xff),
	'H': (integer_types, 0x0000, 0xffff),
	'L': (integer_types, 0x00000000, 0xffffffff),
	'Q': (integer_types, 0x0000000000000000, 0xffffffffffffffff),
	'b': (integer_types, -0x80, 0x7f),
	'h': (integer_types, -0x8000, 0x7fff),
	'l': (integer_types, -0x80000000, 0x7fffffff) ,
	'q': (integer_types, -0x8000000000000000, 0x7fffffffffffffff),
}

def check_range(varname, fmt, value):
	if value == None:
		raise ValueError('Variable "%s" not set' % varname)
	if not fmt in ranges:
		return
	vartype, varmin, varmax = ranges[fmt]
	if not isinstance(value, vartype):
		raise ValueError('Variable "%s" is %s but should be %s' %
		                 (varname, str(type(value)), str(vartype)))
	if value < varmin or value > varmax:
		raise ValueError('Variable "%s" value %s out of range %s..%s' %
		                 (varname, repr(value), repr(varmin), repr(varmax)))

class Struct:
	def size(self):
		return struct.calcsize(self._format_)

	def pack(self):
		args = []
		for variable, fmt, length in self._args_:
			value = self.__dict__[variable]
			if isinstance(value, list):
				if length != None and length != len(value):
					raise ValueError('Variable "%s" length %u does not match %u' %
				                      (variable, len(value), length))
				for index, item in enumerate(value):
					check_range(variable + '[' + repr(index) + ']', fmt, item)
					args.append(item)
			else:
				if (fmt == "s"):
					value = ensure_string(value)
				check_range(variable, fmt, value)
				args.append(value)
		return struct.pack(self._format_, *args)

	def unpack(self, data):
		values = struct.unpack(self._format_, data)
		i = 0
		for variable, fmt, length in self._args_:
			self.__dict__[variable] = values[i]
			i += 1

def create(definition):
	"Create structure object"

	tokens = definition.split(None)

	# Initial byte order tag
	format = {
		"little:":  lambda: "<",
		"big:":     lambda: ">",
		"network:": lambda: "!"
	}[tokens[0]]()
	inst = Struct()
	args = []

	# Member tags
	comment = False
	variable = None
	for token in tokens[1:]:
		if (comment):
			if (token == "*/"):
				comment = False
			continue

		if (token == "/*"):
			comment = True
			continue

		if (variable != None):
			subtokens = token.split("[")

			length = None
			if (len(subtokens) > 1):
				length = int(subtokens[1].split("]")[0])
				format += "%d" % length

			format += variable

			inst.__dict__[subtokens[0]] = None
			args.append((subtokens[0], variable, length))

			variable = None
			continue

		if (token[0:8] == "padding["):
			size = token[8:].split("]")[0]
			format += "%dx" % int(size)
			continue

		variable = {
			"char":     lambda: "s",
			"uint8_t":  lambda: "B",
			"uint16_t": lambda: "H",
			"uint32_t": lambda: "L",
			"uint64_t": lambda: "Q",

			"int8_t":   lambda: "b",
			"int16_t":  lambda: "h",
			"int32_t":  lambda: "l",
			"int64_t":  lambda: "q"
		}[token]()

	inst.__dict__['_format_'] = format
	inst.__dict__['_args_'] = args
	return inst
