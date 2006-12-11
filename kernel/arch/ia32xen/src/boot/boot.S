#
# Copyright (C) 2006 Martin Decky
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

#include <arch/boot/boot.h>
#include <arch/mm/page.h>
#include <arch/pm.h>

#define ELFNOTE(name, type, desctype, descval) \
	.section .note.name; \
	.align 4; \
	.long 2f - 1f; \
	.long 4f - 3f; \
	.long type; \
	1:.asciz #name; \
	2:.align 4; \
	3:desctype descval; \
	4:.align 4

ELFNOTE(Xen, XEN_ELFNOTE_GUEST_OS,			.asciz,	"HelenOS")
ELFNOTE(Xen, XEN_ELFNOTE_GUEST_VERSION,		.asciz,	RELEASE)
ELFNOTE(Xen, XEN_ELFNOTE_XEN_VERSION,		.asciz,	"xen-3.0")
ELFNOTE(Xen, XEN_ELFNOTE_VIRT_BASE,			.long,	PA2KA(BOOT_OFFSET))
ELFNOTE(Xen, XEN_ELFNOTE_PADDR_OFFSET,		.long,	0)
ELFNOTE(Xen, XEN_ELFNOTE_ENTRY,				.long,	kernel_image_start)
ELFNOTE(Xen, XEN_ELFNOTE_HYPERCALL_PAGE,	.long,	hypercall_page)
ELFNOTE(Xen, XEN_ELFNOTE_FEATURES,			.asciz,	"auto_translated_physmap|supervisor_mode_kernel")
ELFNOTE(Xen, XEN_ELFNOTE_PAE_MODE,			.asciz,	"no")
ELFNOTE(Xen, XEN_ELFNOTE_LOADER,			.asciz,	"generic")

.text

.code32
.align 4
.global kernel_image_start
	
kernel_image_start:
	# copy start_info (esi initialized by Xen)
	
	movl $start_info, %edi
	movl $START_INFO_SIZE >> 2, %ecx
	cld
	rep movsb
	
	# switch to temporal kernel stack
	
	movl $kernel_stack, %esp
	
	call arch_pre_main
	call main_bsp								# never returns

	cli
	hlt

kernel_stack_bottom:
	.space TEMP_STACK_SIZE
kernel_stack:

.section K_TEXT_START, "aw", @progbits
.global hypercall_page
.org 0
hypercall_page:
	.space PAGE_SIZE

.global shared_info
.org 0x1000
shared_info:
	.space PAGE_SIZE

.global console_page
.org 0x2000
console_page:
	.space PAGE_SIZE

# Xen 3.0.3 ELF loader is somehow buggy
# thus this workaround
.global dummy_fill
dummy_fill:
	.space (1024 * 1024)
