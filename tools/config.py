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
import time
import subprocess
import xtui

INPUT = sys.argv[1]
MAKEFILE = 'Makefile.config'
MACROS = 'config.h'
PRECONF = 'defaults'

def read_defaults(fname, defaults):
	"Read saved values from last configuration run"
	
	inf = open(fname, 'r')
	
	for line in inf:
		res = re.match(r'^(?:#!# )?([^#]\w*)\s*=\s*(.*?)\s*$', line)
		if (res):
			defaults[res.group(1)] = res.group(2)
	
	inf.close()

def check_condition(text, defaults, ask_names):
	"Check that the condition specified on input line is True (only CNF and DNF is supported)"
	
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
	"Check for condition"
	
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
		
		if (not condname in defaults):
			varval = ''
		else:
			varval = defaults[condname]
			if (varval == '*'):
				varval = 'y'
		
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
	
	inf = open(fname, 'r')
	
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

def subchoice(screen, name, choices, default):
	"Return choice of choices"
	
	maxkey = 0
	for key, val in choices:
		length = len(key)
		if (length > maxkey):
			maxkey = length
	
	options = []
	position = None
	cnt = 0
	for key, val in choices:
		if ((default) and (key == default)):
			position = cnt
		
		options.append(" %-*s  %s " % (maxkey, key, val))
		cnt += 1
	
	(button, value) = xtui.choice_window(screen, name, 'Choose value', options, position)
	
	if (button == 'cancel'):
		return None
	
	return choices[value][0]

## Infer and verify configuration values.
#
# Augment @a defaults with values that can be inferred, purge invalid ones
# and verify that all variables have a value (previously specified or inferred).
#
# @param defaults	Configuration to work on
# @param ask_names	Rules
#
# @return		True if configuration is complete and valid, False
#			otherwise.
#
def infer_verify_choices(defaults, ask_names):
	"Infer and verify configuration values."
	
	for varname, vartype, name, choices, cond in ask_names:
		if ((cond) and (not check_condition(cond, defaults, ask_names))):
			continue
		
		if (not varname in defaults):
			default = None
		else:
			default = defaults[varname]

		if not rule_value_is_valid((varname, vartype, name, choices, cond), default):
			default = None

		rdef = rule_get_default((varname, vartype, name, choices, cond))
		if rdef != None:
			default = rdef
			defaults[varname] = rdef

		if (not varname in defaults):
			return False
	
	return True

## Get default value from a rule.
def rule_get_default(rule):
	varname, vartype, name, choices, cond = rule

	default = None

	if (vartype == 'choice'):
		# If there is just one option, use it
		if (len(choices) == 1):
			default = choices[0][0]
	elif (vartype == 'y'):
		default = '*'
	elif (vartype == 'n'):
		default = 'n'
	elif (vartype == 'y/n'):
		default = 'y'
	elif (vartype == 'n/y'):
		default = 'n'
	else:
		raise RuntimeError("Unknown variable type: %s" % vartype)

	return default

## Get option from a rule.
#
# @param rule	Rule for a variable
# @param value	Current value of the variable
#
# @return Option (string) to ask or None which means not to ask.
#
def rule_get_option(rule, value):
	varname, vartype, name, choices, cond = rule

	option = None

	if (vartype == 'choice'):
		# If there is just one option, don't ask
		if (len(choices) != 1):
			if (value == None):
				option = "?     %s --> " % name
			else:
				option = "      %s [%s] --> " % (name, value)
	elif (vartype == 'y'):
		pass
	elif (vartype == 'n'):
		pass
	elif (vartype == 'y/n'):
		option = "  <%s> %s " % (yes_no(value), name)
	elif (vartype == 'n/y'):
		option ="  <%s> %s " % (yes_no(value), name)
	else:
		raise RuntimeError("Unknown variable type: %s" % vartype)

	return option

