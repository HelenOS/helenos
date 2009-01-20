#!/usr/bin/env python
#
# Copyright (c) 2006 Ondrej Palkovsky
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
HelenOS configuration system
"""
import sys
import os
import re
import commands
import snack

INPUT = sys.argv[1]
OUTPUT = 'Makefile.config'

def read_defaults(fname, defaults):
	"Read saved values from last configuration run"
	
	inf = file(fname,'r')
	
	for line in inf:
		res = re.match(r'^(?:#!# )?([^#]\w*)\s*=\s*(.*?)\s*$', line)
		if (res):
			defaults[res.group(1)] = res.group(2)
	
	inf.close()

def check_condition(text, defaults, ask_names):
	"Check for condition"
	
	ctype = 'cnf'
	
	if ((')|' in text) or ('|(' in text)):
		ctype = 'dnf'
	
	if (ctype == 'cnf'):
		conds = text.split('&')
	else:
		conds = text.split('|')
	
	for cond in conds:
		if (cond.startswith('(')) and (cond.endswith(')')):
			cond = cond[1:-1]
		
		inside = check_inside(cond, defaults, ctype)
		
		if (ctype == 'cnf') and (not inside):
			return False
		
		if (ctype == 'dnf') and (inside):
			return True
	
	if (ctype == 'cnf'):
		return True
	return False

def check_inside(text, defaults, ctype):
	"Check that the condition specified on input line is True (only CNF is supported)"
	
	if (ctype == 'cnf'):
		conds = text.split('|')
	else:
		conds = text.split('&')
	
	for cond in conds:
		res = re.match(r'^(.*?)(!?=)(.*)$', cond)
		if (not res):
			raise RuntimeError("Invalid condition: %s" % cond)
		
		condname = res.group(1)
		oper = res.group(2)
		condval = res.group(3)
		
		if (not defaults.has_key(condname)):
			varval = ''
		else:
			varval = defaults[condname]
		
		if (ctype == 'cnf'):
			if (oper == '=') and (condval == varval):
				return True
		
			if (oper == '!=') and (condval != varval):
				return True
		else:
			if (oper == '=') and (condval != varval):
				return False
			
			if (oper == '!=') and (condval == varval):
				return False
	
	if (ctype == 'cnf'):
		return False
	
	return True

def parse_config(fname, ask_names):
	"Parse configuration file"
	
	inf = file(fname, 'r')
	
	name = ''
	choices = []
	
	for line in inf:
		
		if (line.startswith('!')):
			# Ask a question
			res = re.search(r'!\s*(?:\[(.*?)\])?\s*([^\s]+)\s*\((.*)\)\s*$', line)
			
			if (not res):
				raise RuntimeError("Weird line: %s" % line)
			
			cond = res.group(1)
			varname = res.group(2)
			vartype = res.group(3)
			
			ask_names.append((varname, vartype, name, choices, cond))
			name = ''
			choices = []
			continue
		
		if (line.startswith('@')):
			# Add new line into the 'choices' array
			res = re.match(r'@\s*(?:\[(.*?)\])?\s*"(.*?)"\s*(.*)$', line)
			
			if not res:
				raise RuntimeError("Bad line: %s" % line)
			
			choices.append((res.group(2), res.group(3)))
			continue
		
		if (line.startswith('%')):
			# Name of the option
			name = line[1:].strip()
			continue
		
		if ((line.startswith('#')) or (line == '\n')):
			# Comment or empty line
			continue
		
		
		raise RuntimeError("Unknown syntax: %s" % line)
	
	inf.close()

def yes_no(default):
	"Return '*' if yes, ' ' if no"
	
	if (default == 'y'):
		return '*'
	
	return ' '

def subchoice(screen, name, choices):
	"Return choice of choices"
	
	maxlen = 0
	for choice in choices:
		length = len(choice[0])
		if (length > maxlen):
			maxlen = length
	
	options = []
	for choice in choices:
		options.append(" %-*s  %s " % (maxlen, choice[0], choice[1]))
	
	retval = snack.ListboxChoiceWindow(screen, name, 'Choose value', options)
	
	if (retval[0] == 'cancel'):
		return None
	
	return choices[retval[1]][0]

def check_choices(defaults, ask_names):
	"Check whether all accessible variables have a default"
	
	for row in ask_names:
		varname = row[0]
		cond = row[4]
		
		if ((cond) and (not check_condition(cond, defaults, ask_names))):
			continue
		
		if (not defaults.has_key(varname)):
			return False
	
	return True

def create_output(fname, defaults, ask_names):
	"Create output configuration"
	
	outf = file(fname, 'w')
	
	outf.write('#########################################\n')
	outf.write('## AUTO-GENERATED FILE, DO NOT EDIT!!! ##\n')
	outf.write('#########################################\n\n')
	
	for row in ask_names:
		varname = row[0]
		name = row[2]
		cond = row[4]
		
		if ((cond) and (not check_condition(cond, defaults, ask_names))):
			continue
		
		if (not defaults.has_key(varname)):
			default = ''
		else:
			default = defaults[varname]
		
		outf.write('# %s\n%s = %s\n\n' % (name, varname, default))
	
	outf.write('REVISION = %s\n' % commands.getoutput('svnversion . 2> /dev/null'))
	outf.write('TIMESTAMP = %s\n' % commands.getoutput('date "+%Y-%m-%d %H:%M:%S"'))
	outf.close()

def main():
	defaults = {}
	ask_names = []
	
	# Parse configuration file
	parse_config(INPUT, ask_names)
	
	# Read defaults from previous run
	if os.path.exists(OUTPUT):
		read_defaults(OUTPUT, defaults)
	
	# Default mode: only check defaults and regenerate configuration
	if ((len(sys.argv) >= 3) and (sys.argv[2] == 'default')):
		if (check_choices(defaults, ask_names)):
			create_output(OUTPUT, defaults, ask_names)
			return 0
	
	screen = snack.SnackScreen()
	try:
		selname = None
		while True:
			
			options = []
			opt2row = {}
			position = None
			cnt = 0
			for row in ask_names:
				
				varname = row[0]
				vartype = row[1]
				name = row[2]
				choices = row[3]
				cond = row[4]
				
				if ((cond) and (not check_condition(cond, defaults, ask_names))):
					continue
				
				if (varname == selname):
					position = cnt
				
				if (not defaults.has_key(varname)):
					default = None
				else:
					default = defaults[varname]
				
				if (vartype == 'choice'):
					# Check if the default is an acceptable value
					if ((default) and (not default in [choice[0] for choice in choices])):
						default = None
						defaults.pop(varname)
					
					# If there is just one option, use it
					if (len(choices) == 1):
						default = choices[0][0]
						defaults[varname] = default
					
					options.append("     %s [%s] --> " % (name, default))
				elif (vartype == 'y/n'):
					if (default == None):
						default = 'y'
						defaults[varname] = default
					options.append(" <%s> %s " % (yes_no(default), name))
				elif (vartype == 'n/y'):
					if (default == None):
						default = 'n'
						defaults[varname] = default
					options.append(" <%s> %s " % (yes_no(default), name))
				else:
					raise RuntimeError("Unknown variable type: %s" % vartype)
				
				opt2row[cnt] = row
				
				cnt += 1
			
			retval = snack.ListboxChoiceWindow(screen, 'HelenOS configuration', 'Choose configuration option', options, default = position)
			
			if (retval[0] == 'cancel'):
				return 'Configuration canceled'
			
			row = opt2row[retval[1]]
			if (row == None):
				raise RuntimeError("Error selecting value: %s" % retval[1])
			
			selname = row[0]
			seltype = row[1]
			name = row[2]
			choices = row[3]
			
			if (retval[0] == 'ok'):
				if (check_choices(defaults, ask_names)):
					break
				else:
					snack.ButtonChoiceWindow(screen, 'Error', 'Some options have still undefined values.', ['Ok']);
					continue
			
			if (seltype == 'choice'):
				defaults[selname] = subchoice(screen, name, choices)
			elif ((seltype == 'y/n') or (seltype == 'n/y')):
				if (defaults[selname] == 'y'):
					defaults[selname] = 'n'
				else:
					defaults[selname] = 'y'
	finally:
		screen.finish()
	
	create_output(OUTPUT, defaults, ask_names)
	return 0

if __name__ == '__main__':
    exit(main())
