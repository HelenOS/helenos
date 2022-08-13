#
# SPDX-FileCopyrightText: 2021 Jiri Svoboda
# SPDX-FileCopyrightText: 2015 Petr Pavlu
#
# SPDX-License-Identifier: BSD-3-Clause
#

arch_uspace_c_args = [ '-D__LE__', '-fno-omit-frame-pointer' ]
arch_kernel_c_args = arch_uspace_c_args + [ '-march=armv8-a+nofp+nosimd', '-mgeneral-regs-only' ]
arch_kernel_link_args = [ '-nostdlib' ]
arch_uspace_link_args = [ '-nostdlib', '-lgcc' ]

# UEFI binaries should be relocatable, the EFI boot service LoadImage() will
# rebase the boot file using the .reloc information in the image if it cannot
# load the binary at its preferred address. The ARM64 port does not provide this
# information in its PE file (that would require manually creating it) but
# instead the boot code is compiled with the -fpic option and the bootloader
# relocates itself at runtime.
#
# N.B. The UEFI guarantees for AArch64 that during boot time the primary
# processor is in the execution mode that has unaligned access enabled. The
# -mstrict-align option is therefore not needed.
arch_boot_c_args = arch_uspace_c_args + [ '-fpic', '-fvisibility=hidden', '-fno-function-sections' ]
arch_boot_link_args = [ '-Wl,-shared', '-Wl,--no-gc-sections' ]

if MACHINE == 'virt'
	rd_essential_drv += [
		'drv/char/pl011',
		'drv/intctl/gicv2',
		'drv/platform/arm64virt',
	]
endif

if MACHINE == 'hikey960'
	rd_essential_drv += [
		'drv/char/pl011',
		'drv/intctl/gicv2',
		'drv/platform/hikey960',
	]
endif

rd_drv += rd_essential_drv
