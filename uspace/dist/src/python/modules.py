#!/usr/bin/python

import sys

for m in sys.builtin_module_names:
	print("Built-in module '%s'." % m)

try:
	import pkgutil
	for (loader, name, ispkg) in pkgutil.iter_modules():
		print("Loadable module '%s'." % name)
except ImportError:
	print("Cannot determine list of loadable modules (pkgutil not found)!")


