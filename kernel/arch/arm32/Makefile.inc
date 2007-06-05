#
# Copyright (c) 2005 Martin Decky
# Copyright (c) 2007 Jakub Jermar
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

## Toolchain configuration
#

BFD_NAME = elf32-little
BFD_ARCH = arm
BFD = elf32-little
TARGET = arm-linux-gnu
TOOLCHAIN_DIR = /usr/local/arm

GCC_CFLAGS += 

DEFS += -D__32_BITS__ -DMACHINE=$(MACHINE)

## Compile with hierarchical page tables support.
#

CONFIG_PAGE_PT = y
DEFS += -DCONFIG_PAGE_PT

## Compile with support for address space identifiers.
#

CONFIG_ASID = y
CONFIG_ASID_FIFO = y

## Compile with support with software division and multiplication.
#

CONFIG_SOFTINT = y

ARCH_SOURCES = \
	arch/$(ARCH)/src/arm32.c \
	arch/$(ARCH)/src/start.S \
	arch/$(ARCH)/src/context.S \
	arch/$(ARCH)/src/dummy.S \
	arch/$(ARCH)/src/cpu/cpu.c \
	arch/$(ARCH)/src/ddi/ddi.c \
	arch/$(ARCH)/src/mm/as.c \
	arch/$(ARCH)/src/mm/frame.c \
	arch/$(ARCH)/src/mm/page.c
	