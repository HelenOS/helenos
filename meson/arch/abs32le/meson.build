
arch_kernel_c_args = [ '-D__LE__' ]
arch_uspace_c_args = [ '-D__LE__' ]

if CROSS_TARGET == 'arm32'
	atsign = '%'
elif CROSS_TARGET == 'mips32'
	arch_kernel_c_args += [ '-mno-abicalls' ]
endif

arch_kernel_link_args = [ '-nostdlib' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]
