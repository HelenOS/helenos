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

#ifndef DEBUG_UTIL_H_
#define DEBUG_UTIL_H_

#include <assert.h>
#include <stdint.h>
#include <debug/constants.h>
#include <debug/sections.h>
#include <debug.h>

#define DEBUGF dummy_printf

extern bool skip_data(unsigned, const uint8_t **const, const uint8_t *, unsigned);
extern void print_format(const char *, const uint8_t *, const uint8_t *);
extern void print_formatted_list(debug_sections_t *, const char *,
    const uint8_t *, const uint8_t *, const uint8_t *, const uint8_t *, unsigned);

extern void print_block(const uint8_t **, const uint8_t *, unsigned);
extern void print_formed_data(debug_sections_t *scs, unsigned, const uint8_t **const, const uint8_t *, unsigned);

static inline uint8_t read_byte(const uint8_t **data, const uint8_t *data_end)
{
	if (*data >= data_end) {
		return 0;
	} else {
		return *((*data)++);
	}
}

/* Casting to these structures allows us to read unaligned integers safely. */
struct u16 {
	uint16_t val;
} __attribute__((packed));

struct u32 {
	uint32_t val;
} __attribute__((packed));

struct u64 {
	uint64_t val;
} __attribute__((packed));

static inline uint16_t read_uint16(const uint8_t **data, const uint8_t *data_end)
{
	if (*data + 2 > data_end) {
		/* Safe exit path for malformed input. */
		*data = data_end;
		return 0;
	}

	uint16_t v = ((struct u16 *) *data)->val;
	*data += 2;
	return v;
}

static inline uint32_t read_uint24(const uint8_t **data, const uint8_t *data_end)
{
	if (*data + 3 > data_end) {
		/* Safe exit path for malformed input. */
		*data = data_end;
		return 0;
	}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint32_t v = (*data)[0] | (*data)[1] << 8 | (*data)[2] << 16;
#else
	uint32_t v = (*data)[2] | (*data)[1] << 8 | (*data)[0] << 16;
#endif

	*data += 3;
	return v;
}

static inline uint32_t read_uint32(const uint8_t **data, const uint8_t *data_end)
{
	if (*data + 4 > data_end) {
		/* Safe exit path for malformed input. */
		*data = data_end;
		return 0;
	}

	uint32_t v = ((struct u32 *) *data)->val;
	*data += 4;
	return v;
}

static inline uint64_t read_uint64(const uint8_t **data, const uint8_t *data_end)
{
	if (*data + 8 > data_end) {
		/* Safe exit path for malformed input. */
		*data = data_end;
		return 0;
	}

	uint64_t v = ((struct u64 *) *data)->val;
	*data += 8;
	return v;
}

static inline uint64_t read_uint(const uint8_t **data, const uint8_t *data_end, unsigned bytes)
{
	switch (bytes) {
	case 1:
		return read_byte(data, data_end);
	case 2:
		return read_uint16(data, data_end);
	case 4:
		return read_uint32(data, data_end);
	case 8:
		return read_uint64(data, data_end);
	default:
		panic("unimplemented");
	}
}

static inline uint64_t read_uleb128(const uint8_t **data, const uint8_t *data_end)
{
	uint64_t result = 0;
	unsigned shift = 0;

	while (*data < data_end) {
		uint8_t byte = *((*data)++);
		result |= (byte & 0x7f) << shift;
		shift += 7;

		if ((byte & 0x80) == 0)
			break;
	}

	return result;
}

static inline int64_t read_sleb128(const uint8_t **data, const uint8_t *data_end)
{
	uint64_t result = 0;
	unsigned shift = 0;

	while (*data < data_end) {
		uint8_t byte = *((*data)++);
		result |= (byte & 0x7f) << shift;
		shift += 7;

		if ((byte & 0x80) == 0) {
			if (shift < 64 && (byte & 0x40) != 0) {
				/* sign extend */
				result |= -(1 << shift);
			}
			break;
		}
	}

	return (int64_t) result;
}

static inline void skip_leb128(const uint8_t **data, const uint8_t *data_end)
{
	while (*data < data_end) {
		uint8_t byte = *((*data)++);
		if ((byte & 0x80) == 0)
			break;
	}
}

static inline uint64_t read_initial_length(const uint8_t **data, const uint8_t *data_end, unsigned *width)
{
	uint32_t initial = read_uint32(data, data_end);
	if (initial == 0xffffffffu) {
		*width = 8;
		return read_uint64(data, data_end);
	} else {
		*width = 4;
		return initial;
	}
}

static inline const char *read_string(const uint8_t **data, const uint8_t *data_end)
{
	const char *start = (const char *) *data;

	// NUL-terminated string.
	while (*data < data_end && **data != 0)
		(*data)++;

	if (*data < data_end) {
		// Skip the terminating zero.
		(*data)++;
		return start;
	} else {
		// No terminating zero, we can't use this.
		return NULL;
	}
}

static inline void skip_string(const uint8_t **data, const uint8_t *data_end)
{
	(void) read_string(data, data_end);
}

static inline void safe_increment(const uint8_t **data,
    const uint8_t *data_end, ptrdiff_t increment)
{
	assert(data_end >= *data);

	if (increment >= data_end - *data) {
		*data = data_end;
	} else {
		(*data) += increment;
	}
}

static inline void skip_format(const uint8_t **data, const uint8_t *const data_end,
    unsigned count)
{
	for (unsigned i = 0; i < count; i++) {
		(void) read_uleb128(data, data_end);
		(void) read_uleb128(data, data_end);
	}
}

static inline void skip_formatted_entry(const uint8_t **data, const uint8_t *const data_end,
    const uint8_t *format, const uint8_t *format_end, unsigned width)
{
	while (format < format_end) {
		/* Ignore content type code */
		(void) read_uleb128(&format, format_end);

		uint64_t form = read_uleb128(&format, format_end);
		skip_data(form, data, data_end, width);
	}
}

static inline void skip_formatted_list(const uint8_t **data, const uint8_t *const data_end,
    unsigned count,	const uint8_t *format, const uint8_t *format_end,
    unsigned width)
{
	for (unsigned i = 0; i < count; i++) {
		skip_formatted_entry(data, data_end, format, format_end, width);
	}
}

#endif /* DEBUG_UTIL_H_ */