## Check if variable value is valid.
#
# @param rule	Rule for the variable
# @param value	Value of the variable
#
# @return	True if valid, False if not valid.
#
def rule_value_is_valid(rule, value):
	varname, vartype, name, choices, cond = rule
	
	if value == None:
		return True

	if (vartype == 'choice'):
		if (not value in [choice[0] for choice in choices]):
			return False
	elif (vartype == 'y'):
		if value != 'y':
			return False
	elif (vartype == 'n'):
		if value != 'n':
			return False
	elif (vartype == 'y/n'):
		if not value in ['y', 'n']:
			return False
	elif (vartype == 'n/y'):
		if not value in ['y', 'n']:
			return False
	else:
		raise RuntimeError("Unknown variable type: %s" % vartype)

	return True

def create_output(mkname, mcname, defaults, ask_names):
	"Create output configuration"
	
	timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
	
	sys.stderr.write("Fetching current revision identifier ... ")
	
	try:
		version = subprocess.Popen(['bzr', 'version-info', '--custom', '--template={clean}:{revno}:{revision_id}'], stdout = subprocess.PIPE).communicate()[0].decode().split(':')
		sys.stderr.write("ok\n")
	except:
		version = [1, "unknown", "unknown"]
		sys.stderr.write("failed\n")
	
	if (len(version) == 3):
		revision = version[1]
		if (version[0] != 1):
			revision += 'M'
		revision += ' (%s)' % version[2]
	else:
		revision = None
	
	outmk = open(mkname, 'w')
	outmc = open(mcname, 'w')
	
	outmk.write('#########################################\n')
	outmk.write('## AUTO-GENERATED FILE, DO NOT EDIT!!! ##\n')
	outmk.write('#########################################\n\n')
	
	outmc.write('/***************************************\n')
	outmc.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	outmc.write(' ***************************************/\n\n')
	
	defs = 'CONFIG_DEFS ='
	
	for varname, vartype, name, choices, cond in ask_names:
		if ((cond) and (not check_condition(cond, defaults, ask_names))):
			continue
		
		if (not varname in defaults):
			default = ''
		else:
			default = defaults[varname]
			if (default == '*'):
				default = 'y'
		
		outmk.write('# %s\n%s = %s\n\n' % (name, varname, default))
		
		if ((vartype == "y") or (vartype == "n") or (vartype == "y/n") or (vartype == "n/y")):
			if (default == "y"):
				outmc.write('/* %s */\n#define %s\n\n' % (name, varname))
				defs += ' -D%s' % varname
		else:
			outmc.write('/* %s */\n#define %s %s\n#define %s_%s\n\n' % (name, varname, default, varname, default))
			defs += ' -D%s=%s -D%s_%s' % (varname, default, varname, default)
	
	if (revision is not None):
		outmk.write('REVISION = %s\n' % revision)
		outmc.write('#define REVISION %s\n' % revision)
		defs += ' "-DREVISION=%s"' % revision
	
	outmk.write('TIMESTAMP = %s\n' % timestamp)
	outmc.write('#define TIMESTAMP %s\n' % timestamp)
	defs += ' "-DTIMESTAMP=%s"\n' % timestamp
	
	outmk.write(defs)
	
	outmk.close()
	outmc.close()

def sorted_dir(root):
	list = os.listdir(root)
	list.sort()
	return list

