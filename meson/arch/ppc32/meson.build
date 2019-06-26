# FIXME: enable --gc-sections

_common_c_args = [ '-D__BE__', '-fno-omit-frame-pointer', '-m32', '-Wa,-a32', '-mcpu=powerpc' ]

arch_kernel_c_args = _common_c_args + [ '-msoft-float', '-fno-function-sections' ]
arch_uspace_c_args = _common_c_args + [ CONFIG_FPU ? '-mhard-float' : '-msoft-float' ]
arch_kernel_link_args = [ '-nostdlib', '-Wl,-z,max-page-size=0x1000', '-Wl,--no-check-sections', '-Wl,--no-gc-sections' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc', '-Wl,-z,max-page-size=0x1000' ]


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
