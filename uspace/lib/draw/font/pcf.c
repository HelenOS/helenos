/*
 * Copyright (c) 2014 Martin Sucha
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <byteorder.h>
#include <stdio.h>
#include <align.h>
#include <offset.h>
#include <stdlib.h>
#include <str.h>

#include "pcf.h"
#include "../drawctx.h"
#include "bitmap_backend.h"

#define PCF_TABLE_ACCELERATORS 0x02
#define PCF_TABLE_METRICS      0x04
#define PCF_TABLE_BITMAPS      0x08
#define PCF_TABLE_INK_METRICS  0x10
#define PCF_TABLE_ENCODINGS    0x20

#define PCF_FORMAT_DEFAULT            0x00000000
#define PCF_FORMAT_MASK               0xffffff00
#define PCF_FORMAT_MSBYTE_FIRST       0x00000004
#define PCF_FORMAT_MSBIT_FIRST        0x00000008
#define PCF_FORMAT_COMPRESSED_METRICS 0x00000100

typedef struct {
	uint32_t type;
	uint32_t format;
	uint32_t size; /* in bytes */
	uint32_t offset; /* in bytes from beginning of file */
} __attribute__((__packed__)) pcf_toc_entry_t;

typedef struct {
	uint16_t min_byte2;
	uint16_t max_byte2;
	uint16_t min_byte1;
	uint16_t max_byte1;
	uint16_t default_char;
} __attribute__((__packed__)) pcf_encoding_t;

typedef struct {
	uint8_t left_side_bearing;
	uint8_t right_side_bearing;
	uint8_t character_width;
	uint8_t character_ascent;
	uint8_t character_descent;
} __attribute__((__packed__)) pcf_compressed_metrics_t;

typedef struct {
	int16_t left_side_bearing;
	int16_t right_side_bearing;
	int16_t character_width;
	int16_t character_ascent;
	int16_t character_descent;
	uint16_t character_attributes;
} __attribute__((__packed__)) pcf_default_metrics_t;

typedef struct {
	uint8_t unused_font_information[8];
	int32_t font_ascent;
	int32_t font_descent;
} __attribute__((__packed__)) pcf_accelerators_t;

typedef struct {
	FILE *file;
	uint32_t glyph_count;
	pcf_toc_entry_t bitmap_table;
	pcf_toc_entry_t metrics_table;
	pcf_toc_entry_t encodings_table;
	pcf_toc_entry_t accelerators_table;
	pcf_encoding_t encoding;
	font_metrics_t font_metrics;
} pcf_data_t;

static inline uint32_t uint32_t_pcf2host(uint32_t val, uint32_t format)
{
	if (format & PCF_FORMAT_MSBYTE_FIRST) {
		return uint32_t_be2host(val);
	} else {
		return uint32_t_le2host(val);
	}
}

static inline uint16_t uint16_t_pcf2host(uint16_t val, uint32_t format)
{
	if (format & PCF_FORMAT_MSBYTE_FIRST) {
		return uint16_t_be2host(val);
	} else {
		return uint16_t_le2host(val);
	}
}

static inline int16_t int16_t_pcf2host(int16_t val, uint32_t format)
{
	return (int16_t) uint16_t_pcf2host((uint16_t) val, format);
}

static inline int32_t int32_t_pcf2host(int32_t val, uint32_t format)
{
	return (int32_t) uint32_t_pcf2host((uint32_t) val, format);
}


static int16_t compressed2int(uint8_t compressed)
{
	int16_t ret = compressed;
	ret -= 0x80;
	return ret;
}

