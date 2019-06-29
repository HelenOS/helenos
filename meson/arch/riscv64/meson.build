arch_uspace_c_args = [ '-D__LE__', '-fno-omit-frame-pointer' ]
arch_kernel_c_args = arch_uspace_c_args + [ '-mcmodel=medany' ]
arch_boot_c_args = arch_kernel_c_args
arch_kernel_link_args = [ '-nostdlib' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]
arch_boot_link_args = []

rd_essential += [
]
