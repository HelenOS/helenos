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
import sys
import subprocess
import autotool
import platform
import thread
import time

overrides = {}

def is_override(str):
	if str in overrides.keys():
		return overrides[str]
	return False

def cfg_get(platform, machine, processor):
	if machine == "" or emulators[platform].has_key("run"):
		return emulators[platform]
	elif processor == "" or emulators[platform][machine].has_key("run"):
		return emulators[platform][machine]
	else:
		return emulators[platform][machine][processor]

def termemu_detect():
	for termemu in ['xfce4-terminal', 'xterm']:
		try:
			subprocess.check_output('which ' + termemu, shell = True)
			return termemu
		except:
			pass

def run_in_console(cmd, title):
	ecmd = cmd.replace('"', '\\"')
	cmdline = termemu_detect() + ' -T ' + '"' + title + '"' + ' -e "' + ecmd + '"'
	print(cmdline)
	if not is_override('dryrun'):
		subprocess.call(cmdline, shell = True)

def get_host_native_width():
	return int(platform.architecture()[0].strip('bit'))

def pc_options(guest_width):
	opts = ''

	# Do not enable KVM if running 64 bits HelenOS
	# on 32 bits host
	host_width = get_host_native_width()
	if guest_width <= host_width and not is_override('nokvm'):
		opts = opts + ' -enable-kvm'

	# Remove the leading space
	return opts[1:]

def malta_options():
	return '-cpu 4Kc'

def platform_to_qemu_options(platform, machine, processor):
	if platform == 'amd64':
		return 'system-x86_64', pc_options(64)
	elif platform == 'arm32':
		return 'system-arm', '-M integratorcp'
	elif platform == 'ia32':
		return 'system-i386', pc_options(32)
	elif platform == 'mips32':
		if machine == 'lmalta':
			return 'system-mipsel', malta_options()
		elif machine == 'bmalta':
			return 'system-mips', malta_options()
	elif platform == 'ppc32':
		return 'system-ppc', '-m 256'
	elif platform == 'sparc64':
		if machine != 'generic':
			raise Exception
		if processor == 'us':
			return 'system-sparc64', '-M sun4u --prom-env boot-args="console=devices/\\hw\\pci0\\01:01.0\\com1\\a"'
		elif processor == 'sun4v':
			default_path = '/usr/local/opensparc/image/'
		try:
			if os.path.exists(default_path):
				opensparc_bins = default_path
			elif os.path.exists(os.environ['OPENSPARC_BINARIES']):
				opensparc_bins = os.environ['OPENSPARC_BINARIES']
			else:
				raise Exception
		except:
			print("Cannot find OpenSPARC binary images!")
			print("Either set OPENSPARC_BINARIES environment variable accordingly or place the images in %s." % (default_path))
			raise Exception

		return 'system-sparc64', '-M niagara -m 256 -L %s' % (opensparc_bins)


def hdisk_mk():
	if not os.path.exists('hdisk.img'):
		subprocess.call('tools/mkfat.py 1048576 uspace/dist/data hdisk.img', shell = True)

def qemu_bd_options():
	if is_override('nohdd'):
		return ''

	hdisk_mk()

	return ' -drive file=hdisk.img,index=0,media=disk,format=raw'

def qemu_nic_ne2k_options():
	return ' -device ne2k_isa,irq=5,vlan=0'

def qemu_nic_e1k_options():
	return ' -device e1000,vlan=0'

def qemu_nic_rtl8139_options():
	return ' -device rtl8139,vlan=0'

def qemu_net_options():
	if is_override('nonet'):
		return ''

	nic_options = ''
	if 'net' in overrides.keys():
		if 'e1k' in overrides['net'].keys():
			nic_options += qemu_nic_e1k_options()
		if 'rtl8139' in overrides['net'].keys():
			nic_options += qemu_nic_rtl8139_options()
		if 'ne2k' in overrides['net'].keys():
			nic_options += qemu_nic_ne2k_options()
	else:
		# Use the default NIC
		nic_options += qemu_nic_e1k_options()

	return nic_options + ' -net user,hostfwd=udp::8080-:8080,hostfwd=udp::8081-:8081,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081,hostfwd=tcp::2223-:2223'

