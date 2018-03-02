#!/usr/bin/env python
#
# Copyright (c) 2006 Ondrej Palkovsky
# Copyright (c) 2009 Martin Decky
# Copyright (c) 2010 Jiri Svoboda
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
import random

RULES_FILE = sys.argv[1]
MAKEFILE = 'Makefile.config'
MACROS = 'config.h'
PRESETS_DIR = 'defaults'

def read_config(fname, config):
	"Read saved values from last configuration run or a preset file"

	inf = open(fname, 'r')

	for line in inf:
		res = re.match(r'^(?:#!# )?([^#]\w*)\s*=\s*(.*?)\s*$', line)
		if res:
			config[res.group(1)] = res.group(2)

	inf.close()

def check_condition(text, config, rules):
	"Check that the condition specified on input line is True (only CNF and DNF is supported)"

	ctype = 'cnf'

	if (')|' in text) or ('|(' in text):
		ctype = 'dnf'

	if ctype == 'cnf':
		conds = text.split('&')
	else:
		conds = text.split('|')

	for cond in conds:
		if cond.startswith('(') and cond.endswith(')'):
			cond = cond[1:-1]

		inside = check_inside(cond, config, ctype)

		if (ctype == 'cnf') and (not inside):
			return False

		if (ctype == 'dnf') and inside:
			return True

	if ctype == 'cnf':
		return True

	return False

def check_inside(text, config, ctype):
	"Check for condition"

	if ctype == 'cnf':
		conds = text.split('|')
	else:
		conds = text.split('&')

	for cond in conds:
		res = re.match(r'^(.*?)(!?=)(.*)$', cond)
		if not res:
			raise RuntimeError("Invalid condition: %s" % cond)

		condname = res.group(1)
		oper = res.group(2)
		condval = res.group(3)

		if not condname in config:
			varval = ''
		else:
			varval = config[condname]
			if (varval == '*'):
				varval = 'y'

		if ctype == 'cnf':
			if (oper == '=') and (condval == varval):
				return True

			if (oper == '!=') and (condval != varval):
				return True
		else:
			if (oper == '=') and (condval != varval):
				return False

			if (oper == '!=') and (condval == varval):
				return False

	if ctype == 'cnf':
		return False

	return True

def parse_rules(fname, rules):
	"Parse rules file"

	inf = open(fname, 'r')

	name = ''
	choices = []

	for line in inf:

		if line.startswith('!'):
			# Ask a question
			res = re.search(r'!\s*(?:\[(.*?)\])?\s*([^\s]+)\s*\((.*)\)\s*$', line)

			if not res:
				raise RuntimeError("Weird line: %s" % line)

			cond = res.group(1)
			varname = res.group(2)
			vartype = res.group(3)

			rules.append((varname, vartype, name, choices, cond))
			name = ''
			choices = []
			continue

		if line.startswith('@'):
			# Add new line into the 'choices' array
			res = re.match(r'@\s*(?:\[(.*?)\])?\s*"(.*?)"\s*(.*)$', line)

			if not res:
				raise RuntimeError("Bad line: %s" % line)

			choices.append((res.group(2), res.group(3)))
			continue

		if line.startswith('%'):
			# Name of the option
			name = line[1:].strip()
			continue

		if line.startswith('#') or (line == '\n'):
			# Comment or empty line
			continue


		raise RuntimeError("Unknown syntax: %s" % line)

	inf.close()

def yes_no(default):
	"Return '*' if yes, ' ' if no"

	if default == 'y':
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
		if (default) and (key == default):
			position = cnt

		options.append(" %-*s  %s " % (maxkey, key, val))
		cnt += 1

	(button, value) = xtui.choice_window(screen, name, 'Choose value', options, position)

	if button == 'cancel':
		return None

	return choices[value][0]

## Infer and verify configuration values.
#
# Augment @a config with values that can be inferred, purge invalid ones
# and verify that all variables have a value (previously specified or inferred).
#
# @param config Configuration to work on
# @param rules  Rules
#
# @return True if configuration is complete and valid, False
#         otherwise.
#
def infer_verify_choices(config, rules):
	"Infer and verify configuration values."

	for rule in rules:
		varname, vartype, name, choices, cond = rule

		if cond and (not check_condition(cond, config, rules)):
			continue

		if not varname in config:
			value = None
		else:
			value = config[varname]

		if not validate_rule_value(rule, value):
			value = None

		default = get_default_rule(rule)

		#
		# If we don't have a value but we do have
		# a default, use it.
		#
		if value == None and default != None:
			value = default
			config[varname] = default

		if not varname in config:
			return False

	return True

