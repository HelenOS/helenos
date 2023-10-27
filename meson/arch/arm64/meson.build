#
# Copyright (c) 2021 Jiri Svoboda
# Copyright (c) 2015 Petr Pavlu
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

arch_uspace_c_args = [
	'-D__LE__',
	'-fno-omit-frame-pointer',
	cc.get_supported_arguments(['-mno-outline-atomics']),
]
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
arch_boot_c_args = arch_uspace_c_args + [ '-fpic', '-fvisibility=hidden' ]
arch_boot_link_args = [ '-Wl,-shared' ] + ldflags_ignore_rwx_segments

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
