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

#include <stdio.h>
#include <str.h>

const void *debug_aranges = NULL;
size_t debug_aranges_size = 0;

const void *debug_info = NULL;
size_t debug_info_size = 0;

const void *debug_abbrev = NULL;
size_t debug_abbrev_size = 0;

const void *debug_line = NULL;
size_t debug_line_size = 0;

const char *debug_str = NULL;
size_t debug_str_size = 0;

const char *debug_line_str = NULL;
size_t debug_line_str_size = 0;

const void *debug_rnglists = NULL;
size_t debug_rnglists_size = 0;

// TODO: get this from the program image
const void *eh_frame_hdr = NULL;
size_t eh_frame_hdr_size = 0;

const void *eh_frame = NULL;
size_t eh_frame_size = 0;

const elf_symbol_t *symtab = NULL;
size_t symtab_size = 0;

const char *strtab = NULL;
size_t strtab_size = 0;

struct section_manifest {
	const char *name;
	const void **address_field;
	size_t *size_field;
};

#define section(name) { "." #name, (const void **)&name, &name##_size }

static const struct section_manifest manifest[] = {
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

#undef section

static const size_t manifest_len = sizeof(manifest) / sizeof(struct section_manifest);

void register_debug_data(const void *data, size_t data_size)
{
	const void *data_end = data + data_size;

	/*
	 * While this data is technically "trusted", insofar as it is provided
	 * by the bootloader, it's not critical, so we try to make sure malformed or
	 * misconfigured debug data doesn't crash the kernel.
	 */

	if (data_size < sizeof(elf_header_t)) {
		printf("bad debug data: too short\n");
		return;
	}

	if (((uintptr_t) data) % 8 != 0) {
		printf("bad debug data: unaligned input\n");
		return;
	}

	const elf_header_t *hdr = data;

	if (hdr->e_ident[0] != ELFMAG0 || hdr->e_ident[1] != ELFMAG1 ||
	    hdr->e_ident[2] != ELFMAG2 || hdr->e_ident[3] != ELFMAG3) {
		printf("bad debug data: wrong ELF magic bytes\n");
		return;
	}

	if (hdr->e_shentsize != sizeof(elf_section_header_t)) {
		printf("bad debug data: wrong e_shentsize\n");
		return;
	}

	/* Get section header table. */
	const elf_section_header_t *shdr = data + hdr->e_shoff;
	size_t shdr_len = hdr->e_shnum;

	if ((void *) &shdr[shdr_len] > data_end) {
		printf("bad debug data: truncated section header table\n");
		return;
	}

	if (hdr->e_shstrndx >= shdr_len) {
		printf("bad debug data: string table index out of range\n");
		return;
	}

	/* Get data on section name string table. */
	const elf_section_header_t *shstr = &shdr[hdr->e_shstrndx];
	const char *shstrtab = data + shstr->sh_offset;
	size_t shstrtab_size = shstr->sh_size;

	if ((void *) shstrtab + shstrtab_size > data_end) {
		printf("bad debug data: truncated string table\n");
		return;
	}

	/* Check NUL-terminator. */
	while (shstrtab_size > 0 && shstrtab[shstrtab_size - 1] != 0) {
		shstrtab_size--;
	}

	if (shstrtab_size == 0) {
		printf("bad debug data: empty or non-null-terminated string table\n");
		return;
	}

	/* List all present sections. */
	for (size_t i = 0; i < shdr_len; i++) {
		if (shdr[i].sh_type == SHT_NULL || shdr[i].sh_type == SHT_NOBITS)
			continue;

		if (shdr[i].sh_name >= shstrtab_size) {
			printf("bad debug data: string table index out of range\n");
			return;
		}

		const char *name = shstrtab + shdr[i].sh_name;
		size_t offset = shdr[i].sh_offset;
		size_t size = shdr[i].sh_size;

		printf("section '%s': offset %zd (%zd bytes)\n", name, offset, size);

		if (data + offset + size > data_end) {
			printf("bad debug data: truncated section %s\n", name);
			continue;
		}

		for (size_t i = 0; i < manifest_len; i++) {
			if (str_cmp(manifest[i].name, name) == 0) {
				*manifest[i].address_field = data + offset;
				*manifest[i].size_field = size;
				break;
			}
		}
	}

	/* Check NUL-terminator in .strtab, .debug_str and .debug_line_str */
	while (strtab_size > 0 && strtab[strtab_size - 1] != 0) {
		strtab_size--;
	}

	while (debug_str_size > 0 && debug_str[debug_str_size - 1] != 0) {
		debug_str_size--;
	}

	while (debug_line_str_size > 0 && debug_str[debug_line_str_size - 1] != 0) {
		debug_line_str_size--;
	}
}