static errno_t pcf_resolve_glyph(void *opaque_data, const wchar_t chr,
    glyph_id_t *glyph_id)
{
	pcf_data_t *data = (pcf_data_t *) opaque_data;

	/* TODO is this correct? */
	uint8_t byte1 = (chr >> 8) & 0xff;
	uint8_t byte2 = chr & 0xff;
	pcf_encoding_t *e = &data->encoding;

	aoff64_t entry_index =
	    (byte1 - e->min_byte1) * (e->max_byte2 - e->min_byte2 + 1) +
	    (byte2 - e->min_byte2);

	aoff64_t entry_offset = data->encodings_table.offset +
	    (sizeof(uint32_t) + 5 * sizeof(uint16_t)) +
	    entry_index * sizeof(uint16_t);

	int rc = fseek(data->file, entry_offset, SEEK_SET);
	if (rc != 0)
		return errno;

	uint16_t glyph = 0;
	size_t records_read = fread(&glyph, sizeof(uint16_t), 1, data->file);
	if (records_read != 1)
		return EINVAL;

	glyph = uint16_t_pcf2host(glyph, data->encodings_table.format);

	if (glyph == 0xffff)
		return ENOENT;

	*glyph_id = glyph;

	return EOK;
}

static errno_t load_glyph_metrics(pcf_data_t *data, uint32_t glyph_id,
    pcf_toc_entry_t *table, pcf_default_metrics_t *metrics)
{
	aoff64_t offset;
	int rc;
	size_t records_read;

	if (table->format & PCF_FORMAT_COMPRESSED_METRICS) {
		offset = table->offset + sizeof(uint32_t) + sizeof(uint16_t) +
		    glyph_id * sizeof(pcf_compressed_metrics_t);

		rc = fseek(data->file, offset, SEEK_SET);
		if (rc != 0)
			return errno;

		pcf_compressed_metrics_t compressed_metrics;
		records_read = fread(&compressed_metrics,
		    sizeof(pcf_compressed_metrics_t), 1, data->file);
		if (records_read != 1)
			return EINVAL;

		metrics->left_side_bearing =
		    compressed2int(compressed_metrics.left_side_bearing);
		metrics->right_side_bearing =
		    compressed2int(compressed_metrics.right_side_bearing);
		metrics->character_width =
		    compressed2int(compressed_metrics.character_width);
		metrics->character_ascent =
		    compressed2int(compressed_metrics.character_ascent);
		metrics->character_descent =
		    compressed2int(compressed_metrics.character_descent);
		metrics->character_attributes = 0;
	} else {
		offset = table->offset + 2 * sizeof(uint32_t) +
		    glyph_id * sizeof(pcf_default_metrics_t);

		rc = fseek(data->file, offset, SEEK_SET);
		if (rc != 0)
			return errno;

		pcf_default_metrics_t uncompressed_metrics;
		records_read = fread(&uncompressed_metrics,
		    sizeof(pcf_default_metrics_t), 1, data->file);
		if (records_read != 1)
			return EINVAL;

		metrics->left_side_bearing =
		    int16_t_pcf2host(uncompressed_metrics.left_side_bearing,
		    table->format);
		metrics->right_side_bearing =
		    int16_t_pcf2host(uncompressed_metrics.right_side_bearing,
		    table->format);
		metrics->character_width =
		    int16_t_pcf2host(uncompressed_metrics.character_width,
		    table->format);
		metrics->character_ascent =
		    int16_t_pcf2host(uncompressed_metrics.character_ascent,
		    table->format);
		metrics->character_descent =
		    int16_t_pcf2host(uncompressed_metrics.character_descent,
		    table->format);
		metrics->character_attributes =
		    uint16_t_pcf2host(uncompressed_metrics.character_attributes,
		    table->format);
	}

	return EOK;
}

