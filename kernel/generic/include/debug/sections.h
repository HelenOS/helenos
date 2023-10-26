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

extern const void *debug_aranges;
extern size_t debug_aranges_size;

extern const void *debug_info;
extern size_t debug_info_size;

extern const void *debug_abbrev;
extern size_t debug_abbrev_size;

extern const void *debug_line;
extern size_t debug_line_size;

extern const char *debug_str;
extern size_t debug_str_size;

extern const char *debug_line_str;
extern size_t debug_line_str_size;

extern const void *debug_rnglists;
extern size_t debug_rnglists_size;

extern const void *eh_frame_hdr;
extern size_t eh_frame_hdr_size;

extern const void *eh_frame;
extern size_t eh_frame_size;

extern const elf_symbol_t *symtab;
extern size_t symtab_size;

extern const char *strtab;
extern size_t strtab_size;

#endif /* DEBUG_SECTIONS_H_ */