## Fill the configuration with random (but valid) values.
#
# The random selection takes next rule and if the condition does
# not violate existing configuration, random value of the variable
# is selected.
# This happens recursively as long as there are more rules.
# If a conflict is found, we backtrack and try other settings of the
# variable or ignoring the variable altogether.
#
# @param config Configuration to work on
# @param rules  Rules
# @param start_index With which rule to start (initial call must specify 0 here).
# @return True if able to find a valid configuration
def random_choices(config, rules, start_index):
	"Fill the configuration with random (but valid) values."
	if start_index >= len(rules):
		return True

	varname, vartype, name, choices, cond = rules[start_index]

	# First check that this rule would make sense
	if cond:
		if not check_condition(cond, config, rules):
			return random_choices(config, rules, start_index + 1)

	# Remember previous choices for backtracking
	yes_no = 0
	choices_indexes = range(0, len(choices))
	random.shuffle(choices_indexes)

	# Remember current configuration value
	old_value = None
	try:
		old_value = config[varname]
	except KeyError:
		old_value = None

	# For yes/no choices, we ran the loop at most 2 times, for select
	# choices as many times as there are options.
	try_counter = 0
	while True:
		if vartype == 'choice':
			if try_counter >= len(choices_indexes):
				break
			value = choices[choices_indexes[try_counter]][0]
		elif vartype == 'y' or vartype == 'n':
			if try_counter > 0:
				break
			value = vartype
		elif vartype == 'y/n' or vartype == 'n/y':
			if try_counter == 0:
				yes_no = random.randint(0, 1)
			elif try_counter == 1:
				yes_no = 1 - yes_no
			else:
				break
			if yes_no == 0:
				value = 'n'
			else:
				value = 'y'
		else:
			raise RuntimeError("Unknown variable type: %s" % vartype)

		config[varname] = value

		ok = random_choices(config, rules, start_index + 1)
		if ok:
			return True

		try_counter = try_counter + 1

	# Restore the old value and backtrack
	# (need to delete to prevent "ghost" variables that do not exist under
	# certain configurations)
	config[varname] = old_value
	if old_value is None:
		del config[varname]

	return random_choices(config, rules, start_index + 1)


## Get default value from a rule.
def get_default_rule(rule):
	varname, vartype, name, choices, cond = rule

	default = None

	if vartype == 'choice':
		# If there is just one option, use it
		if len(choices) == 1:
			default = choices[0][0]
	elif vartype == 'y':
		default = '*'
	elif vartype == 'n':
		default = 'n'
	elif vartype == 'y/n':
		default = 'y'
	elif vartype == 'n/y':
		default = 'n'
	else:
		raise RuntimeError("Unknown variable type: %s" % vartype)

	return default

## Get option from a rule.
#
# @param rule  Rule for a variable
# @param value Current value of the variable
#
# @return Option (string) to ask or None which means not to ask.
#
def get_rule_option(rule, value):
	varname, vartype, name, choices, cond = rule

	option = None

	if vartype == 'choice':
		# If there is just one option, don't ask
		if len(choices) != 1:
			if (value == None):
				option = "?     %s --> " % name
			else:
				option = "      %s [%s] --> " % (name, value)
	elif vartype == 'y':
		pass
	elif vartype == 'n':
		pass
	elif vartype == 'y/n':
		option = "  <%s> %s " % (yes_no(value), name)
	elif vartype == 'n/y':
		option ="  <%s> %s " % (yes_no(value), name)
	else:
		raise RuntimeError("Unknown variable type: %s" % vartype)

	return option

## Check if variable value is valid.
#
# @param rule  Rule for the variable
# @param value Value of the variable
#
# @return True if valid, False if not valid.
#
def validate_rule_value(rule, value):
	varname, vartype, name, choices, cond = rule

	if value == None:
		return True

	if vartype == 'choice':
		if not value in [choice[0] for choice in choices]:
			return False
	elif vartype == 'y':
		if value != 'y':
			return False
	elif vartype == 'n':
		if value != 'n':
			return False
	elif vartype == 'y/n':
		if not value in ['y', 'n']:
			return False
	elif vartype == 'n/y':
		if not value in ['y', 'n']:
			return False
	else:
		raise RuntimeError("Unknown variable type: %s" % vartype)

	return True