static errno_t pcf_load_glyph_surface(void *opaque_data, glyph_id_t glyph_id,
    surface_t **out_surface)
{
	pcf_data_t *data = (pcf_data_t *) opaque_data;

	pcf_default_metrics_t pcf_metrics;
	memset(&pcf_metrics, 0, sizeof(pcf_default_metrics_t));
	errno_t rc = load_glyph_metrics(data, glyph_id, &data->metrics_table,
	    &pcf_metrics);
	if (rc != EOK)
		return rc;

	aoff64_t offset = data->bitmap_table.offset + (2 * sizeof(uint32_t)) +
	    (glyph_id * sizeof(uint32_t));

	if (fseek(data->file, offset, SEEK_SET) < 0)
		return errno;

	uint32_t bitmap_offset = 0;
	size_t records_read = fread(&bitmap_offset, sizeof(uint32_t), 1,
	    data->file);
	if (records_read != 1)
		return EINVAL;
	bitmap_offset = uint32_t_pcf2host(bitmap_offset,
	    data->bitmap_table.format);

	offset = data->bitmap_table.offset + (2 * sizeof(uint32_t)) +
	    (data->glyph_count * sizeof(uint32_t)) + (4 * sizeof(uint32_t)) +
	    bitmap_offset;

	if (fseek(data->file, offset, SEEK_SET) < 0)
		return errno;

	surface_coord_t width = pcf_metrics.character_width;
	surface_coord_t height = pcf_metrics.character_ascent +
	    pcf_metrics.character_descent;
	size_t row_padding_bytes = (1 << (data->bitmap_table.format & 3));
	size_t word_size_bytes = (1 << ((data->bitmap_table.format >> 4) & 3));
	size_t row_bytes = ALIGN_UP(ALIGN_UP(width, 8) / 8, row_padding_bytes);
	size_t bitmap_bytes = height * row_bytes;

	uint8_t *bitmap = malloc(bitmap_bytes);
	if (bitmap == NULL)
		return ENOMEM;

	records_read = fread(bitmap, sizeof(uint8_t), bitmap_bytes,
	    data->file);

	surface_t *surface = surface_create(width, height, NULL, 0);
	if (!surface) {
		free(bitmap);
		return ENOMEM;
	}

	for (unsigned int y = 0; y < height; ++y) {
		size_t row_offset = row_bytes * y;
		for (unsigned int x = 0; x < width; ++x) {
			size_t word_index = x / (word_size_bytes * 8);
			size_t column_offset1 = word_index * word_size_bytes;
			size_t byte_index_within_word =
			    (x % (word_size_bytes * 8)) / 8;
			size_t column_offset2;
			if (data->bitmap_table.format & PCF_FORMAT_MSBYTE_FIRST) {
				column_offset2 = (word_size_bytes - 1) - byte_index_within_word;
			} else {
				column_offset2 = byte_index_within_word;
			}
			uint8_t b = bitmap[row_offset + column_offset1 + column_offset2];
			bool set;
			if (data->bitmap_table.format & PCF_FORMAT_MSBIT_FIRST) {
				set = (b >> (7 - (x % 8))) & 1;
			} else {
				set = (b >> (x % 8)) & 1;
			}
			pixel_t p = set ? PIXEL(255, 0, 0, 0) : PIXEL(0, 0, 0, 0);
			surface_put_pixel(surface, x, y, p);
		}
	}

	*out_surface = surface;
	free(bitmap);
	return EOK;
}

static errno_t pcf_load_glyph_metrics(void *opaque_data, glyph_id_t glyph_id,
    glyph_metrics_t *gm)
{
	pcf_data_t *data = (pcf_data_t *) opaque_data;

	pcf_default_metrics_t pcf_metrics;
	memset(&pcf_metrics, 0, sizeof(pcf_default_metrics_t));
	errno_t rc = load_glyph_metrics(data, glyph_id, &data->metrics_table,
	    &pcf_metrics);
	if (rc != EOK)
		return rc;

	gm->left_side_bearing = pcf_metrics.left_side_bearing;
	gm->width = pcf_metrics.character_width;
	gm->right_side_bearing = pcf_metrics.right_side_bearing -
	    pcf_metrics.character_width;
	gm->height = pcf_metrics.character_descent +
	    pcf_metrics.character_ascent;
	gm->ascender = pcf_metrics.character_ascent;

	return EOK;
}

static void pcf_release(void *opaque_data)
{
	pcf_data_t *data = (pcf_data_t *) opaque_data;

	fclose(data->file);
	free(data);
}

