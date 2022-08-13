#
# SPDX-FileCopyrightText: 2010 Martin Decky
#
# SPDX-License-Identifier: BSD-3-Clause
#
"""
Helper routines for parsing jobfiles
"""

def parse_arg(record):
	"Parse jobfile line arguments"

	arg = []
	i = 0
	current = ""
	nil = True
	inside = False

	while (i < len(record)):
		if (inside):
			if (record[i] == "}"):
				inside = False
			else:
				current = "%s%s" % (current, record[i])
		else:
			if (record[i] == "{"):
				nil = False
				inside = True
			elif (record[i] == ","):
				arg.append(current)
				current = ""
				nil = True
			else:
				print("Unexpected '%s'" % record[i])
				return False

		i += 1

	if (not nil):
		arg.append(current)

	return arg
