#
# Copyright (C) 2001-2004 Jakub Jermar
# Copyright (C) 2005-2006 Martin Decky
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
#include <arch/boot/memmap.h>
#include <arch/mm/page.h>
#include <arch/pm.h>

#define START_STACK (BOOT_OFFSET - BOOT_STACK_SIZE)

.section K_TEXT_START, "ax"

KTEXT=8
KDATA=16

.code32
.align 4
.global multiboot_image_start
multiboot_header:
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)	# checksum
	.long multiboot_header
	.long unmapped_ktext_start
	.long 0
	.long 0
	.long multiboot_image_start
	
multiboot_image_start:
	movl $START_STACK, %esp			# initialize stack pointer
	lgdt KA2PA(bootstrap_gdtr)		# initialize Global Descriptor Table register

	movw $KDATA, %cx
	movw %cx, %es
	movw %cx, %gs
	movw %cx, %fs
	movw %cx, %ds					# kernel data + stack
	movw %cx, %ss
	
	jmpl $KTEXT, $multiboot_meeting_point
	multiboot_meeting_point:
	
	pushl %ebx							# save parameters from GRUB
	pushl %eax

#ifdef CONFIG_FB
	mov $vesa_init,%esi;
	mov $VESA_INIT_SEGMENT<<4,%edi;
	mov $e_vesa_init-vesa_init,%ecx;
	cld;
	rep movsb;

	mov $VESA_INIT_SEGMENT<<4,%edi;
	call *%edi;
	mov %esi,KA2PA(vesa_ph_addr);
	mov %di,KA2PA(vesa_height);
	shr $16,%edi;
	mov %di,KA2PA(vesa_width);
	mov %bx,KA2PA(vesa_scanline);
	shr $16,%ebx;
	mov %bx,KA2PA(vesa_bpp);
#endif	
	
	call map_kernel							# map kernel and turn paging on
	
	popl %eax
	popl %ebx
	cmpl $MULTIBOOT_LOADER_MAGIC, %eax				# compare GRUB signature
	je valid_boot
		
		xorl %ecx, %ecx							# no memory size or map available
		movl %ecx, e801memorysize
		movl %ecx, e820counter
		
		jmp invalid_boot
		
	valid_boot:
		
		movl (%ebx), %eax						# ebx = physical address of struct multiboot_info
		
		bt $0, %eax								# mbi->flags[0] (mem_lower, mem_upper valid)
		jc mem_valid
			
			xorl %ecx, %ecx
			jmp mem_invalid
			
		mem_valid:
		movl 4(%ebx), %ecx						# mbi->mem_lower
		addl 8(%ebx), %ecx						# mbi->mem_upper
		
		mem_invalid:
		movl %ecx, e801memorysize
		
		bt $3, %eax								# mbi->flags[3] (mods_count, mods_addr valid)
		jc mods_valid
			
			xorl %ecx, %ecx
			movl %ecx, init
			jmp mods_end
		
		mods_valid:
		
		movl 20(%ebx), %ecx						# mbi->mods_count
		movl %ecx, init
		
		cmpl $0, %ecx
		je mods_end
		
		movl 24(%ebx), %esi						# mbi->mods_addr
		movl $init, %edi
		
		mods_loop:
		
			movl 0(%esi), %edx					# mods->mod_start
			addl $0x80000000, %edx
			movl %edx, 4(%edi)
			
			movl 4(%esi), %edx
			subl 0(%esi), %edx					# mods->mod_end - mods->mod_start
			movl %edx, 8(%edi)
			
			addl $16, %esi
			addl $8	, %edi
			
			loop mods_loop
			
		mods_end:
		
		bt $6, %eax								# mbi->flags[6] (mmap_length, mmap_addr valid)	
		jc mmap_valid
			
			xorl %edx, %edx
			jmp mmap_invalid
			
		mmap_valid:
		movl 44(%ebx), %ecx						# mbi->mmap_length
		movl 48(%ebx), %esi						# mbi->mmap_addr
		movl $e820table, %edi
		xorl %edx, %edx
		
		mmap_loop:
			cmpl $0, %ecx
			jle mmap_end
			
			movl 4(%esi), %eax					# mmap->base_addr_low
			movl %eax, (%edi)
			
			movl 8(%esi), %eax					# mmap->base_addr_high
			movl %eax, 4(%edi)
			
			movl 12(%esi), %eax					# mmap->length_low
			movl %eax, 8(%edi)
			
			movl 16(%esi), %eax					# mmap->length_high
			movl %eax, 12(%edi)
			
			movl 20(%esi), %eax					# mmap->type
			movl %eax, 16(%edi)
			
			movl (%esi), %eax					# mmap->size
			addl $0x4, %eax
			addl %eax, %esi
			subl %eax, %ecx
			addl $MEMMAP_E820_RECORD_SIZE, %edi
			incl %edx
			jmp mmap_loop
		
		mmap_end:
		
		mmap_invalid:
		movl %edx, e820counter
		
	invalid_boot:
	
