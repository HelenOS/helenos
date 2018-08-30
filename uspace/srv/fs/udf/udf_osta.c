/*
 * Copyright (c) 2012 Julia Medvedeva
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

/*
 * OSTA compliant Unicode compression, uncompression routines,
 * file name translation routine for OS/2, Windows 95, Windows NT,
 * Macintosh and UNIX.
 * Copyright 1995 Micro Design International, Inc.
 * Written by Jason M. Rinn.
 * Micro Design International gives permission for the free use of the
 * following source code.
 */

/** @addtogroup udf
 * @{
 */
/**
 * @file udf_osta.c
 * @brief OSTA compliant functions.
 */

#include <stdlib.h>
#include <str.h>
#include <macros.h>
#include <errno.h>
#include "udf_osta.h"
#include "udf_cksum.h"

/** Illegal UNIX characters are NULL and slash.
 *
 */
static bool legal_check(uint16_t ch)
{
	if ((ch == 0x0000) || (ch == 0x002F))
		return false;

	return true;
}

/** Convert OSTA CS0 compressed Unicode name to Unicode.
 *
 * The Unicode output will be in the byte order that the local compiler
 * uses for 16-bit values.
 *
 * NOTE: This routine only performs error checking on the comp_id.
 * It is up to the user to ensure that the Unicode buffer is large
 * enough, and that the compressed Unicode name is correct.
 *
 * @param[in]  number_of_bytes Number of bytes read from media
 * @param[in]  udf_compressed  Bytes read from media
 * @param[out] unicode         Uncompressed unicode characters
 * @param[in]  unicode_max_len Size of output array
 *
 * @return Number of Unicode characters which were uncompressed.
 *
 */
static size_t udf_uncompress_unicode(size_t number_of_bytes,
    uint8_t *udf_compressed, uint16_t *unicode, size_t unicode_max_len)
{
	/* Use udf_compressed to store current byte being read. */
	uint8_t comp_id = udf_compressed[0];

	/* First check for valid compID. */
	if ((comp_id != 8) && (comp_id != 16))
		return 0;

	size_t unicode_idx = 0;
	size_t byte_idx = 1;

	/* Loop through all the bytes. */
	while ((byte_idx < number_of_bytes) && (unicode_idx < unicode_max_len)) {
		if (comp_id == 16) {
			/*
			 * Move the first byte to the high bits of the
			 * Unicode char.
			 */
			unicode[unicode_idx] = udf_compressed[byte_idx++] << 8;
		} else
			unicode[unicode_idx] = 0;

		if (byte_idx < number_of_bytes) {
			/* Then the next byte to the low bits. */
			unicode[unicode_idx] |= udf_compressed[byte_idx++];
		}

		unicode_idx++;
	}

	return unicode_idx;
}

/** Translate a long file name
 *
 * Translate a long file name to one using a MAXLEN and an illegal char set
 * in accord with the OSTA requirements. Assumes the name has already been
 * translated to Unicode.
 *
 * @param[out] new_name Translated name. Must be of length MAXLEN
 * @param[in]  udf_name Name from UDF volume
 * @param[in]  udf_len  Length of UDF Name
 *
 * @return Number of Unicode characters in translated name.
 *
 */