def qemu_usb_options():
	if is_override('nousb'):
		return ''
	return ' -usb'

def qemu_xhci_options():
	if is_override('noxhci'):
		return ''
	return ' -device nec-usb-xhci,id=xhci'

def qemu_tablet_options():
	if is_override('notablet') or (is_override('nousb') and is_override('noxhci')):
		return ''
	return ' -device usb-tablet'

def qemu_audio_options():
	if is_override('nosnd'):
		return ''
	return ' -device intel-hda -device hda-duplex'

def qemu_run(platform, machine, processor):
	cfg = cfg_get(platform, machine, processor)
	suffix, options = platform_to_qemu_options(platform, machine, processor)
	cmd = 'qemu-' + suffix

	cmdline = cmd
	if 'qemu_path' in overrides.keys():
		cmdline = overrides['qemu_path'] + cmd

	if options != '':
		cmdline += ' ' + options

	cmdline += qemu_bd_options()

	if (not 'net' in cfg.keys()) or cfg['net']:
		cmdline += qemu_net_options()
	if (not 'usb' in cfg.keys()) or cfg['usb']:
		cmdline += qemu_usb_options()
	if (not 'xhci' in cfg.keys()) or cfg['xhci']:
		cmdline += qemu_xhci_options()
	if (not 'tablet' in cfg.keys()) or cfg['tablet']:
		cmdline += qemu_tablet_options()
	if (not 'audio' in cfg.keys()) or cfg['audio']:
		cmdline += qemu_audio_options()

	if cfg['image'] == 'image.iso':
		cmdline += ' -boot d -cdrom image.iso'
	elif cfg['image'] == 'image.boot':
		cmdline += ' -kernel image.boot'
	else:
		cmdline += ' ' + cfg['image']

	if ('console' in cfg.keys()) and not cfg['console']:
		cmdline += ' -nographic'

		title = 'HelenOS/' + platform
		if machine != '':
			title += ' on ' + machine
		if 'expect' in cfg.keys():
			cmdline = 'expect -c \'spawn %s; expect "%s" { send "%s" } timeout exp_continue; interact\'' % (cmdline, cfg['expect']['src'], cfg['expect']['dst'])
		run_in_console(cmdline, title)
	else:
		print(cmdline)
		if not is_override('dryrun'):
			subprocess.call(cmdline, shell = True)

def ski_run(platform, machine, processor):
	run_in_console('ski -i contrib/conf/ski.conf', 'HelenOS/ia64 on ski')

def msim_run(platform, machine, processor):
	hdisk_mk()
	run_in_console('msim -c contrib/conf/msim.conf', 'HelenOS/mips32 on msim')

def spike_run(platform, machine, processor):
	run_in_console('spike -m1073741824:1073741824 image.boot', 'HelenOS/risvc64 on Spike')

emulators = {
	'amd64' : {
		'run' : qemu_run,
		'image' : 'image.iso'
	},
	'arm32' : {
		'integratorcp' : {
			'run' : qemu_run,
			'image' : 'image.boot',
			'net' : False,
			'audio' : False,
			'xhci' : False,
			'tablet' : False
		}
	},
	'ia32' : {
		'run' : qemu_run,
		'image' : 'image.iso'
	},
	'ia64' : {
		'ski' : {
			'run' : ski_run
		}
	},
	'mips32' : {
		'msim' : {
			'run' : msim_run
		},
		'lmalta' : {
			'run' : qemu_run,
			'image' : 'image.boot',
			'console' : False
		},
		'bmalta' : {
			'run' : qemu_run,
			'image' : 'image.boot',
			'console' : False
		},
	},
	'ppc32' : {
		'run' : qemu_run,
		'image' : 'image.iso',
		'audio' : False
	},
	'riscv64' : {
		'run' : spike_run,
		'image' : 'image.boot'
	},
	'sparc64' : {
		'generic' : {
			'us' : {
				'run' : qemu_run,
				'image' : 'image.iso',
				'audio' : False,
				'console' : False,
				'net' : False,
				'usb' : False,
				'xhci' : False,
				'tablet' : False
			},
			'sun4v' : {
				'run' : qemu_run,
				'image' : '-drive if=pflash,readonly=on,file=image.iso',
				'audio' : False,
				'console' : False,
				'net' : False,
				'usb' : False,
				'xhci' : False,
				'tablet' : False,
				'expect' : {
					'src' : 'ok ',
					'dst' : 'boot\n'
				},
			}
		}
	},
}