#ifdef CONFIG_SMP
	
	# copy AP bootstrap routines below 1 MB
	
	movl $BOOT_OFFSET, %esi
	movl $AP_BOOT_OFFSET, %edi
	movl $_hardcoded_unmapped_size, %ecx
	cld
	rep movsb
	
#endif
	
	call main_bsp								# never returns

	cli
	hlt

.global map_kernel
map_kernel:
	#
	# Here we setup mapping for both the unmapped and mapped sections of the kernel.
	# For simplicity, we map the entire 4G space.
	#
	movl %cr4, %ecx
	orl $(1<<4), %ecx
	movl %ecx, %cr4							# turn PSE on
	
	movl $(page_directory+0), %esi
	movl $(page_directory+2048), %edi
	xorl %ecx, %ecx
	xorl %ebx, %ebx
0:
	movl $((1<<7)|(1<<0)), %eax
	orl %ebx, %eax
	movl %eax, (%esi,%ecx,4)					# mapping 0x00000000+%ecx*4M => 0x00000000+%ecx*4M
	movl %eax, (%edi,%ecx,4)					# mapping 0x80000000+%ecx*4M => 0x00000000+%ecx*4M
	addl $(4*1024*1024), %ebx

	incl %ecx
	cmpl $512, %ecx
	jl 0b

	movl %esi, %cr3
	
	# turn paging on
	movl %cr0, %ebx
	orl $(1<<31), %ebx
	movl %ebx, %cr0
	ret

#ifdef CONFIG_FB
vesa_init:
	jmp $selector(VESA_INIT_DES),$vesa_init_real-vesa_init;
.code16
vesa_init_real:	

	mov %cr0,%eax;
	and $~1,%eax;
	mov %eax,%cr0;


	jmp $VESA_INIT_SEGMENT,$vesa_init_real2-vesa_init;

vesa_init_real2:	


	mov %esp,%ebp;
	mov %ss,%cx;
	mov $VESA_INIT_SEGMENT,%bx;
	mov %bx,%ss;
	mov $0x0000fffc,%esp;
	push %ds;
	push %es;
	push %fs;
	push %gs;
	push %ebp;
	push %cx;

	mov %bx,%ds;
	mov %bx,%es;
	mov %bx,%fs;
	mov %bx,%gs;

	
	mov $vesa_idt-vesa_init,%ebx;
	lidtl (%ebx);

#define VESA_INFO_SIZE 1024

#define VESA_MODE_LIST_PTR_OFFSET 14 
#define VESA_MODE_WIDTH_OFFSET 18
#define VESA_MODE_HEIGHT_OFFSET 20
#define VESA_MODE_BPP_OFFSET 25
#define VESA_MODE_SCANLINE_OFFSET 16
#define VESA_MODE_PHADDR_OFFSET 40

#define VESA_END_OF_MODES 0xffff 

#define VESA_OK 0x4f 

#define VESA_GET_INFO 0x4f00 
#define VESA_GET_MODE_INFO 0x4f01
#define VESA_SET_MODE 0x4f02

#define CONFIG_VESA_BPP_a 255

#if CONFIG_VESA_BPP==24
#undef CONFIG_VESA_BPP_a
#define CONFIG_VESA_BPP_a 32
#endif


	mov $VESA_GET_INFO,%ax;
	mov $e_vesa_init-vesa_init,%di
	push %di;
	int $0x10;	
	pop %di;
	cmp $VESA_OK,%al;
	jnz 0f;
	mov 2+VESA_MODE_LIST_PTR_OFFSET(%di),%si;
	mov %si,%gs;
	mov VESA_MODE_LIST_PTR_OFFSET(%di),%si;

	add $VESA_INFO_SIZE,%di;

1:# Try next mode
	mov %gs:(%si),%cx;
	cmp $VESA_END_OF_MODES,%cx;
	jz 0f;
	inc %si;
	inc %si;
	push %cx;
	push %di;
	push %si;
	mov $VESA_GET_MODE_INFO,%ax;
	int $0x10;
	pop %si;
	pop %di;
	pop %cx;
	cmp $VESA_OK,%al;
	jnz 0f;


	mov $CONFIG_VESA_WIDTH,%ax;
	cmp VESA_MODE_WIDTH_OFFSET(%di),%ax;
	jnz 1b;
	mov $CONFIG_VESA_HEIGHT,%ax;
	cmp VESA_MODE_HEIGHT_OFFSET(%di),%ax;
	jnz 1b;
	mov $CONFIG_VESA_BPP,%al;
	cmp VESA_MODE_BPP_OFFSET(%di),%al;
	jz 2f;
	mov $CONFIG_VESA_BPP_a,%al;
	cmp VESA_MODE_BPP_OFFSET(%di),%al;
	jnz 1b;

2:

	mov %cx,%bx;
	or $0xC000,%bx;
	push %di;
	mov $VESA_SET_MODE,%ax;
	int $0x10;
	pop %di;
	cmp $VESA_OK,%al;
	jnz 0f;
	
	mov VESA_MODE_PHADDR_OFFSET(%di),%esi;
	mov VESA_MODE_WIDTH_OFFSET(%di),%ax;
	shl $16,%eax;
	mov VESA_MODE_HEIGHT_OFFSET(%di),%ax;
	mov VESA_MODE_BPP_OFFSET(%di),%bl;
	xor %bh,%bh;
	shl $16,%ebx;
	mov VESA_MODE_SCANLINE_OFFSET(%di),%bx;
	mov %eax,%edi; 
	
	

8:	

	mov %cr0,%eax;
	or $1,%eax;
	mov %eax,%cr0;

	jmp 9f;
9:

	pop %cx;
	pop %ebp;
	pop %gs;
	pop %fs;
	pop %es;
	pop %ds;
	mov %cx,%ss;
	mov %ebp,%esp;

	ljmpl $KTEXT,$(vesa_init_protect-vesa_init+VESA_INIT_SEGMENT<<4);


0:	#Error no Prefered mode found
	mov $0x111,%cx;
	push %di;
	push %cx;
	mov $VESA_GET_MODE_INFO,%ax;
	int $0x10;
	pop %cx;
	pop %di;
	cmp $VESA_OK,%al;
	jnz 1f;
	jz 2b;    /* Force relative jump */

1:mov $0x0003,%ax;
	int $0x10;
	mov $0xffffffff,%edi; /* EGA text mode used, because of problems with VESA */
	xor %ax,%ax
	jz 8b;    /* Force relative jump */

vesa_init_protect:	
.code32
	ret;

vesa_idt:
.word 0x03ff
.long 0
.align 4
e_vesa_init:
#endif	

.section K_DATA_START, "aw", @progbits

.align 4096
page_directory:
	.space 4096, 0
