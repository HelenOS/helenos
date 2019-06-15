arch_src += files(
	'src/atomic.c',
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.c',
	'src/fibril.S',
	'src/tls.c',
	'src/eabi.S',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
	'src/rtld/dynamic.c',
	'src/rtld/reloc.c',
)

arch_start_src = files('src/crt0.S')

if MACHINE == 'gta02'
	rd_essential += [
		'srv/hid/s3c24xx_ts',
		'srv/hw/char/s3c24xx_uart',
	]

elif MACHINE == 'beagleboardxm' or MACHINE == 'beaglebone'
	rd_essential += [
		'drv/platform/amdm37x',
		'drv/fb/amdm37x_dispc',
	]

elif MACHINE == 'integratorcp'
	rd_essential += [
		'drv/char/pl050',
		'drv/hid/atkbd',
		'drv/hid/ps2mouse',
		'drv/intctl/icp-ic',
		'drv/platform/icp',
	]
endif

rd_essential += [
	'drv/bus/usb/ehci',
	'drv/bus/usb/ohci',
	'drv/bus/usb/usbdiag',
	'drv/bus/usb/usbflbk',
	'drv/bus/usb/usbhub',
	'drv/bus/usb/usbmid',
	'drv/block/usbmast',
	'drv/hid/usbhid',
]