def preprocess_config(config, rules):
	"Preprocess configuration"

	varname_mode = 'CONFIG_BFB_MODE'
	varname_width = 'CONFIG_BFB_WIDTH'
	varname_height = 'CONFIG_BFB_HEIGHT'

	if varname_mode in config:
		mode = config[varname_mode].partition('x')

		config[varname_width] = mode[0]
		rules.append((varname_width, 'choice', 'Default framebuffer width', None, None))

		config[varname_height] = mode[2]
		rules.append((varname_height, 'choice', 'Default framebuffer height', None, None))

def create_output(mkname, mcname, config, rules):
	"Create output configuration"

	varname_strip = 'CONFIG_STRIP_REVISION_INFO'
	strip_rev_info = (varname_strip in config) and (config[varname_strip] == 'y')

	if strip_rev_info:
		timestamp_unix = int(0)
	else:
		# TODO: Use commit timestamp instead of build time.
		timestamp_unix = int(time.time())

	timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(timestamp_unix))

	sys.stderr.write("Fetching current revision identifier ... ")

	try:
		version = subprocess.Popen(['git', 'log', '-1', '--pretty=%h'], stdout = subprocess.PIPE).communicate()[0].decode().strip()
		sys.stderr.write("ok\n")
	except:
		version = None
		sys.stderr.write("failed\n")

	if (not strip_rev_info) and (version is not None):
		revision = version
	else:
		revision = None

	outmk = open(mkname, 'w')
	outmc = open(mcname, 'w')

	outmk.write('#########################################\n')
	outmk.write('## AUTO-GENERATED FILE, DO NOT EDIT!!! ##\n')
	outmk.write('## Generated by: tools/config.py       ##\n')
	outmk.write('#########################################\n\n')

	outmc.write('/***************************************\n')
	outmc.write(' * AUTO-GENERATED FILE, DO NOT EDIT!!! *\n')
	outmc.write(' * Generated by: tools/config.py       *\n')
	outmc.write(' ***************************************/\n\n')

	defs = 'CONFIG_DEFS ='

	for varname, vartype, name, choices, cond in rules:
		if cond and (not check_condition(cond, config, rules)):
			continue

		if not varname in config:
			value = ''
		else:
			value = config[varname]
			if (value == '*'):
				value = 'y'

		outmk.write('# %s\n%s = %s\n\n' % (name, varname, value))

		if vartype in ["y", "n", "y/n", "n/y"]:
			if value == "y":
				outmc.write('/* %s */\n#define %s\n\n' % (name, varname))
				defs += ' -D%s' % varname
		else:
			outmc.write('/* %s */\n#define %s %s\n#define %s_%s\n\n' % (name, varname, value, varname, value))
			defs += ' -D%s=%s -D%s_%s' % (varname, value, varname, value)

	if revision is not None:
		outmk.write('REVISION = %s\n' % revision)
		outmc.write('#define REVISION %s\n' % revision)
		defs += ' "-DREVISION=%s"' % revision

	outmk.write('TIMESTAMP_UNIX = %d\n' % timestamp_unix)
	outmc.write('#define TIMESTAMP_UNIX %d\n' % timestamp_unix)
	defs += ' "-DTIMESTAMP_UNIX=%d"\n' % timestamp_unix

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

## Ask user to choose a configuration profile.
#
def choose_profile(root, fname, screen, config):
	options = []
	opt2path = {}
	cnt = 0

	# Look for profiles
	for name in sorted_dir(root):
		path = os.path.join(root, name)
		canon = os.path.join(path, fname)

		if os.path.isdir(path) and os.path.exists(canon) and os.path.isfile(canon):
			subprofile = False

			# Look for subprofiles
			for subname in sorted_dir(path):
				subpath = os.path.join(path, subname)
				subcanon = os.path.join(subpath, fname)

				if os.path.isdir(subpath) and os.path.exists(subcanon) and os.path.isfile(subcanon):
					subprofile = True
					options.append("%s (%s)" % (name, subname))
					opt2path[cnt] = [name, subname]
					cnt += 1

			if not subprofile:
				options.append(name)
				opt2path[cnt] = [name]
				cnt += 1

	(button, value) = xtui.choice_window(screen, 'Load preconfigured defaults', 'Choose configuration profile', options, None)

	if button == 'cancel':
		return None

	return opt2path[value]

## Read presets from a configuration profile.
#
# @param profile Profile to load from (a list of string components)
# @param config  Output configuration
#
def read_presets(profile, config):
	path = os.path.join(PRESETS_DIR, profile[0], MAKEFILE)
	read_config(path, config)

	if len(profile) > 1:
		path = os.path.join(PRESETS_DIR, profile[0], profile[1], MAKEFILE)
		read_config(path, config)

