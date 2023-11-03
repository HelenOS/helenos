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

#include <stdatomic.h>

#include <stdlib.h>

#include <debug/constants.h>
#include <debug/line.h>
#include <debug/names.h>
#include <debug/sections.h>

#include "util.h"

static inline void line_program_reset(struct debug_line_program *lp)
{
	lp->address = 0;
	lp->op_advance = 0;
	lp->file = 1;
	lp->line = 1;
	lp->column = 0;
	lp->end_sequence = false;
}

/**
 * Quickly advance program just past the next sequence end, without processing
 * anything on the way. This is useful for when we just want to get start
 * addresses of every range to build a table.
 *
 * @param lp
 */
static void debug_line_program_skip_to_sequence_end(struct debug_line_program *lp)
{
	const uint8_t opcode_base = lp->hdr->opcode_base;

	const uint8_t *program = lp->program;
	const uint8_t *const program_end = lp->program_end;

	if (lp->end_sequence)
		line_program_reset(lp);

	while (program < program_end) {
		uint8_t opcode = read_byte(&program, program_end);

		if (opcode >= opcode_base) {
			// Special opcode.
			continue;
		}

		switch (opcode) {
		case DW_LNS_fixed_advance_pc:
			safe_increment(&program, program_end, 2);
			break;

		case DW_LNS_copy:
		case DW_LNS_negate_stmt:
		case DW_LNS_set_basic_block:
		case DW_LNS_set_prologue_end:
		case DW_LNS_set_epilogue_begin:
		case DW_LNS_const_add_pc:
			break;

		case DW_LNS_advance_pc:
		case DW_LNS_advance_line:
		case DW_LNS_set_file:
		case DW_LNS_set_column:
		case DW_LNS_set_isa:
			skip_leb128(&program, program_end);
			break;

		case 0:
			// Extended opcodes.
			size_t length = read_uleb128(&program, program_end);

			if (read_byte(&program, program_end) == DW_LNE_end_sequence) {
				lp->program = program;
				lp->end_sequence = true;
				return;
			}

			safe_increment(&program, program_end, length - 1);
			break;

		default:
			// Unknown standard opcode.

			// If the opcode length array is truncated, there is already
			// something wrong with the parse, so we don't care if we misparse
			// the remainder.
			if (opcode < lp->hdr->standard_opcode_lengths_size) {
				int len = lp->hdr->standard_opcode_lengths[opcode];
				for (int i = 0; i < len; i++) {
					// Drop arguments.
					skip_leb128(&program, program_end);
				}
			}
		}
	}

	lp->program = program;
	lp->truncated = true;
}

static void transfer_op_advance(struct debug_line_program *lp)
{
	if (lp->hdr->version < 5) {
		lp->address += lp->op_advance * lp->hdr->minimum_instruction_length;
		lp->op_advance = 0;
	} else {
		int maxops = lp->hdr->v5.maximum_operations_per_instruction;

		// For branch prediction, tell GCC maxops is likely to be 1, because
		// I don't want to punish sensible architectures for VLIW madness.
		if (__builtin_expect(maxops == 1, 1)) {
			lp->address += lp->op_advance * lp->hdr->minimum_instruction_length;
			lp->op_advance = 0;
		} else {
			int adv = lp->op_advance;
			int d = adv / maxops;
			int r = adv % maxops;

			lp->address += d * lp->hdr->minimum_instruction_length;
			lp->op_advance = r;
		}
	}
}

