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

#ifndef DWARFS_LINE_H_
#define DWARFS_LINE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <debug/sections.h>

struct debug_line_program_header {
	uint64_t unit_length;
	uint64_t header_length;
	const uint8_t *unit_end;
	const uint8_t *header_end;
	const uint8_t *standard_opcode_lengths;
	size_t standard_opcode_lengths_size;
	unsigned width;
	uint16_t version;
	uint8_t minimum_instruction_length;
	bool default_is_stmt;
	int8_t line_base;
	uint8_t line_range;
	uint8_t opcode_base;

	union {
		struct {
			const uint8_t *include_directories;
			const uint8_t *include_directories_end;
			const uint8_t *file_names;
		} v3;
		struct {
			uint64_t directories_count;
			uint64_t file_names_count;
			const uint8_t *directory_entry_format;
			const uint8_t *directory_entry_format_end;
			const uint8_t *directories;
			const uint8_t *directories_end;
			const uint8_t *file_name_entry_format;
			const uint8_t *file_name_entry_format_end;
			const uint8_t *file_names;
			const uint8_t *file_names_end;
			uint8_t address_size;
			uint8_t segment_selector_size;
			uint8_t directory_entry_format_count;
			uint8_t file_name_entry_format_count;
			uint8_t maximum_operations_per_instruction;
		} v5;
	};
};

struct debug_line_program {
	const struct debug_line_program_header *hdr;
	const uint8_t *program;
	const uint8_t *program_end;

	uintptr_t address;
	int op_advance;
	int file;
	int line;
	int column;

	bool end_sequence;
	bool truncated;
};

static inline struct debug_line_program debug_line_program_create(const uint8_t *program,
    const uint8_t *const program_end,
    const struct debug_line_program_header *hdr)
{
	return (struct debug_line_program) {
		.hdr = hdr,
		.program = program,
		.program_end = program_end,
		.end_sequence = true,
		.truncated = false,
	};
}

extern bool debug_line_get_address_info(debug_sections_t *scs, uintptr_t addr, int op_index, const char **file, const char **dir, int *line, int *col);

#endif /* DWARFS_LINE_H_ */
