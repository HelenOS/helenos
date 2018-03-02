#
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
Text User Interface wrapper
"""

import sys
import os

def call_dlg(dlgcmd, *args, **kw):
	"Wrapper for calling 'dialog' program"

	indesc, outdesc = os.pipe()
	pid = os.fork()
	if (not pid):
		os.dup2(outdesc, 2)
		os.close(indesc)

		dlgargs = [dlgcmd]
		for key, val in kw.items():
			dlgargs.append('--' + key)
			dlgargs.append(val)

		dlgargs += args
		os.execlp(dlgcmd, *dlgargs)

	os.close(outdesc)

	try:
		errout = os.fdopen(indesc, 'r')
		data = errout.read()
		errout.close()
		pid, status = os.wait()
	except:
		# Reset terminal
		os.system('reset')
		raise

	if (not os.WIFEXITED(status)):
		# Reset terminal
		os.system('reset')
		raise EOFError

	status = os.WEXITSTATUS(status)
	if (status == 255):
		raise EOFError

	return (status, data)

try:
	import snack
	newt = True
	dialog = False
except ImportError:
	newt = False

	dlgcmd = os.environ.get('DIALOG', 'dialog')
	if (call_dlg(dlgcmd, '--print-maxsize')[0] != 0):
		dialog = False
	else:
		dialog = True

width_extra = 13
height_extra = 11

def width_fix(screen, width):
	"Correct width to screen size"

	if (width + width_extra > screen.width):
		width = screen.width - width_extra

	if (width <= 0):
		width = screen.width

	return width

def height_fix(screen, height):
	"Correct height to screen size"

	if (height + height_extra > screen.height):
		height = screen.height - height_extra

	if (height <= 0):
		height = screen.height

	return height

def screen_init():
	"Initialize the screen"

	if (newt):
		return snack.SnackScreen()

	return None

def screen_done(screen):
	"Cleanup the screen"

	if (newt):
		screen.finish()

def choice_window(screen, title, text, options, position):
	"Create options menu"

	maxopt = 0
	for option in options:
		length = len(option)
		if (length > maxopt):
			maxopt = length

	width = maxopt
	height = len(options)

	if (newt):
		width = width_fix(screen, width + width_extra)
		height = height_fix(screen, height)

		if (height > 3):
			large = True
		else:
			large = False

		buttonbar = snack.ButtonBar(screen, ('Done', 'Cancel'))
		textbox = snack.TextboxReflowed(width, text)
		listbox = snack.Listbox(height, scroll = large, returnExit = 1)

		cnt = 0
		for option in options:
			listbox.append(option, cnt)
			cnt += 1

		if (position != None):
			listbox.setCurrent(position)

		grid = snack.GridForm(screen, title, 1, 3)
		grid.add(textbox, 0, 0)
		grid.add(listbox, 0, 1, padding = (0, 1, 0, 1))
		grid.add(buttonbar, 0, 2, growx = 1)

		retval = grid.runOnce()

		return (buttonbar.buttonPressed(retval), listbox.current())
	elif (dialog):
		if (width < 35):
			width = 35

		args = []
		cnt = 0
		for option in options:
			args.append(str(cnt + 1))
			args.append(option)

			cnt += 1

		kw = {}
		if (position != None):
			kw['default-item'] = str(position + 1)

		status, data = call_dlg(dlgcmd, '--title', title, '--extra-button', '--extra-label', 'Done', '--menu', text, str(height + height_extra), str(width + width_extra), str(cnt), *args, **kw)

		if (status == 1):
			return ('cancel', None)

		try:
			choice = int(data) - 1
		except ValueError:
			return ('cancel', None)

		if (status == 0):
			return (None, choice)

		return ('done', choice)

	sys.stdout.write("\n *** %s *** \n%s\n\n" % (title, text))

	maxcnt = len(str(len(options)))
	cnt = 0
	for option in options:
		sys.stdout.write("%*s. %s\n" % (maxcnt, cnt + 1, option))
		cnt += 1

	sys.stdout.write("\n%*s. Done\n" % (maxcnt, '0'))
	sys.stdout.write("%*s. Quit\n\n" % (maxcnt, 'q'))

	while True:
		if (position != None):
			sys.stdout.write("Selection[%s]: " % str(position + 1))
		else:
			if (cnt > 0):
				sys.stdout.write("Selection[1]: ")
			else:
				sys.stdout.write("Selection[0]: ")
		inp = sys.stdin.readline()

		if (not inp):
			raise EOFError

		if (not inp.strip()):
			if (position != None):
				return (None, position)
			else:
				if (cnt > 0):
					inp = '1'
				else:
					inp = '0'

		if (inp.strip() == 'q'):
			return ('cancel', None)

		try:
			choice = int(inp.strip())
		except ValueError:
			continue

		if (choice == 0):
			return ('done', 0)

		if (choice < 1) or (choice > len(options)):
			continue

		return (None, choice - 1)

def error_dialog(screen, title, msg):
	"Print error dialog"

	width = len(msg)

	if (newt):
		width = width_fix(screen, width)

		buttonbar = snack.ButtonBar(screen, ['Ok'])
		textbox = snack.TextboxReflowed(width, msg)

		grid = snack.GridForm(screen, title, 1, 2)
		grid.add(textbox, 0, 0, padding = (0, 0, 0, 1))
		grid.add(buttonbar, 0, 1, growx = 1)
		grid.runOnce()
	elif (dialog):
		call_dlg(dlgcmd, '--title', title, '--msgbox', msg, '6', str(width + width_extra))
	else:
		sys.stdout.write("\n%s: %s\n" % (title, msg))