static void debug_line_program_advance(struct debug_line_program *lp)
{
	const uint8_t opcode_base = lp->hdr->opcode_base;
	const int8_t line_base = lp->hdr->line_base;
	const uint8_t line_range = lp->hdr->line_range;
	const int const_advance = (255 - opcode_base) / lp->hdr->line_range;

	const uint8_t *program = lp->program;
	const uint8_t *const program_end = lp->program_end;

	if (lp->end_sequence)
		line_program_reset(lp);

	while (program < program_end) {
		uint8_t opcode = read_byte(&program, program_end);

		if (opcode >= opcode_base) {
			// Special opcode.
			int adjusted = opcode - opcode_base;
			DEBUGF("DW_LNS_special(%d)\n", adjusted);

			lp->op_advance += adjusted / line_range;
			lp->line += line_base + (adjusted % line_range);

			transfer_op_advance(lp);
			lp->program = program;
			return;
		}

		const char *opname = dw_lns_name(opcode);

		uint64_t arg;
		uint16_t arg16;

		switch (opcode) {
		case DW_LNS_copy:
			DEBUGF("%s()\n", opname);

			transfer_op_advance(lp);
			lp->program = program;
			return;

		case DW_LNS_advance_pc:
			arg = read_uleb128(&program, program_end);
			DEBUGF("%s(%" PRIu64 ")\n", opname, arg);
			lp->op_advance += arg;
			break;

		case DW_LNS_advance_line:
			lp->line += read_sleb128(&program, program_end);
			DEBUGF("%s(line = %d)\n", opname, lp->line);
			break;

		case DW_LNS_set_file:
			lp->file = read_uleb128(&program, program_end);
			DEBUGF("%s(%d)\n", opname, lp->file);
			break;

		case DW_LNS_set_column:
			lp->column = read_uleb128(&program, program_end);
			DEBUGF("%s(%d)\n", opname, lp->column);
			break;

		case DW_LNS_negate_stmt:
		case DW_LNS_set_basic_block:
		case DW_LNS_set_prologue_end:
		case DW_LNS_set_epilogue_begin:
			DEBUGF("%s()\n", opname);
			// ignore
			break;

		case DW_LNS_set_isa:
			// discard argument and ignore
			arg = read_uleb128(&program, program_end);
			DEBUGF("%s(%" PRIu64 ")\n", opname, arg);
			break;

		case DW_LNS_const_add_pc:
			DEBUGF("%s(%d)\n", opname, const_advance);
			lp->op_advance += const_advance;
			break;

		case DW_LNS_fixed_advance_pc:
			arg16 = read_uint16(&program, program_end);
			DEBUGF("%s(%u)\n", opname, arg16);

			transfer_op_advance(lp);
			lp->address += arg16;
			lp->op_advance = 0;
			break;

		case 0:
			/* Extended opcodes. */
			size_t length = read_uleb128(&program, program_end);

			opcode = read_byte(&program, program_end);
			const char *opname = dw_lne_name(opcode);

			switch (opcode) {
			case DW_LNE_end_sequence:
				DEBUGF("%s:%zu()\n", opname, length);

				transfer_op_advance(lp);
				lp->program = program;
				lp->end_sequence = true;
				return;

			case DW_LNE_set_address:
				lp->address = read_uint(&program, program_end, sizeof(uintptr_t));
				lp->op_advance = 0;

				DEBUGF("%s:%zu(0x%zx)\n", opname, length, lp->address);
				break;

			case DW_LNE_set_discriminator:
				uint64_t arg = read_uleb128(&program, program_end);
				DEBUGF("%s:%zu(%" PRIu64 ")\n", opname, length, arg);
				break;

			default:
				DEBUGF("unknown extended opcode %d:%zu\n", opcode, length);
				safe_increment(&program, program_end, length - 1);
			}
			break;

		default:
			DEBUGF("unknown standard opcode %d\n", opcode);

			// If the opcode length array is truncated, there is already
			// something wrong with the parse, so we don't care if we misparse
			// the remainder.
			if (opcode < lp->hdr->standard_opcode_lengths_size) {
				int len = lp->hdr->standard_opcode_lengths[opcode];
				for (int i = 0; i < len; i++) {
					// Drop arguments.
					skip_leb128(&program, program_end);
				}
			}
		}
	}

	transfer_op_advance(lp);
	lp->program = program;
	lp->truncated = true;
}

