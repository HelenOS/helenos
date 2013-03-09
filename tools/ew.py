#!/usr/bin/env python
#
# Copyright (c) 2013 Jakub Jermar 
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
Emulator wrapper for running HelenOS
"""

import os 
import subprocess 
import autotool

def run_in_console(cmd, title):
	cmdline = 'xterm -T ' + '"' + title + '"' + ' -e ' + cmd
	print(cmdline)
	subprocess.call(cmdline, shell = True);

def pc_options():
	return '-enable-kvm'

def malta_options():
	return '-cpu 4Kc'

def platform_to_qemu_options(platform, machine):
	if platform == 'amd64':
		return 'system-x86_64', pc_options()
	elif platform == 'arm32':
		return 'system-arm', ''
	elif platform == 'ia32':
		return 'system-i386', pc_options()
	elif platform == 'mips32':
		if machine == 'lmalta':
			return 'system-mipsel', malta_options()
		elif machine == 'bmalta':
			return 'system-mips', malta_options()
	elif platform == 'ppc32':
		return 'system-ppc', ''
	elif platform == 'sparc64':
		return 'system-sparc64', ''

def qemu_bd_options():
	if not os.path.exists('hdisk.img'):
		subprocess.call('tools/mkfat.py 1048576 uspace/dist/data hdisk.img', shell = True)
	return ' -hda hdisk.img'

def qemu_nic_ne2k_options():
	return ' -device ne2k_isa,irq=5,vlan=0'

def qemu_nic_e1k_options():
	return ' -device e1000,vlan=0'

def qemu_nic_rtl8139_options():
	return ' -device rtl8139,vlan=0'

def qemu_net_options():
	nic_options = qemu_nic_e1k_options()
	return nic_options + ' -net user -redir udp:8080::8080 -redir udp:8081::8081 -redir tcp:8080::8080 -redir tcp:8081::8081 -redir tcp:2223::2223'

def qemu_usb_options():
	return ''

def qemu_run(platform, machine, console, image, networking, storage, usb):
	suffix, options = platform_to_qemu_options(platform, machine)
	cmd = 'qemu-' + suffix

	cmdline = cmd
	if options != '':
		cmdline += ' ' + options

	if storage:
		cmdline += qemu_bd_options()
	if networking:
		cmdline += qemu_net_options()
	if usb:
		cmdline += qemu_usb_options()
	
	if image == 'image.iso':
		cmdline += ' -boot d -cdrom image.iso'
	elif image == 'image.boot':
		cmdline += ' -kernel image.boot'

	if not console:
		cmdline += ' -nographic'

		title = 'HelenOS/' + platform
		if machine != '':
			title += ' on ' + machine
		run_in_console(cmdline, title)
	else:
		print(cmdline)
		subprocess.call(cmdline, shell = True)
		
def ski_run(platform, machine, console, image, networking, storage, usb):
	run_in_console('ski -i contrib/conf/ski.conf', 'HelenOS/ia64 on ski')

def msim_run(platform, machine, console, image, networking, storage, usb):
	run_in_console('msim -c contrib/conf/msim.conf', 'HelenOS/mips32 on msim')

emulators = {}
emulators['amd64'] = {}
emulators['arm32'] = {}
emulators['ia32'] = {}
emulators['ia64'] = {}
emulators['mips32'] = {}
emulators['ppc32'] = {}
emulators['sparc64'] = {}

emulators['amd64'][''] = qemu_run, True, 'image.iso', True, True, True
emulators['arm32']['integratorcp'] = qemu_run, True, 'image.boot', False, False, False
emulators['ia32'][''] = qemu_run, True, 'image.iso', True, True, True
emulators['ia64']['ski'] = ski_run, False, 'image.boot', False, False, False
emulators['mips32']['msim'] = msim_run, False, 'image.boot', False, False, False
emulators['mips32']['lmalta'] = qemu_run, False, 'image.boot', False, False, False
emulators['mips32']['bmalta'] = qemu_run, False, 'image.boot', False, False, False
emulators['ppc32'][''] = qemu_run, True, 'image.iso', True, True, True
emulators['sparc64']['generic'] = qemu_run, True, 'image.iso', True, True, True

def run():
	config = {}
	autotool.read_config(autotool.CONFIG, config)

	try:
		platform = config['PLATFORM']
	except:
		platform = ''

	try:
		mach = config['MACHINE']
	except:
		mach = ''

	try:
		emu_run, console, image, networking, storage, usb = emulators[platform][mach]
	except:
		print("Cannot start emulation for the chosen configuration.")
		return

	emu_run(platform, mach, console, image, networking, storage, usb)

run()
