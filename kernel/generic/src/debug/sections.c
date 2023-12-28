/*
 * Copyright (c) 2023 Jiří Zárevúcky
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

#include <debug/sections.h>
#include <debug/register.h>
#include <debug.h>

#include <stdio.h>
#include <str.h>

#define DEBUGF dummy_printf

debug_sections_t kernel_sections;

struct section_manifest {
	const char *name;
	const void **address_field;
	size_t *size_field;
};

static void _check_string_section(const char *section, size_t *section_size)
{
	while (*section_size > 0 && section[*section_size - 1] != 0) {
		(*section_size)--;
	}
}

debug_sections_t get_debug_sections(const void *elf, size_t elf_size)
{
	debug_sections_t out = { };

#define section(name) { "." #name, (const void **)&out.name, &out.name##_size }

	const struct section_manifest manifest[] = {
		section(debug_aranges),
		section(debug_info),
		section(debug_abbrev),
		section(debug_line),
		section(debug_str),
		section(debug_line_str),
		section(debug_rnglists),
		section(eh_frame_hdr),
		section(eh_frame),
		section(symtab),
		section(strtab),
	};

	const size_t manifest_len = sizeof(manifest) / sizeof(struct section_manifest);

	const void *data_end = elf + elf_size;

	/*
	 * While this data is technically "trusted", insofar as it is provided
	 * by the bootloader, it's not critical, so we try to make sure malformed or
	 * misconfigured debug data doesn't crash the kernel.
	 */

	if (elf_size < sizeof(elf_header_t)) {
		printf("bad debug data: too short\n");
		return out;
	}

	if (((uintptr_t) elf) % 8 != 0) {
		printf("bad debug data: unaligned input\n");
		return out;
	}

	const elf_header_t *hdr = elf;

	if (hdr->e_ident[0] != ELFMAG0 || hdr->e_ident[1] != ELFMAG1 ||
	    hdr->e_ident[2] != ELFMAG2 || hdr->e_ident[3] != ELFMAG3) {
		printf("bad debug data: wrong ELF magic bytes\n");
		return out;
	}

	if (hdr->e_shentsize != sizeof(elf_section_header_t)) {
		printf("bad debug data: wrong e_shentsize\n");
		return out;
	}

	/* Get section header table. */
	const elf_section_header_t *shdr = elf + hdr->e_shoff;
	size_t shdr_len = hdr->e_shnum;

	if ((void *) &shdr[shdr_len] > data_end) {
		printf("bad debug data: truncated section header table\n");
		return out;
	}

	if (hdr->e_shstrndx >= shdr_len) {
		printf("bad debug data: string table index out of range\n");
		return out;
	}

	/* Get data on section name string table. */
	const elf_section_header_t *shstr = &shdr[hdr->e_shstrndx];
	const char *shstrtab = elf + shstr->sh_offset;
	size_t shstrtab_size = shstr->sh_size;

	if ((void *) shstrtab + shstrtab_size > data_end) {
		printf("bad debug data: truncated string table\n");
		return out;
	}

	/* Check NUL-terminator. */
	_check_string_section(shstrtab, &shstrtab_size);

	if (shstrtab_size == 0) {
		printf("bad debug data: empty or non-null-terminated string table\n");
		return out;
	}

	/* List all present sections. */
	for (size_t i = 0; i < shdr_len; i++) {
		if (shdr[i].sh_type == SHT_NULL || shdr[i].sh_type == SHT_NOBITS)
			continue;

		if (shdr[i].sh_name >= shstrtab_size) {
			printf("bad debug data: string table index out of range\n");
			continue;
		}

		const char *name = shstrtab + shdr[i].sh_name;
		size_t offset = shdr[i].sh_offset;
		size_t size = shdr[i].sh_size;

		DEBUGF("section '%s': offset %zd (%zd bytes)\n", name, offset, size);

		if (elf + offset + size > data_end) {
			printf("bad debug data: truncated section %s\n", name);
			continue;
		}

		for (size_t i = 0; i < manifest_len; i++) {
			if (str_cmp(manifest[i].name, name) == 0) {
				*manifest[i].address_field = elf + offset;
				*manifest[i].size_field = size;
				break;
			}
		}
	}

	/* Check NUL-terminator in .strtab, .debug_str and .debug_line_str */
	_check_string_section(out.strtab, &out.strtab_size);
	_check_string_section(out.debug_str, &out.debug_str_size);
	_check_string_section(out.debug_line_str, &out.debug_line_str_size);

	return out;
}

void register_debug_data(const void *elf, size_t elf_size)
{
	kernel_sections = get_debug_sections(elf, elf_size);
}