static void debug_line_program_header_parse(debug_sections_t *scs, const uint8_t *data, const uint8_t *data_end, struct debug_line_program_header *hdr)
{
	const uint8_t *unit_start = data;

#define FIELD(name, spec, expr) (hdr->name = expr, DEBUGF(#name ": %" spec "\n", hdr->name))

	unsigned width;

	FIELD(unit_length, PRId64, read_initial_length(&data, data_end, &width));
	FIELD(width, "d", width);

	hdr->unit_end = data;
	safe_increment(&hdr->unit_end, data_end, hdr->unit_length);
	DEBUGF("unit size: %zu\n", hdr->unit_end - unit_start);

	data_end = hdr->unit_end;

	FIELD(version, "u", read_uint16(&data, data_end));

	if (hdr->version < 3 || hdr->version > 5) {
		// Not supported.
		hdr->header_end = hdr->unit_end;
		return;
	}

	if (hdr->version >= 5) {
		FIELD(v5.address_size, "u", read_byte(&data, data_end));
		FIELD(v5.segment_selector_size, "u", read_byte(&data, data_end));
	}

	FIELD(header_length, PRIu64, read_uint(&data, data_end, width));

	hdr->header_end = data;
	safe_increment(&hdr->header_end, data_end, hdr->header_length);

	data_end = hdr->header_end;

	FIELD(minimum_instruction_length, "u", read_byte(&data, data_end));
	if (hdr->version >= 5)
		FIELD(v5.maximum_operations_per_instruction, "u", read_byte(&data, data_end));
	FIELD(default_is_stmt, "u", read_byte(&data, data_end));
	FIELD(line_base, "d", (int8_t) read_byte(&data, data_end));
	FIELD(line_range, "u", read_byte(&data, data_end));
	FIELD(opcode_base, "u", read_byte(&data, data_end));

	hdr->standard_opcode_lengths = data;
	safe_increment(&data, data_end, hdr->opcode_base - 1);
	hdr->standard_opcode_lengths_size = data - hdr->standard_opcode_lengths;

	if (hdr->version < 5) {
		hdr->v3.include_directories = data;
		while (data < data_end && *data != 0)
			skip_string(&data, data_end);
		hdr->v3.include_directories_end = data;

		// Sanitize the list a little.
		while (hdr->v3.include_directories < hdr->v3.include_directories_end && hdr->v3.include_directories_end[-1] != 0)
			hdr->v3.include_directories_end--;

		safe_increment(&data, data_end, 1);

		hdr->v3.file_names = data;
	}

	if (hdr->version >= 5) {
		FIELD(v5.directory_entry_format_count, "u", read_byte(&data, data_end));

		hdr->v5.directory_entry_format = data;
		skip_format(&data, data_end, hdr->v5.directory_entry_format_count);
		hdr->v5.directory_entry_format_end = data;

		print_format("directory_entry_format",
		    hdr->v5.directory_entry_format, hdr->v5.directory_entry_format_end);

		FIELD(v5.directories_count, PRIu64, read_uleb128(&data, data_end));

		hdr->v5.directories = data;
		skip_formatted_list(&data, data_end, hdr->v5.directories_count,
		    hdr->v5.directory_entry_format, hdr->v5.directory_entry_format_end, width);
		hdr->v5.directories_end = data;

		print_formatted_list(scs, "directories", hdr->v5.directories, hdr->v5.directories_end,
		    hdr->v5.directory_entry_format, hdr->v5.directory_entry_format_end, width);

		FIELD(v5.file_name_entry_format_count, "u", read_byte(&data, data_end));

		hdr->v5.file_name_entry_format = data;
		skip_format(&data, data_end, hdr->v5.file_name_entry_format_count);
		hdr->v5.file_name_entry_format_end = data;

		print_format("file_name_entry_format",
		    hdr->v5.file_name_entry_format, hdr->v5.file_name_entry_format_end);

		FIELD(v5.file_names_count, PRIu64, read_uleb128(&data, data_end));

		hdr->v5.file_names = data;
		skip_formatted_list(&data, data_end, hdr->v5.file_names_count,
		    hdr->v5.file_name_entry_format, hdr->v5.file_name_entry_format_end, width);
		hdr->v5.file_names_end = data;

		print_formatted_list(scs, "file_names", hdr->v5.file_names, hdr->v5.file_names_end,
		    hdr->v5.file_name_entry_format, hdr->v5.file_name_entry_format_end, width);
	}
}

static bool has_usable_name(const uint8_t *format, const uint8_t *format_end, unsigned width)
{
	// Check if there is an appropriate entry we can use.
	bool has_usable_name = false;
	const uint8_t *dummy = NULL;

	while (format < format_end) {
		uint64_t type = read_uleb128(&format, format_end);
		uint64_t form = read_uleb128(&format, format_end);

		if (type == DW_LNCT_path) {
			if (form == DW_FORM_string || form == DW_FORM_line_strp) {
				has_usable_name = true;
			}
		}

		if (!skip_data(form, &dummy, NULL, width)) {
			// Encountered DW_FORM that we don't understand,
			// which means we can't skip it.
			return false;
		}
	}

	return has_usable_name;
}

