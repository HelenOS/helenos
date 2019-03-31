/*
 * Copyright (c) 2019 Petr Pavlu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup boot_arm64
 * @{
 */
/** @file
 * @brief Image self-relocation support.
 */

#include <arch/relocate.h>
#include <printf.h>

/** Self-relocate the bootloader.
 *
 * Note: This code is responsible for self-relocating the bootloader when it is
 * started. As such, it is required that it is written in a way that it itself
 * does not need any dynamic relocation.
 */
efi_status_t self_relocate(uintptr_t base, const elf_dyn_t *dyn)
{
	const elf_rela_t *rela = NULL;
	uint64_t relasz = 0;
	uint64_t relaent = 0;

	/* Parse the dynamic array. */
	while (dyn->d_tag != DT_NULL) {
		switch (dyn->d_tag) {
		case DT_RELA:
			rela = (const elf_rela_t *) (base + dyn->d_un.d_ptr);
			break;
		case DT_RELASZ:
			relasz = dyn->d_un.d_val;
			break;
		case DT_RELAENT:
			relaent = dyn->d_un.d_val;
			break;
		}
		dyn++;
	}

	/* Validate obtained information. */
	if (rela == NULL)
		return EFI_SUCCESS;
	if (relaent == 0 || relasz % relaent != 0)
		return EFI_UNSUPPORTED;

	/* Process relocations in the image. */
	while (relasz > 0) {
		switch (ELF_R_TYPE(rela->r_info)) {
		case R_AARCH64_RELATIVE:
			*(uint64_t *) (base + rela->r_offset) =
			    base + rela->r_addend;
			break;
		default:
			return EFI_UNSUPPORTED;
		}

		rela = (const elf_rela_t *) ((const char *) rela + relaent);
		relasz -= relaent;
	}
	return EFI_SUCCESS;
}

/** @}
 */