def read_preconfigured(root, fname, screen, defaults):
	options = []
	opt2path = {}
	cnt = 0
	
	# Look for profiles
	for name in sorted_dir(root):
		path = os.path.join(root, name)
		canon = os.path.join(path, fname)
		
		if ((os.path.isdir(path)) and (os.path.exists(canon)) and (os.path.isfile(canon))):
			subprofile = False
			
			# Look for subprofiles
			for subname in sorted_dir(path):
				subpath = os.path.join(path, subname)
				subcanon = os.path.join(subpath, fname)
				
				if ((os.path.isdir(subpath)) and (os.path.exists(subcanon)) and (os.path.isfile(subcanon))):
					subprofile = True
					options.append("%s (%s)" % (name, subname))
					opt2path[cnt] = (canon, subcanon)
					cnt += 1
			
			if (not subprofile):
				options.append(name)
				opt2path[cnt] = (canon, None)
				cnt += 1
	
	(button, value) = xtui.choice_window(screen, 'Load preconfigured defaults', 'Choose configuration profile', options, None)
	
	if (button == 'cancel'):
		return None
	
	read_defaults(opt2path[value][0], defaults)
	if (opt2path[value][1] != None):
		read_defaults(opt2path[value][1], defaults)

def main():
	defaults = {}
	ask_names = []
	
	# Parse configuration file
	parse_config(INPUT, ask_names)
	
	# Read defaults from previous run
	if os.path.exists(MAKEFILE):
		read_defaults(MAKEFILE, defaults)
	
	# Default mode: only check defaults and regenerate configuration
	if ((len(sys.argv) >= 3) and (sys.argv[2] == 'default')):
		if (infer_verify_choices(defaults, ask_names)):
			create_output(MAKEFILE, MACROS, defaults, ask_names)
			return 0
	
	# Check mode: only check defaults
	if ((len(sys.argv) >= 3) and (sys.argv[2] == 'check')):
		if (infer_verify_choices(defaults, ask_names)):
			return 0
		return 1
	
	screen = xtui.screen_init()
	try:
		selname = None
		position = None
		while True:
			
			# Cancel out all defaults which have to be deduced
			for varname, vartype, name, choices, cond in ask_names:
				if ((vartype == 'y') and (varname in defaults) and (defaults[varname] == '*')):
					defaults[varname] = None
			
			options = []
			opt2row = {}
			cnt = 1
			
			options.append("  --- Load preconfigured defaults ... ")
			
			for rule in ask_names:
				varname, vartype, name, choices, cond = rule
				
				if ((cond) and (not check_condition(cond, defaults, ask_names))):
					continue
				
				if (varname == selname):
					position = cnt
				
				if (not varname in defaults):
					default = None
				else:
					default = defaults[varname]
				
				if not rule_value_is_valid(rule, default):
					default = None

				rdef = rule_get_default(rule)
				if rdef != None:
					default = rdef
					defaults[varname] = rdef

				option = rule_get_option(rule, default)
				if option != None:
					options.append(option)
				
				opt2row[cnt] = (varname, vartype, name, choices)
				
				cnt += 1
			
			if (position != None) and (position >= len(options)):
				position = None
			
			(button, value) = xtui.choice_window(screen, 'HelenOS configuration', 'Choose configuration option', options, position)
			
			if (button == 'cancel'):
				return 'Configuration canceled'
			
			if (button == 'done'):
				if (infer_verify_choices(defaults, ask_names)):
					break
				else:
					xtui.error_dialog(screen, 'Error', 'Some options have still undefined values. These options are marked with the "?" sign.')
					continue
			
			if (value == 0):
				read_preconfigured(PRECONF, MAKEFILE, screen, defaults)
				position = 1
				continue
			
			position = None
			if (not value in opt2row):
				raise RuntimeError("Error selecting value: %s" % value)
			
			(selname, seltype, name, choices) = opt2row[value]
			
			if (not selname in defaults):
				default = None
			else:
				default = defaults[selname]
			
			if (seltype == 'choice'):
				defaults[selname] = subchoice(screen, name, choices, default)
			elif ((seltype == 'y/n') or (seltype == 'n/y')):
				if (defaults[selname] == 'y'):
					defaults[selname] = 'n'
				else:
					defaults[selname] = 'y'
	finally:
		xtui.screen_done(screen)
	
	create_output(MAKEFILE, MACROS, defaults, ask_names)
	return 0

if __name__ == '__main__':
	sys.exit(main())
