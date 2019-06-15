arch_src += [ autocheck.process('include/libarch/fibril_context.h') ]

arch_src += files(
	'src/entryjmp.S',
	'src/thread_entry.S',
	'src/syscall.c',
	'src/fibril.S',
	'src/tls.c',
	'src/stacktrace.c',
	'src/stacktrace_asm.S',
	'src/rtld/dynamic.c',
	'src/rtld/reloc.c',
)

arch_start_src = files('src/crt0.S')

rd_essential += [
	'drv/platform/mac',
	'drv/bus/adb/cuda_adb',
	'drv/bus/pci/pciintel',
	'drv/bus/usb/ohci',
	'drv/bus/usb/usbdiag',
	'drv/bus/usb/usbflbk',
	'drv/bus/usb/usbhub',
	'drv/bus/usb/usbmid',
	'drv/bus/usb/vhc',
	'drv/block/usbmast',
	'drv/hid/adb-kbd',
	'drv/hid/adb-mouse',
	'drv/hid/usbhid',
]