def usage():
	print("%s - emulator wrapper for running HelenOS\n" % os.path.basename(sys.argv[0]))
	print("%s [-d] [-h] [-net e1k|rtl8139|ne2k] [-nohdd] [-nokvm] [-nonet] [-nosnd] [-nousb] [-noxhci] [-notablet]\n" %
	    os.path.basename(sys.argv[0]))
	print("-d\tDry run: do not run the emulation, just print the command line.")
	print("-h\tPrint the usage information and exit.")
	print("-nohdd\tDisable hard disk, if applicable.")
	print("-nokvm\tDisable KVM, if applicable.")
	print("-nonet\tDisable networking support, if applicable.")
	print("-nosnd\tDisable sound, if applicable.")
	print("-nousb\tDisable USB support, if applicable.")
	print("-noxhci\tDisable XHCI support, if applicable.")
	print("-notablet\tDisable USB tablet (use only relative-position PS/2 mouse instead), if applicable.")

def fail(platform, machine):
	print("Cannot start emulation for the chosen configuration. (%s/%s)" % (platform, machine))


def run():
	expect_nic = False
	expect_qemu = False

	for i in range(1, len(sys.argv)):

		if expect_nic:
			expect_nic = False
			if not 'net' in overrides.keys():
				overrides['net'] = {}
			if sys.argv[i] == 'e1k':
				overrides['net']['e1k'] = True
			elif sys.argv[i] == 'rtl8139':
				overrides['net']['rtl8139'] = True
			elif sys.argv[i] == 'ne2k':
				overrides['net']['ne2k'] = True
			else:
				usage()
				exit()

		if expect_qemu:
			expect_qemu = False
			overrides['qemu_path'] = sys.argv[i]

		elif sys.argv[i] == '-h':
			usage()
			exit()
		elif sys.argv[i] == '-d':
			overrides['dryrun'] = True
		elif sys.argv[i] == '-net' and i < len(sys.argv) - 1:
			expect_nic = True
		elif sys.argv[i] == '-nohdd':
			overrides['nohdd'] = True
		elif sys.argv[i] == '-nokvm':
			overrides['nokvm'] = True
		elif sys.argv[i] == '-nonet':
			overrides['nonet'] = True
		elif sys.argv[i] == '-nosnd':
			overrides['nosnd'] = True
		elif sys.argv[i] == '-nousb':
			overrides['nousb'] = True
		elif sys.argv[i] == '-noxhci':
			overrides['noxhci'] = True
		elif sys.argv[i] == '-notablet':
			overrides['notablet'] = True
		elif sys.argv[i] == '-qemu_path' and i < len(sys.argv) - 1:
			expect_qemu = True
		else:
			usage()
			exit()

	config = {}
	autotool.read_config(autotool.CONFIG, config)

	if 'PLATFORM' in config.keys():
		platform = config['PLATFORM']
	else:
		platform = ''

	if 'MACHINE' in config.keys():
		mach = config['MACHINE']
	else:
		mach = ''

	if 'PROCESSOR' in config.keys():
		processor = config['PROCESSOR']
	else:
		processor = ''

	try:
		emu_run = cfg_get(platform, mach, processor)['run']
		emu_run(platform, mach, processor)
	except:
		fail(platform, mach)
		return

run()
