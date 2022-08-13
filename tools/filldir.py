#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2011 Martin Sucha
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""
Fill a directory with N empty directories
"""

import sys
import os
import os.path

if len(sys.argv) < 3:
	print('Usage: filldir <parent-dir> <count>')
	exit(2)

parent = sys.argv[1]
num = int(sys.argv[2])

i = 0
while i < num:
	name = hex(i)[2:].zfill(5)
	path = os.path.join(parent, name)
	os.mkdir(path)
	i += 1