static const char *get_file_name_v3(struct debug_line_program_header *hdr, int file, int *dir)
{
	if (file == 0) {
		// We'd have to find and read the compilation unit header for this one,
		// and we don't wanna.
		return NULL;
	}

	file--;

	const uint8_t *files = hdr->v3.file_names;
	const uint8_t *files_end = hdr->header_end;

	for (int i = 0; i < file; i++) {
		if (files >= files_end || *files == 0) {
			// End of list.
			return NULL;
		}

		// Skip an entry.
		skip_string(&files, files_end);
		skip_leb128(&files, files_end);
		skip_leb128(&files, files_end);
		skip_leb128(&files, files_end);
	}

	if (files >= files_end || *files == 0)
		return NULL;

	const char *fn = (const char *) files;
	skip_string(&files, files_end);
	*dir = read_uleb128(&files, files_end);
	return fn;
}

static const char *get_file_name_v5(debug_sections_t *scs, struct debug_line_program_header *hdr, int file, int *dir)
{
	// DWARF5 has dynamic layout for file information, which is why
	// this is so horrible to decode. Enjoy.

	if (!has_usable_name(hdr->v5.file_name_entry_format, hdr->v5.file_name_entry_format_end, hdr->width))
		return NULL;

	const uint8_t *fns = hdr->v5.file_names;
	const uint8_t *fns_end = hdr->v5.file_names_end;

	const char *filename = NULL;

	for (uint8_t i = 0; i < hdr->v5.file_names_count; i++) {

		const uint8_t *fnef = hdr->v5.file_name_entry_format;
		const uint8_t *fnef_end = hdr->v5.file_name_entry_format_end;

		for (uint8_t j = 0; j < hdr->v5.file_name_entry_format_count; j++) {
			uint64_t type = read_uleb128(&fnef, fnef_end);
			uint64_t form = read_uleb128(&fnef, fnef_end);

			if (i == file && type == DW_LNCT_path) {
				if (form == DW_FORM_string) {
					filename = read_string(&fns, fns_end);
					continue;
				}

				if (form == DW_FORM_line_strp) {
					uint64_t offset = read_uint(&fns, fns_end, hdr->width);
					if (offset < scs->debug_line_str_size) {
						filename = scs->debug_line_str + offset;
					}
					continue;
				}
			}

			if (i == file && type == DW_LNCT_directory_index) {
				if (form == DW_FORM_data1) {
					*dir = read_byte(&fns, fns_end);
					continue;
				}

				if (form == DW_FORM_data2) {
					*dir = read_uint16(&fns, fns_end);
					continue;
				}

				if (form == DW_FORM_udata) {
					*dir = read_uleb128(&fns, fns_end);
					continue;
				}
			}

			skip_data(form, &fns, fns_end, hdr->width);
		}

		if (i == file)
			break;
	}

	return filename;
}

static const char *get_file_name(debug_sections_t *scs, struct debug_line_program_header *hdr, int file, int *dir)
{
	switch (hdr->version) {
	case 3:
	case 4:
		return get_file_name_v3(hdr, file, dir);
	case 5:
		return get_file_name_v5(scs, hdr, file, dir);
	default:
		return NULL;
	}
}

static const char *get_dir_name_v3(struct debug_line_program_header *hdr, int dir)
{
	if (dir == 0)
		return ".";

	const uint8_t *dirs = hdr->v3.include_directories;
	const uint8_t *dirs_end = hdr->v3.include_directories_end;

	for (int i = 1; i < dir; i++) {
		skip_string(&dirs, dirs_end);
	}

	if (dirs < dirs_end)
		return (const char *) dirs;
	else
		return NULL;
}

static const char *get_dir_name_v5(debug_sections_t *scs, struct debug_line_program_header *hdr, int dir)
{
	// TODO: basically a copypaste of get_file_name(). Try to deduplicate it.

	if (!has_usable_name(hdr->v5.directory_entry_format, hdr->v5.directory_entry_format_end, hdr->width))
		return NULL;

	const uint8_t *fns = hdr->v5.directories;
	const uint8_t *fns_end = hdr->v5.directories_end;

	const char *filename = NULL;

	for (uint8_t i = 0; i < hdr->v5.directories_count; i++) {

		const uint8_t *fnef = hdr->v5.directory_entry_format;
		const uint8_t *fnef_end = hdr->v5.directory_entry_format_end;

		for (uint8_t j = 0; j < hdr->v5.directory_entry_format_count; j++) {
			uint64_t type = read_uleb128(&fnef, fnef_end);
			uint64_t form = read_uleb128(&fnef, fnef_end);

			if (i == dir && type == DW_LNCT_path) {
				if (form == DW_FORM_string) {
					filename = read_string(&fns, fns_end);
					continue;
				}

				if (form == DW_FORM_line_strp) {
					uint64_t offset = read_uint(&fns, fns_end, hdr->width);
					if (offset < scs->debug_line_str_size) {
						filename = scs->debug_line_str + offset;
					}
					continue;
				}
			}

			skip_data(form, &fns, fns_end, hdr->width);
		}

		if (i == dir)
			break;
	}

	return filename;
}

