/* reloc_ia64.S - position independent IA-64 ELF shared object relocator
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

/*
 * This is written in assembly because the entire code needs to be position
 * independent.  Note that the compiler does not generate code that's position
 * independent by itself because it relies on the global offset table being
 * relocated.
 */
	.text
	.psr abi64
	.psr lsb
	.lsb

/*
 * This constant determines how many R_IA64_FPTR64LSB relocations we
 * can deal with.  If you get EFI_BUFFER_TOO_SMALL errors, you may
 * need to increase this number.
 */
#define MAX_FUNCTION_DESCRIPTORS	750

#define ST_VALUE_OFF	8		/* offset of st_value in elf sym */

#define EFI_SUCCESS		0
#define EFI_LOAD_ERROR		1
#define EFI_BUFFER_TOO_SMALL	5

#define DT_NULL		0		/* Marks end of dynamic section */
#define DT_RELA		7		/* Address of Rela relocs */
#define DT_RELASZ	8		/* Total size of Rela relocs */
#define DT_RELAENT	9		/* Size of one Rela reloc */
#define DT_SYMTAB	6		/* Address of symbol table */
#define DT_SYMENT	11		/* Size of one symbol table entry */

#define R_IA64_NONE		0
#define R_IA64_REL64MSB		0x6e
#define R_IA64_REL64LSB		0x6f
#define R_IA64_DIR64MSB		0x26
#define R_IA64_DIR64LSB		0x27
#define R_IA64_FPTR64MSB	0x46
#define R_IA64_FPTR64LSB	0x47

#define	ldbase	in0	/* load address (address of .text) */
#define	dyn	in1	/* address of _DYNAMIC */

#define d_tag	r16
#define d_val	r17
#define rela	r18
#define relasz	r19
#define relaent	r20
#define addr	r21
#define r_info	r22
#define r_offset r23
#define r_addend r24
#define r_type	r25
#define r_sym	r25	/* alias of r_type ! */
#define fptr	r26
#define fptr_limit r27
#define symtab	f8
#define syment	f9
#define ftmp	f10

#define	target	r16
#define val	r17

#define NLOC	0

#define Pnull		p6
#define Prela		p7
#define Prelasz		p8
#define Prelaent	p9
#define Psymtab		p10
#define Psyment		p11

#define Pnone		p6
#define Prel		p7
#define Pfptr		p8

#define Pmore		p6

#define Poom		p6	/* out-of-memory */

	.global _relocate
	.proc _relocate
_relocate:
	alloc r2=ar.pfs,2,0,0,0
	movl	fptr = @gprel(fptr_mem_base)
	;;
	add	fptr = fptr, gp
	movl	fptr_limit = @gprel(fptr_mem_limit)
	;;
	add	fptr_limit = fptr_limit, gp

search_dynamic:
	ld8	d_tag = [dyn],8
	;;
	ld8	d_val = [dyn],8
	cmp.eq	Pnull,p0 = DT_NULL,d_tag
(Pnull)	br.cond.sptk.few apply_relocs
	cmp.eq	Prela,p0 = DT_RELA,d_tag
	cmp.eq	Prelasz,p0 = DT_RELASZ,d_tag
	cmp.eq	Psymtab,p0 = DT_SYMTAB,d_tag
	cmp.eq	Psyment,p0 = DT_SYMENT,d_tag
	cmp.eq	Prelaent,p0 = DT_RELAENT,d_tag
	;;
(Prela)	add rela = d_val, ldbase
(Prelasz) mov relasz = d_val
(Prelaent) mov relaent = d_val
(Psymtab) add val = d_val, ldbase
(Psyment) setf.sig syment = d_val
	;;
(Psymtab) setf.sig symtab = val
	br.sptk.few search_dynamic

apply_loop:
	ld8	r_offset = [rela]
	add	addr = 8,rela
	sub	relasz = relasz,relaent
	;;

	ld8	r_info = [addr],8
	;;
	ld8	r_addend = [addr]
	add	target = ldbase, r_offset

	add	rela = rela,relaent
	extr.u	r_type = r_info, 0, 32
	;;
	cmp.eq	Pnone,p0 = R_IA64_NONE,r_type
	cmp.eq	Prel,p0 = R_IA64_REL64LSB,r_type
	cmp.eq	Pfptr,p0 = R_IA64_FPTR64LSB,r_type
(Prel)	br.cond.sptk.few apply_REL64
	;;
	cmp.eq	Prel,p0 = R_IA64_DIR64LSB,r_type // treat DIR64 just like REL64

(Pnone)	br.cond.sptk.few apply_relocs
(Prel)	br.cond.sptk.few apply_REL64
(Pfptr)	br.cond.sptk.few apply_FPTR64

	mov	r8 = EFI_LOAD_ERROR
	br.ret.sptk.few rp

apply_relocs:
	cmp.ltu	Pmore,p0=0,relasz
(Pmore)	br.cond.sptk.few apply_loop

	mov	r8 = EFI_SUCCESS
	br.ret.sptk.few rp

apply_REL64:
	ld8 val = [target]
	;;
	add val = val,ldbase
	;;
	st8 [target] = val
	br.cond.sptk.few apply_relocs

	// FPTR relocs are a bit more interesting: we need to lookup
	// the symbol's value in symtab, allocate 16 bytes of memory,
	// store the value in [target] in the first and the gp in the
	// second dword.
apply_FPTR64:
	st8	[target] = fptr
	extr.u	r_sym = r_info,32,32
	add	target = 8,fptr
	;;

	setf.sig ftmp = r_sym
	mov	r8=EFI_BUFFER_TOO_SMALL
	;;
	cmp.geu	Poom,p0 = fptr,fptr_limit

	xma.lu	ftmp = ftmp,syment,symtab
(Poom)	br.ret.sptk.few rp
	;;
	getf.sig addr = ftmp
	st8	[target] = gp
	;;
	add	addr = ST_VALUE_OFF, addr
	;;
	ld8	val = [addr]
	;;
	add	val = val,ldbase
	;;
	st8	[fptr] = val,16
	br.cond.sptk.few apply_relocs

	.endp _relocate

	.data
	.align 16
fptr_mem_base:
	.space  MAX_FUNCTION_DESCRIPTORS*16
fptr_mem_limit:
