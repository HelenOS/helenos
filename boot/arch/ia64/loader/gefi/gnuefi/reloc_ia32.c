/* reloc_ia32.c - position independent x86 ELF shared object relocator
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

#include <elf.h>
#include <link.h>	/* get _DYNAMIC decl and ElfW and ELFW macros */

#undef NULL
#define uint64_t	efi_uint64_t
#define int64_t		efi_int64_t
#define uint32_t	efi_uint32_t
#define int32_t		efi_int32_t
#define uint16_t	efi_uint16_t
#define int16_t		efi_int16_t
#define uint8_t		efi_uint8_t
#define int8_t		efi_int8_t

#undef NULL
#define uint64_t	efi_uint64_t
#define int64_t		efi_int64_t
#define uint32_t	efi_uint32_t
#define int32_t		efi_int32_t
#define uint16_t	efi_uint16_t
#define int16_t		efi_int16_t
#define uint8_t		efi_uint8_t
#define int8_t		efi_int8_t

#include <efi.h>
#include <efilib.h>

int
_relocate (long ldbase, ElfW(Dyn) *dyn, EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
	extern EFI_STATUS efi_main (EFI_HANDLE, EFI_SYSTEM_TABLE *);
	long relsz = 0, relent = 0;
	ElfW(Rel) *rel = 0;
	int i;

	for (i = 0; dyn[i].d_tag != DT_NULL; ++i) {
		switch (dyn[i].d_tag) {
		      case DT_REL:
			rel = (ElfW(Rel)*) ((long) dyn[i].d_un.d_ptr + ldbase);
			break;

		      case DT_RELSZ:
			relsz = dyn[i].d_un.d_val;
			break;

		      case DT_RELENT:
			relent = dyn[i].d_un.d_val;
			break;

		      case DT_RELA:
			break;

		      default:
			break;
		}
	}

	while (relsz > 0) {
 		if (!rel || relent == 0)
 			return EFI_LOAD_ERROR;

		/* apply the relocs */
		switch (ELF32_R_TYPE (rel->r_info)) {
		      case R_386_NONE:
			break;

		      case R_386_RELATIVE:
		      {
			      long *addr;

			      addr = (long *) (ldbase + rel->r_offset);
			      *addr += ldbase;
			      break;
		      }

		      default:
			return EFI_LOAD_ERROR;
		}
		rel = (ElfW(Rel)*) ((char *) rel + relent);
		relsz -= relent;
	}
	return EFI_SUCCESS;
}
