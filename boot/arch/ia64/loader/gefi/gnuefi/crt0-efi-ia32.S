/* crt0-efi-ia32.S - x86 EFI startup code.
   Copyright (C) 1999 Hewlett-Packard Co.
	Contributed by David Mosberger <davidm@hpl.hp.com>.

   This file is part of GNU-EFI, the GNU EFI development environment.

   GNU EFI is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU EFI is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU EFI; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA. */

	.text
	.align 4

	.globl _start
_start:
	pushl %ebp
	movl %esp,%ebp

	pushl 12(%ebp)			# copy "image" argument
	pushl  8(%ebp)			# copy "systab" argument

	call 0f
0:	popl %eax
	movl %eax,%ebx

	addl $ImageBase-0b,%eax		# %eax = ldbase
	addl $_DYNAMIC-0b,%ebx		# %ebx = _DYNAMIC

	pushl %ebx			# pass _DYNAMIC as second argument
	pushl %eax			# pass ldbase as first argument
	call _relocate
	popl %ebx
	popl %ebx
 	testl %eax,%eax
 	jne .exit
  
  	call efi_main			# call app with "image" and "systab" argument

.exit:	leave
  	ret
 
 	// hand-craft a dummy .reloc section so EFI knows it's a relocatable executable:
 
 	.data
dummy:	.long	0

#define IMAGE_REL_ABSOLUTE	0
 	.section .reloc, "a"
 	.long	dummy					// Page RVA
 	.long	10					// Block Size (2*4+2)
 	.word	(IMAGE_REL_ABSOLUTE<<12) +  0		// reloc for dummy
