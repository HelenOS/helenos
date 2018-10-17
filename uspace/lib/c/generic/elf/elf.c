/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

#include <assert.h>
#include <elf/elf.h>
#include <stdbool.h>
#include <stdlib.h>

/** @addtogroup libc
 * @{
 */

/**
 * Checks that the ELF header is valid for the running system.
 */
static bool elf_is_valid(const elf_header_t *header)
{
	// TODO: check more things
	// TODO: debug output

	if (header->e_ident[EI_MAG0] != ELFMAG0 ||
	    header->e_ident[EI_MAG1] != ELFMAG1 ||
	    header->e_ident[EI_MAG2] != ELFMAG2 ||
	    header->e_ident[EI_MAG3] != ELFMAG3) {
		return false;
	}

	if (header->e_ident[EI_DATA] != ELF_DATA_ENCODING ||
	    header->e_machine != ELF_MACHINE ||
	    header->e_ident[EI_VERSION] != EV_CURRENT ||
	    header->e_version != EV_CURRENT ||
	    header->e_ident[EI_CLASS] != ELF_CLASS) {
		return false;
	}

	if (header->e_phentsize != sizeof(elf_segment_header_t)) {
		return false;
	}

	if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
		return false;
	}

	return true;
}

/**
 * Given the base of an ELF image in memory (i.e. pointer to the file
 * header at the beginning of the text segment), returns pointer to the
 * first segment header with the given p_type.
 */
const elf_segment_header_t *elf_get_phdr(const void *base, unsigned p_type)
{
	const elf_header_t *hdr = base;
	assert(elf_is_valid(hdr));

	const elf_segment_header_t *phdr =
	    (const elf_segment_header_t *) ((uintptr_t) base + hdr->e_phoff);

	for (int i = 0; i < hdr->e_phnum; i++) {
		if (phdr[i].p_type == p_type)
			return &phdr[i];
	}

	return NULL;
}

uintptr_t elf_get_bias(const void *base)
{
	const elf_header_t *hdr = base;
	assert(elf_is_valid(hdr));

	/*
	 * There are two legal options for a HelenOS ELF file.
	 * Either the file is ET_DYN (shared library or PIE), and the base is
	 * (required to be) at vaddr 0. Or the file is ET_EXEC (non-relocatable)
	 * and the bias is trivially 0.
	 */

	if (hdr->e_type == ET_DYN)
		return (uintptr_t) base;

	return 0;
}

/** @}
 */
