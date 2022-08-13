#
# SPDX-FileCopyrightText: 2010-2011 Vojtech Horky
#
# SPDX-License-Identifier: BSD-3-Clause
#

deps = [ 'usbhid', 'usbdev', 'usb' ]
includes += [ include_directories('.') ]
src = files(
	'main.c',
	'usbhid.c',
	'subdrivers.c',
	'generic/hiddev.c',
	'kbd/conv.c',
	'kbd/kbddev.c',
	'kbd/kbdrepeat.c',
	'mouse/mousedev.c',
	'multimedia/multimedia.c',
	'multimedia/keymap.c',
	'blink1/blink1.c',
)
