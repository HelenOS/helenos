/* crt0-efi-ia64.S - IA-64 EFI startup code.
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
	.psr abi64
	.psr lsb
	.lsb

	.proc _start
_start:
	alloc loc0=ar.pfs,2,2,2,0
	mov loc1=rp
	movl out0=@gprel(ImageBase)	// out0 <- ImageBase (ldbase)
	;;
	add out0=out0,gp
	movl out1=@gprel(_DYNAMIC)	// out1 <- _DYNAMIC
	;;		// avoid WAW on CFM
	add out1=out1,gp
	br.call.sptk.few rp=_relocate
.Lret0:	
	cmp.ne p6,p0=r0,r8		// r8 == EFI_SUCCESS?
(p6)	br.cond.sptk.few .exit		// no ->

.Lret1:

	mov out0=in0			// image handle
	mov out1=in1			// systab
	br.call.sptk.few rp=efi_main
.Lret2:
.exit:
	mov ar.pfs=loc0
	mov rp=loc1
	;;
	br.ret.sptk.few rp

	.endp _start


	// PE32+ wants a PLABEL, not the code address of the entry point:

	.align 16
	.global _start_plabel
	.section .plabel, "a"
_start_plabel:
	data8	_start
	data8	__gp

	// hand-craft a .reloc section for the plabel:

#define IMAGE_REL_BASED_DIR64	10

	.section .reloc, "a"
	data4	_start_plabel				// Page RVA
	data4	12					// Block Size (2*4+2*2)
	data2	(IMAGE_REL_BASED_DIR64<<12) +  0	// reloc for plabel's entry point
	data2	(IMAGE_REL_BASED_DIR64<<12) +  8	// reloc for plabel's global pointer