bitmap_font_decoder_t fd_pcf = {
	.resolve_glyph = pcf_resolve_glyph,
	.load_glyph_surface = pcf_load_glyph_surface,
	.load_glyph_metrics = pcf_load_glyph_metrics,
	.release = pcf_release
};

static errno_t pcf_read_toc(pcf_data_t *data)
{
	int rc = fseek(data->file, 0, SEEK_END);
	if (rc != 0)
		return errno;

	aoff64_t file_size = ftell(data->file);

	rc = fseek(data->file, 0, SEEK_SET);
	if (rc != 0)
		return errno;

	char header[4];
	size_t records_read = fread(header, sizeof(char), 4, data->file);
	if (records_read != 4)
		return EINVAL;

	if (header[0] != 1 || header[1] != 'f' || header[2] != 'c' ||
	    header[3] != 'p')
		return EINVAL;

	uint32_t table_count;
	records_read = fread(&table_count, sizeof(uint32_t), 1,
	    data->file);
	if (records_read != 1)
		return EINVAL;

	table_count = uint32_t_le2host(table_count);

	bool found_bitmap_table = false;
	bool found_metrics_table = false;
	bool found_encodings_table = false;
	bool found_accelerators_table = false;

	for (uint32_t index = 0; index < table_count; index++) {
		pcf_toc_entry_t toc_entry;
		records_read = fread(&toc_entry, sizeof(pcf_toc_entry_t), 1,
		    data->file);
		toc_entry.type = uint32_t_le2host(toc_entry.type);
		toc_entry.format = uint32_t_le2host(toc_entry.format);
		toc_entry.size = uint32_t_le2host(toc_entry.size);
		toc_entry.offset = uint32_t_le2host(toc_entry.offset);

		if (toc_entry.offset >= file_size)
			continue;

		aoff64_t end = ((aoff64_t) toc_entry.offset) + ((aoff64_t) toc_entry.size);
		if (end > file_size)
			continue;

		if (toc_entry.type == PCF_TABLE_BITMAPS) {
			if (found_bitmap_table)
				return EINVAL;
			found_bitmap_table = true;
			data->bitmap_table = toc_entry;
		} else if (toc_entry.type == PCF_TABLE_METRICS) {
			if (found_metrics_table)
				return EINVAL;
			found_metrics_table = true;
			data->metrics_table = toc_entry;
		} else if (toc_entry.type == PCF_TABLE_ENCODINGS) {
			if (found_encodings_table)
				return EINVAL;
			found_encodings_table = true;
			data->encodings_table = toc_entry;
		} else if (toc_entry.type == PCF_TABLE_ACCELERATORS) {
			if (found_accelerators_table)
				return EINVAL;
			found_accelerators_table = true;
			data->accelerators_table = toc_entry;
		}
	}

	if (!found_bitmap_table || !found_metrics_table ||
	    !found_encodings_table || !found_accelerators_table)
		return EINVAL;

	return EOK;
}

static errno_t pcf_seek_table_header(pcf_data_t *data, pcf_toc_entry_t *table)
{
	uint32_t format;
	int rc = fseek(data->file, table->offset, SEEK_SET);
	if (rc != 0)
		return errno;

	size_t records_read = fread(&format, sizeof(uint32_t), 1, data->file);
	if (records_read != 1)
		return EINVAL;

	format = uint32_t_le2host(format);
	if (format != table->format)
		return EINVAL;

	return EOK;
}

static errno_t pcf_read_bitmap_table_header(pcf_data_t *data)
{
	errno_t rc = pcf_seek_table_header(data, &data->bitmap_table);
	if (rc != EOK)
		return rc;

	if ((data->bitmap_table.format & PCF_FORMAT_MASK) != PCF_FORMAT_DEFAULT)
		return EINVAL;

	uint32_t glyph_count = 0;
	size_t records_read = fread(&glyph_count, sizeof(uint32_t), 1,
	    data->file);
	if (records_read != 1)
		return EINVAL;
	glyph_count =  uint32_t_pcf2host(glyph_count, data->bitmap_table.format);

	data->glyph_count = glyph_count;
	return EOK;
}