size_t udf_translate_name(uint16_t *new_name, uint16_t *udf_name,
    size_t udf_len)
{
	bool needs_crc = false;
	bool has_ext = false;
	size_t ext_idx = 0;
	size_t new_idx = 0;
	size_t new_ext_idx = 0;

	for (size_t idx = 0; idx < udf_len; idx++) {
		uint16_t current = udf_name[idx];

		if ((!legal_check(current)) || (!ascii_check(current))) {
			needs_crc = true;

			/*
			 * Replace Illegal and non-displayable chars with
			 * underscore.
			 */
			current = ILLEGAL_CHAR_MARK;

			/*
			 * Skip any other illegal or non-displayable
			 * characters.
			 */
			while ((idx + 1 < udf_len) &&
			    (!legal_check(udf_name[idx + 1]) ||
			    (!ascii_check(udf_name[idx + 1]))))
				idx++;
		}

		/* Record position of extension, if one is found. */
		if ((current == PERIOD) && ((udf_len - idx - 1) <= EXT_SIZE)) {
			if (udf_len == idx + 1) {
				/* A trailing period is NOT an extension. */
				has_ext = false;
			} else {
				has_ext = true;
				ext_idx = idx;
				new_ext_idx = new_idx;
			}
		}

		if (new_idx < MAXLEN)
			new_name[new_idx++] = current;
		else
			needs_crc = true;
	}

	if (needs_crc) {
		uint16_t ext[EXT_SIZE];
		size_t local_ext_idx = 0;

		if (has_ext) {
			size_t max_filename_len;

			/* Translate extension, and store it in ext. */
			for (size_t idx = 0; (idx < EXT_SIZE) &&
			    (ext_idx + idx + 1 < udf_len); idx++) {
				uint16_t current = udf_name[ext_idx + idx + 1];

				if ((!legal_check(current)) || (!ascii_check(current))) {
					needs_crc = true;

					/*
					 * Replace Illegal and non-displayable
					 * chars with underscore.
					 */
					current = ILLEGAL_CHAR_MARK;

					/*
					 * Skip any other illegal or
					 * non-displayable characters.
					 */
					while ((idx + 1 < EXT_SIZE) &&
					    ((!legal_check(udf_name[ext_idx + idx + 2])) ||
					    (!ascii_check(udf_name[ext_idx + idx + 2]))))
						idx++;
				}

				ext[local_ext_idx++] = current;
			}

			/*
			 * Truncate filename to leave room for extension and
			 * CRC.
			 */
			max_filename_len = ((MAXLEN - 5) - local_ext_idx - 1);
			if (new_idx > max_filename_len)
				new_idx = max_filename_len;
			else
				new_idx = new_ext_idx;
		} else if (new_idx > MAXLEN - 5) {
			/* If no extension, make sure to leave room for CRC. */
			new_idx = MAXLEN - 5;
		}

		/* Add mark for CRC. */
		new_name[new_idx++] = CRC_MARK;

		/* Calculate CRC from original filename. */
		uint16_t value_crc = udf_unicode_cksum(udf_name, udf_len);

		/* Convert 16-bits of CRC to hex characters. */
		const char hex_char[] = "0123456789ABCDEF";

		new_name[new_idx++] = hex_char[(value_crc & 0xf000) >> 12];
		new_name[new_idx++] = hex_char[(value_crc & 0x0f00) >> 8];
		new_name[new_idx++] = hex_char[(value_crc & 0x00f0) >> 4];
		new_name[new_idx++] = hex_char[(value_crc & 0x000f)];

		/* Place a translated extension at end, if found. */
		if (has_ext) {
			new_name[new_idx++] = PERIOD;

			for (size_t idx = 0; idx < local_ext_idx; idx++)
				new_name[new_idx++] = ext[idx];
		}
	}

	return new_idx;
}

/** Decode from dchar to utf8
 *
 * @param result     Returned value - utf8 string
 * @param result_len Length of output string
 * @param id         Input string
 * @param len        Length of input string
 * @param chsp       Decode method
 *
 */
void udf_to_unix_name(char *result, size_t result_len, char *id, size_t len,
    udf_charspec_t *chsp)
{
	const char *osta_id = "OSTA Compressed Unicode";
	size_t ucode_chars, nice_uchars;

	uint16_t *raw_name = malloc(MAX_BUF * sizeof(uint16_t));
	uint16_t *unix_name = malloc(MAX_BUF * sizeof(uint16_t));

	// FIXME: Check for malloc returning NULL

	bool is_osta_typ0 = (chsp->type == 0) &&
	    (str_cmp((char *) chsp->info, osta_id) == 0);

	if (is_osta_typ0) {
		*raw_name = 0;
		*unix_name = 0;

		ucode_chars =
		    udf_uncompress_unicode(len, (uint8_t *) id, raw_name, MAX_BUF);
		ucode_chars = min(ucode_chars, utf16_wsize(raw_name));
		nice_uchars =
		    udf_translate_name(unix_name, raw_name, ucode_chars);

		/* Output UTF-8 */
		unix_name[nice_uchars] = 0;
		utf16_to_str(result, result_len, unix_name);
	} else {
		/* Assume 8 bit char length byte Latin-1 */
		str_ncpy(result, result_len, (char *) (id + 1),
		    str_size((char *) (id + 1)));
	}

	free(raw_name);
	free(unix_name);
}

/**
 * @}
 */
