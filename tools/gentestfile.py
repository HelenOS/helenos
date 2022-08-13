#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2011 Martin Sucha
#
# SPDX-License-Identifier: BSD-3-Clause
#

"""
Generate a file to be used by app/testread
"""

import struct
import sys

if len(sys.argv) < 2:
	print("Usage: gentestfile.py <count of 64-bit numbers to output>")
	exit()

m = int(sys.argv[1])
i = 0
pow_2_64 = 2 ** 64
st = struct.Struct('<Q')
while i < m:
	sys.stdout.write(st.pack(i))
	i = (i + 1) % pow_2_64