static errno_t pcf_read_metrics_table_header(pcf_data_t *data)
{
	errno_t rc = pcf_seek_table_header(data, &data->metrics_table);
	if (rc != EOK)
		return rc;

	size_t records_read;
	uint32_t metrics_count;
	if (data->metrics_table.format & PCF_FORMAT_COMPRESSED_METRICS) {
		uint16_t metrics_count_16;
		records_read = fread(&metrics_count_16, sizeof(uint16_t), 1,
		    data->file);
		if (records_read != 1)
			return EINVAL;
		metrics_count_16 = uint16_t_pcf2host(metrics_count_16,
		    data->metrics_table.format);
		metrics_count = metrics_count_16;
	} else {
		records_read = fread(&metrics_count, sizeof(uint32_t), 1,
		    data->file);
		if (records_read != 1)
			return EINVAL;
		metrics_count = uint32_t_pcf2host(metrics_count,
		    data->metrics_table.format);
	}

	if (metrics_count != data->glyph_count)
		return EINVAL;

	return EOK;
}

static errno_t pcf_read_encodings_table_header(pcf_data_t *data)
{
	errno_t rc = pcf_seek_table_header(data, &data->encodings_table);
	if (rc != EOK)
		return rc;

	pcf_encoding_t encoding;
	size_t records_read = fread(&encoding, sizeof(pcf_encoding_t), 1,
	    data->file);
	if (records_read != 1)
		return EINVAL;

	encoding.min_byte1 = uint16_t_pcf2host(encoding.min_byte1,
	    data->encodings_table.format);
	encoding.max_byte1 = uint16_t_pcf2host(encoding.max_byte1,
	    data->encodings_table.format);
	encoding.min_byte2 = uint16_t_pcf2host(encoding.min_byte2,
	    data->encodings_table.format);
	encoding.max_byte2 = uint16_t_pcf2host(encoding.max_byte2,
	    data->encodings_table.format);
	encoding.default_char = uint16_t_pcf2host(encoding.default_char,
	    data->encodings_table.format);

	data->encoding = encoding;
	return EOK;
}

static errno_t pcf_read_accelerators_table(pcf_data_t *data)
{
	errno_t rc = pcf_seek_table_header(data, &data->accelerators_table);
	if (rc != EOK)
		return rc;

	pcf_accelerators_t accelerators;
	size_t records_read = fread(&accelerators, sizeof(pcf_accelerators_t),
	    1, data->file);
	if (records_read != 1)
		return EINVAL;

	data->font_metrics.ascender = int32_t_pcf2host(accelerators.font_ascent,
	    data->accelerators_table.format);
	data->font_metrics.descender = int32_t_pcf2host(accelerators.font_descent,
	    data->accelerators_table.format);
	data->font_metrics.leading = 0;

	return EOK;
}

errno_t pcf_font_create(font_t **font, char *filename, uint16_t points)
{
	errno_t rc;
	pcf_data_t *data = malloc(sizeof(pcf_data_t));
	if (data == NULL)
		return ENOMEM;

	data->file = fopen(filename, "rb");
	if (data->file == NULL)
		goto read_error;

	rc = pcf_read_toc(data);
	if (rc != EOK)
		goto error;

	rc = pcf_read_bitmap_table_header(data);
	if (rc != EOK)
		goto error;

	rc = pcf_read_metrics_table_header(data);
	if (rc != EOK)
		goto error;

	rc = pcf_read_encodings_table_header(data);
	if (rc != EOK)
		goto error;

	rc = pcf_read_accelerators_table(data);
	if (rc != EOK)
		goto error;

	rc = bitmap_font_create(&fd_pcf, data, data->glyph_count,
	    data->font_metrics, points, font);
	if (rc != EOK)
		goto error;

	return EOK;
read_error:
	rc = EINVAL;
error:
	if (data->file)
		fclose(data->file);
	free(data);
	return rc;
}

/** @}
 */
