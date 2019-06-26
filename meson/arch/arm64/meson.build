arch_uspace_c_args = [ '-D__LE__', '-fno-omit-frame-pointer' ]
arch_kernel_c_args = arch_uspace_c_args + [ '-march=armv8-a+nofp+nosimd', '-mgeneral-regs-only' ]
arch_kernel_link_args = [ '-nostdlib' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]


if MACHINE == 'virt'
	rd_essential += [
		'drv/char/pl011',
		'drv/intctl/gicv2',
		'drv/platform/arm64virt',
	]
endif