## Parse profile name (relative OS path) into a list of components.
#
# @param profile_name Relative path (using OS separator)
# @return             List of components
#
def parse_profile_name(profile_name):
	profile = []

	head, tail = os.path.split(profile_name)
	if head != '':
		profile.append(head)

	profile.append(tail)
	return profile

def main():
	profile = None
	config = {}
	rules = []

	# Parse rules file
	parse_rules(RULES_FILE, rules)

	# Input configuration file can be specified on command line
	# otherwise configuration from previous run is used.
	if len(sys.argv) >= 4:
		profile = parse_profile_name(sys.argv[3])
		read_presets(profile, config)
	elif os.path.exists(MAKEFILE):
		read_config(MAKEFILE, config)

	# Default mode: check values and regenerate configuration files
	if (len(sys.argv) >= 3) and (sys.argv[2] == 'default'):
		if (infer_verify_choices(config, rules)):
			preprocess_config(config, rules)
			create_output(MAKEFILE, MACROS, config, rules)
			return 0

	# Hands-off mode: check values and regenerate configuration files,
	# but no interactive fallback
	if (len(sys.argv) >= 3) and (sys.argv[2] == 'hands-off'):
		# We deliberately test sys.argv >= 4 because we do not want
		# to read implicitly any possible previous run configuration
		if len(sys.argv) < 4:
			sys.stderr.write("Configuration error: No presets specified\n")
			return 2

		if (infer_verify_choices(config, rules)):
			preprocess_config(config, rules)
			create_output(MAKEFILE, MACROS, config, rules)
			return 0

		sys.stderr.write("Configuration error: The presets are ambiguous\n")
		return 1

	# Check mode: only check configuration
	if (len(sys.argv) >= 3) and (sys.argv[2] == 'check'):
		if infer_verify_choices(config, rules):
			return 0
		return 1

	# Random mode
	if (len(sys.argv) == 3) and (sys.argv[2] == 'random'):
		ok = random_choices(config, rules, 0)
		if not ok:
			sys.stderr.write("Internal error: unable to generate random config.\n")
			return 2
		if not infer_verify_choices(config, rules):
			sys.stderr.write("Internal error: random configuration not consistent.\n")
			return 2
		preprocess_config(config, rules)
		create_output(MAKEFILE, MACROS, config, rules)

		return 0

	screen = xtui.screen_init()
	try:
		selname = None
		position = None
		while True:

			# Cancel out all values which have to be deduced
			for varname, vartype, name, choices, cond in rules:
				if (vartype == 'y') and (varname in config) and (config[varname] == '*'):
					config[varname] = None

			options = []
			opt2row = {}
			cnt = 1

			options.append("  --- Load preconfigured defaults ... ")

			for rule in rules:
				varname, vartype, name, choices, cond = rule

				if cond and (not check_condition(cond, config, rules)):
					continue

				if varname == selname:
					position = cnt

				if not varname in config:
					value = None
				else:
					value = config[varname]

				if not validate_rule_value(rule, value):
					value = None

				default = get_default_rule(rule)

				#
				# If we don't have a value but we do have
				# a default, use it.
				#
				if value == None and default != None:
					value = default
					config[varname] = default

				option = get_rule_option(rule, value)
				if option != None:
					options.append(option)
				else:
					continue

				opt2row[cnt] = (varname, vartype, name, choices)

				cnt += 1

			if (position != None) and (position >= len(options)):
				position = None

			(button, value) = xtui.choice_window(screen, 'HelenOS configuration', 'Choose configuration option', options, position)

			if button == 'cancel':
				return 'Configuration canceled'

			if button == 'done':
				if (infer_verify_choices(config, rules)):
					break
				else:
					xtui.error_dialog(screen, 'Error', 'Some options have still undefined values. These options are marked with the "?" sign.')
					continue

			if value == 0:
				profile = choose_profile(PRESETS_DIR, MAKEFILE, screen, config)
				if profile != None:
					read_presets(profile, config)
				position = 1
				continue

			position = None
			if not value in opt2row:
				raise RuntimeError("Error selecting value: %s" % value)

			(selname, seltype, name, choices) = opt2row[value]

			if not selname in config:
				value = None
			else:
				value = config[selname]

			if seltype == 'choice':
				config[selname] = subchoice(screen, name, choices, value)
			elif (seltype == 'y/n') or (seltype == 'n/y'):
				if config[selname] == 'y':
					config[selname] = 'n'
				else:
					config[selname] = 'y'
	finally:
		xtui.screen_done(screen)

	preprocess_config(config, rules)
	create_output(MAKEFILE, MACROS, config, rules)
	return 0

if __name__ == '__main__':
	sys.exit(main())
