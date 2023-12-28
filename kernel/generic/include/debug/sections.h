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

#ifndef DEBUG_SECTIONS_H_
#define DEBUG_SECTIONS_H_

#include <abi/elf.h>
#include <stddef.h>

typedef struct {
	const void *debug_aranges;
	size_t debug_aranges_size;

	const void *debug_info;
	size_t debug_info_size;

	const void *debug_abbrev;
	size_t debug_abbrev_size;

	const void *debug_line;
	size_t debug_line_size;

	const char *debug_str;
	size_t debug_str_size;

	const char *debug_line_str;
	size_t debug_line_str_size;

	const void *debug_rnglists;
	size_t debug_rnglists_size;

	const void *eh_frame_hdr;
	size_t eh_frame_hdr_size;

	const void *eh_frame;
	size_t eh_frame_size;

	const elf_symbol_t *symtab;
	size_t symtab_size;

	const char *strtab;
	size_t strtab_size;
} debug_sections_t;

extern debug_sections_t kernel_sections;

debug_sections_t get_debug_sections(const void *elf, size_t elf_size);

#endif /* DEBUG_SECTIONS_H_ */
