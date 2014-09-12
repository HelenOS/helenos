#!/usr/bin/env python

from distutils.core import setup

setup(
	name = 'bzreml',
	description = 'Commit email plugin for Bazaar',
	keywords = 'plugin bzr email',
	version = '1.4',
	url = 'http://www.decky.cz/',
	license = 'BSD',
	author = 'Martin Decky',
	author_email = 'martin@decky.cz',
	long_description = """Hooks into Bazaar and sends commit notification emails.""",
	package_dir = {'bzrlib.plugins.eml':'.'},
	packages = ['bzrlib.plugins.eml']
)
