#
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

BOOT_OUTPUT = image.boot
POST_OUTPUT = $(ROOT_PATH)/image.iso
POSTBUILD = Makefile.grub
GRUB_LOADER = chainloader

BFD_NAME = elf64-littleaarch64
# Request binary BFD output. The ARM64 port manually prepares the .text
# section to look as a PE file than can be loaded on EFI systems.
BFD_OUTPUT = binary
BFD_ARCH = aarch64

BITS = 64
ENDIANESS = LE

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
EXTRA_CFLAGS = -fpic -fvisibility=hidden
EXTRA_LDFLAGS = -Wl,-shared

ifeq ($(MACHINE), virt)
	RD_DRVS_ESSENTIAL += \
		char/pl011 \
		intctl/gicv2 \
		platform/arm64virt
endif

SOURCES = \
	arch/$(BARCH)/src/asm.S \
	arch/$(BARCH)/src/main.c \
	arch/$(BARCH)/src/relocate.c \
	$(COMPS).o \
	genarch/src/efi.c \
	generic/src/gzip.c \
	generic/src/inflate.c \
	generic/src/kernel.c \
	generic/src/memstr.c \
	generic/src/payload.c \
	generic/src/printf.c \
	generic/src/printf_core.c \
	generic/src/str.c \
	generic/src/tar.c \
	generic/src/version.c \
	generic/src/vprintf.c
