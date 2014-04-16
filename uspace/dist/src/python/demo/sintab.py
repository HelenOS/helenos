#!/usr/bin/python

# Probably not very Pythonic, but it runs well with both Python2 and Python3

import math
import sys

sys.stdout.write("    ")
for frac_part in range(0,10):
	sys.stdout.write(" %5d" % frac_part)
print("")

for angle_deg in range(0,90):
	sys.stdout.write("%3d " % angle_deg)
	for angle_deg_frac in range(0,10):
		angle = math.radians(angle_deg + angle_deg_frac/10.)
		value = math.sin(angle) * 10000 + 0.5
		if value > 10000:
			sys.stdout.write(" %05d" % (value))
		else:
			sys.stdout.write("  %04d" % (value))
	print("")