static const char *get_dir_name(debug_sections_t *scs, struct debug_line_program_header *hdr, int dir)
{
	switch (hdr->version) {
	case 3:
	case 4:
		return get_dir_name_v3(hdr, dir);
	case 5:
		return get_dir_name_v5(scs, hdr, dir);
	default:
		return NULL;
	}
}

static const uint8_t *find_line_program(debug_sections_t *scs, uintptr_t addr)
{
	// TODO: use .debug_aranges to find the data quickly
	// This implementation just iterates through the entire .debug_line

	uintptr_t closest_addr = 0;
	const uint8_t *closest_prog = NULL;

	const uint8_t *debug_line_ptr = scs->debug_line;
	const uint8_t *const debug_line_end = scs->debug_line + scs->debug_line_size;

	while (debug_line_ptr < debug_line_end) {
		const uint8_t *prog = debug_line_ptr;

		// Parse header
		struct debug_line_program_header hdr = { };
		debug_line_program_header_parse(scs, prog, debug_line_end, &hdr);
		assert(hdr.unit_end > debug_line_ptr);
		assert(hdr.unit_end <= debug_line_end);
		debug_line_ptr = hdr.unit_end;

		struct debug_line_program lp = debug_line_program_create(
		    hdr.header_end, hdr.unit_end, &hdr);

		while (lp.program < lp.program_end) {
			// Find the start address of every sequence

			debug_line_program_advance(&lp);
			DEBUGF("<< address: %zx, line: %d, column: %d >>\n", lp.address, lp.line, lp.column);

			if (!lp.truncated) {
				if (lp.address <= addr && lp.address > closest_addr) {
					closest_addr = lp.address;
					closest_prog = prog;
				}
			}

			if (!lp.end_sequence) {
				debug_line_program_skip_to_sequence_end(&lp);
				assert(lp.truncated || lp.end_sequence);
			}
		}
	}

	return closest_prog;
}

static bool get_info(const struct debug_line_program_header *hdr, uintptr_t addr, int op_index, int *file, int *line, int *column)
{
	struct debug_line_program lp = debug_line_program_create(
	    hdr->header_end, hdr->unit_end, hdr);

	int last_file = 0;
	int last_line = 0;
	int last_column = 0;

	while (lp.program < lp.program_end) {
		bool first = lp.end_sequence;

		debug_line_program_advance(&lp);

		if (lp.truncated)
			continue;

		/*
		 * Return previous entry when we pass the target address, because
		 * the address may not be aligned perfectly on instruction boundary.
		 */
		if (lp.address > addr || (lp.address == addr && lp.op_advance > op_index)) {
			if (first) {
				// First address is already too large, skip to the next sequence.
				debug_line_program_skip_to_sequence_end(&lp);
				continue;
			}

			*file = last_file;
			*line = last_line;
			*column = last_column;
			return true;
		}

		last_file = lp.file;
		last_line = lp.line;
		last_column = lp.column;
	}

	return false;
}

bool debug_line_get_address_info(debug_sections_t *scs, uintptr_t addr, int op_index, const char **file_name, const char **dir_name, int *line, int *column)
{
	const uint8_t *data = find_line_program(scs, addr);
	if (data == NULL) {
		return false;
	}

	const uint8_t *const debug_line_end = scs->debug_line + scs->debug_line_size;

	struct debug_line_program_header hdr = { };
	debug_line_program_header_parse(scs, data, debug_line_end, &hdr);
	assert(hdr.unit_end > data);
	assert(hdr.unit_end <= debug_line_end);

	int dir = -1;
	int file = -1;

	if (!get_info(&hdr, addr, op_index, &file, line, column)) {
		// printf("no info for 0x%zx: prog offset 0x%zx\n", addr, (void *) data - debug_line);
		return false;
	}

	// printf("got info for 0x%zx: file %d, line %d, column %d\n", addr, file, *line, *column);

	if (file >= 0)
		*file_name = get_file_name(scs, &hdr, file, &dir);

	if (dir >= 0)
		*dir_name = get_dir_name(scs, &hdr, dir);

	return true;
}
