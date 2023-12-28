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

#include "util.h"

#include <stdio.h>
#include <debug/sections.h>
#include <debug/names.h>

bool skip_data(unsigned form, const uint8_t **const data,
    const uint8_t *data_end, unsigned width)
{
	// Skip data we don't care about reading.

	size_t len;

	switch (form) {
	case DW_FORM_string:
		skip_string(data, data_end);
		break;

	case DW_FORM_strp:
	case DW_FORM_line_strp:
	case DW_FORM_strp_sup:
	case DW_FORM_sec_offset:
		safe_increment(data, data_end, width);
		break;

	case DW_FORM_strx:
	case DW_FORM_sdata:
	case DW_FORM_udata:
		skip_leb128(data, data_end);
		break;

	case DW_FORM_strx1:
	case DW_FORM_data1:
	case DW_FORM_flag:
		safe_increment(data, data_end, 1);
		break;

	case DW_FORM_strx2:
	case DW_FORM_data2:
		safe_increment(data, data_end, 2);
		break;

	case DW_FORM_strx3:
		safe_increment(data, data_end, 3);
		break;

	case DW_FORM_strx4:
	case DW_FORM_data4:
		safe_increment(data, data_end, 4);
		break;

	case DW_FORM_data8:
		safe_increment(data, data_end, 8);
		break;

	case DW_FORM_data16:
		safe_increment(data, data_end, 16);
		break;

	case DW_FORM_block:
		len = read_uleb128(data, data_end);
		safe_increment(data, data_end, len);
		break;

	case DW_FORM_block1:
		len = read_byte(data, data_end);
		safe_increment(data, data_end, len);
		break;

	case DW_FORM_block2:
		len = read_uint16(data, data_end);
		safe_increment(data, data_end, len);
		break;

	case DW_FORM_block4:
		len = read_uint32(data, data_end);
		safe_increment(data, data_end, len);
		break;

	default:
		// Unknown FORM.
		*data = data_end;
		return false;
	}

	return true;
}

void print_format(const char *name, const uint8_t *format, const uint8_t *format_end)
{
	DEBUGF("%s: ", name);

	while (format < format_end) {
		uint64_t lnct = read_uleb128(&format, format_end);
		uint64_t form = read_uleb128(&format, format_end);

		const char *fname = dw_form_name(form);

		if (fname)
			DEBUGF("%s:%s, ", dw_lnct_name(lnct), fname);
		else
			DEBUGF("%s:unknown DW_FORM_(%" PRIu64 "), ", dw_lnct_name(lnct), form);
	}

	DEBUGF("\n");
}

void print_formatted_list(debug_sections_t *scs, const char *name,
    const uint8_t *data, const uint8_t *const data_end,
    const uint8_t *format, const uint8_t *format_end,
    unsigned width)
{
	DEBUGF("%s: ", name);

	while (data < data_end) {
		const uint8_t *old_data = data;
		const uint8_t *format_ptr = format;

		while (format_ptr < format_end) {
			uint64_t lnct = read_uleb128(&format_ptr, format_end);
			uint64_t form = read_uleb128(&format_ptr, format_end);

			DEBUGF("%s:%s:", dw_lnct_name(lnct), dw_form_name(form));
			print_formed_data(scs, form, &data, data_end, width);
			DEBUGF("\n");
		}

		if (data <= old_data)
			break;
	}

	DEBUGF("\n");
}

void print_block(const uint8_t **const data,
    const uint8_t *data_end, unsigned bytes)
{
	while (bytes > 0 && *data < data_end) {
		DEBUGF("%02x ", **data);
		(*data)++;
		bytes--;
	}
}

void print_formed_data(debug_sections_t *scs, unsigned form, const uint8_t **const data,
    const uint8_t *data_end, unsigned width)
{
	size_t len;
	uint64_t offset;

	switch (form) {
	case DW_FORM_string:
		DEBUGF("\"%s\"", read_string(data, data_end));
		break;

	case DW_FORM_strp:
	case DW_FORM_strp_sup:
		offset = read_uint(data, data_end, width);
		if (offset >= scs->debug_str_size)
			DEBUGF("<out of range>");
		else
			DEBUGF("\"%s\"", scs->debug_str + offset);
		break;

	case DW_FORM_line_strp:
		offset = read_uint(data, data_end, width);
		if (offset >= scs->debug_line_str_size)
			DEBUGF("<out of range>");
		else
			DEBUGF("\"%s\"", scs->debug_line_str + offset);
		break;

	case DW_FORM_sec_offset:
		if (width == 4)
			DEBUGF("0x%08" PRIx64, read_uint(data, data_end, width));
		else
			DEBUGF("0x%016" PRIx64, read_uint(data, data_end, width));
		break;

	case DW_FORM_strx:
	case DW_FORM_udata:
		DEBUGF("%" PRIu64, read_uleb128(data, data_end));
		break;

	case DW_FORM_sdata:
		DEBUGF("%" PRId64, read_sleb128(data, data_end));
		break;

	case DW_FORM_strx1:
	case DW_FORM_data1:
	case DW_FORM_flag:
		DEBUGF("%u", read_byte(data, data_end));
		break;

	case DW_FORM_strx2:
	case DW_FORM_data2:
		DEBUGF("%u", read_uint16(data, data_end));
		break;

	case DW_FORM_strx3:
		DEBUGF("%u", read_uint24(data, data_end));
		break;

	case DW_FORM_strx4:
	case DW_FORM_data4:
		DEBUGF("%u", read_uint32(data, data_end));
		break;

	case DW_FORM_data8:
		DEBUGF("%" PRIu64, read_uint64(data, data_end));
		break;

	case DW_FORM_data16:
		uint64_t data1 = read_uint64(data, data_end);
		uint64_t data2 = read_uint64(data, data_end);
		DEBUGF("0x%016" PRIx64 "%016" PRIx64, data2, data1);
		break;

	case DW_FORM_block:
		len = read_uleb128(data, data_end);
		print_block(data, data_end, len);
		break;

	case DW_FORM_block1:
		len = read_byte(data, data_end);
		print_block(data, data_end, len);
		break;

	case DW_FORM_block2:
		len = read_uint16(data, data_end);
		print_block(data, data_end, len);
		break;

	case DW_FORM_block4:
		len = read_uint32(data, data_end);
		print_block(data, data_end, len);
		break;

	default:
		DEBUGF("unexpected form");
		*data = data_end;
	}
}
