/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup fs
 * @{
 */ 

/**
 * @file	fat_dentry.c
 * @brief	Functions that work with FAT directory entries.
 */

#include "fat_dentry.h"
#include <ctype.h>
#include <str.h>
#include <errno.h>
#include <byteorder.h>
#include <assert.h>

static bool is_d_char(const char ch)
{
	if (isalnum(ch) || ch == '_')
		return true;
	else
		return false;
}

/** Compare path component with the name read from the dentry.
 *
 * This function compares the path component with the name read from the dentry.
 * The comparison is case insensitive and tolerates a mismatch on the trailing
 * dot character at the end of the name (i.e. when there is a dot, but no
 * extension).
 *
 * @param name		Node name read from the dentry.
 * @param component	Path component.
 *
 * @return		Zero on match, non-zero otherwise.
 */
int fat_dentry_namecmp(char *name, const char *component)
{
	int rc;
	size_t size;

	if (!(rc = stricmp(name, component)))
		return rc;
	if (!str_chr(name, '.')) {
		/*
		 * There is no '.' in the name, so we know that there is enough
		 * space for appending an extra '.' to name.
		 */
		size = str_size(name);
		name[size] = '.';
		name[size + 1] = '\0';
		rc = stricmp(name, component);
	}
	return rc;
}

bool fat_dentry_name_verify(const char *name)
{
	unsigned int i;
	unsigned int dot = 0;
	bool dot_found = false;
	

	for (i = 0; name[i]; i++) {
		if (name[i] == '.') {
			if (dot_found) {
				return false;
			} else {
				dot_found = true;
				dot = i;
			}
		} else {
			if (!is_d_char(name[i]))
				return false;
		}
	}

	if (dot_found) {
		if (dot > FAT_NAME_LEN)
			return false;
		if (i - dot > FAT_EXT_LEN + 1)
			return false;
	} else {
		if (i > FAT_NAME_LEN)
			return false;
	}

	return true;
}

void fat_dentry_name_get(const fat_dentry_t *d, char *buf)
{
	unsigned int i;
	
	for (i = 0; i < FAT_NAME_LEN; i++) {
		if (d->name[i] == FAT_PAD)
			break;
		
		if (d->name[i] == FAT_DENTRY_E5_ESC)
			*buf++ = 0xe5;
		else {
			if (d->lcase & FAT_LCASE_LOWER_NAME)
				*buf++ = tolower(d->name[i]);
			else
				*buf++ = d->name[i];
		}
	}
	
	if (d->ext[0] != FAT_PAD)
		*buf++ = '.';
	
	for (i = 0; i < FAT_EXT_LEN; i++) {
		if (d->ext[i] == FAT_PAD) {
			*buf = '\0';
			return;
		}
		
		if (d->ext[i] == FAT_DENTRY_E5_ESC)
			*buf++ = 0xe5;
		else {
			if (d->lcase & FAT_LCASE_LOWER_EXT)
				*buf++ = tolower(d->ext[i]);
			else
				*buf++ = d->ext[i];
		}
	}
	
	*buf = '\0';
}

void fat_dentry_name_set(fat_dentry_t *d, const char *name)
{
	unsigned int i;
	const char fake_ext[] = "   ";
	bool lower_name = true;
	bool lower_ext = true;
	
	for (i = 0; i < FAT_NAME_LEN; i++) {
		switch ((uint8_t) *name) {
		case 0xe5:
			d->name[i] = FAT_DENTRY_E5_ESC;
			name++;
			break;
		case '\0':
		case '.':
			d->name[i] = FAT_PAD;
			break;
		default:
			if (isalpha(*name)) {
				if (!islower(*name))
					lower_name = false;
			}
			
			d->name[i] = toupper(*name++);
			break;
		}
	}
	
	if (*name++ != '.')
		name = fake_ext;
	
	for (i = 0; i < FAT_EXT_LEN; i++) {
		switch ((uint8_t) *name) {
		case 0xe5:
			d->ext[i] = FAT_DENTRY_E5_ESC;
			name++;
			break;
		case '\0':
			d->ext[i] = FAT_PAD;
			break;
		default:
			if (isalpha(*name)) {
				if (!islower(*name))
					lower_ext = false;
			}
			
			d->ext[i] = toupper(*name++);
			break;
		}
	}
	
	if (lower_name)
		d->lcase |= FAT_LCASE_LOWER_NAME;
	else
		d->lcase &= ~FAT_LCASE_LOWER_NAME;
	
	if (lower_ext)
		d->lcase |= FAT_LCASE_LOWER_EXT;
	else
		d->lcase &= ~FAT_LCASE_LOWER_EXT;
}

fat_dentry_clsf_t fat_classify_dentry(const fat_dentry_t *d)
{
	if (d->attr == FAT_ATTR_LFN) {
		/* long name entry */
		if (FAT_LFN_ORDER(d) & FAT_LFN_ERASED)
			return FAT_DENTRY_FREE;
		else
			return FAT_DENTRY_LFN;
	}
	if (d->attr & FAT_ATTR_VOLLABEL) {
		/* volume label entry */
		return FAT_DENTRY_SKIP;
	}
	if (d->name[0] == FAT_DENTRY_ERASED) {
		/* not-currently-used entry */
		return FAT_DENTRY_FREE;
	}
	if (d->name[0] == FAT_DENTRY_UNUSED) {
		/* never used entry */
		return FAT_DENTRY_LAST;
	}
	if (d->name[0] == FAT_DENTRY_DOT) {
		/*
		 * Most likely '.' or '..'.
		 * It cannot occur in a regular file name.
		 */
		return FAT_DENTRY_SKIP;
	}
	return FAT_DENTRY_VALID;
}

/** Compute checksum of Node name.
 *
 * Returns an unsigned byte checksum computed on an unsigned byte
 * array. The array must be 11 bytes long and is assumed to contain
 * a name stored in the format of a MS-DOS directory entry.
 *
 * @param name		Node name read from the dentry.
 *
 * @return		An 8-bit unsigned checksum of the name.
 */
uint8_t fat_dentry_chksum(uint8_t *name)
{
	uint8_t i, sum=0;
	for (i=0; i<(FAT_NAME_LEN+FAT_EXT_LEN); i++) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + name[i];
	}
	return sum;
}

/** Get number of bytes in a string with size limit.
 *
 * @param str  NULL-terminated (or not) string.
 * @param size Maximum number of bytes to consider.
 *
 * @return Number of bytes in string (without 0 and ff).
 *
 */
size_t fat_lfn_str_nlength(const uint16_t *str, size_t size)
{
	size_t offset = 0;

	while (offset < size) {
		if (str[offset] == 0 || str[offset] == 0xffff) 
			break;
		offset++;
	}
	return offset;
}

/** Get number of bytes in a FAT long entry occuped by characters.
 *
 * @param d FAT long entry.
 *
 * @return Number of bytes.
 *
 */
size_t fat_lfn_size(const fat_dentry_t *d)
{
	size_t size = 0;
	
	size += fat_lfn_str_nlength(FAT_LFN_PART1(d), FAT_LFN_PART1_SIZE);
	size += fat_lfn_str_nlength(FAT_LFN_PART2(d), FAT_LFN_PART2_SIZE);
	size += fat_lfn_str_nlength(FAT_LFN_PART3(d), FAT_LFN_PART3_SIZE);	
	
	return size;
}

size_t fat_lfn_copy_part(const uint16_t *src, size_t src_size, uint16_t *dst, size_t *offset)
{
	while (src_size!=0 && (*offset)!=0) {
		src_size--;
		if (src[src_size] == 0 || src[src_size] == 0xffff)
			continue;

		(*offset)--;
		dst[(*offset)] = uint16_t_le2host(src[src_size]);
	}
	return (*offset);
}

size_t fat_lfn_copy_entry(const fat_dentry_t *d, uint16_t *dst, size_t *offset)
{
	fat_lfn_copy_part(FAT_LFN_PART3(d), FAT_LFN_PART3_SIZE, dst, offset);
	fat_lfn_copy_part(FAT_LFN_PART2(d), FAT_LFN_PART2_SIZE, dst, offset);
	fat_lfn_copy_part(FAT_LFN_PART1(d), FAT_LFN_PART1_SIZE, dst, offset);

	return *offset;
}

/** Convert utf16 string to string.
 *
 * Convert wide string @a src to string. The output is written to the buffer
 * specified by @a dest and @a size. @a size must be non-zero and the string
 * written will always be well-formed.
 *
 * @param dest	Destination buffer.
 * @param size	Size of the destination buffer.
 * @param src	Source wide string.
 */
int utf16_to_str(char *dest, size_t size, const uint16_t *src)
{
	int rc;
	uint16_t ch;
	size_t src_idx, dest_off;

	/* There must be space for a null terminator in the buffer. */
	assert(size > 0);
	
	src_idx = 0;
	dest_off = 0;

	while ((ch = src[src_idx++]) != 0) {
		rc = chr_encode(ch, dest, &dest_off, size - 1);
		if (rc != EOK)
			return rc;
	}

	dest[dest_off] = '\0';
	return EOK;
}

/** Convert string to utf16 string.
 *
 * Convert string @a src to wide string. The output is written to the
 * buffer specified by @a dest and @a dlen. @a dlen must be non-zero
 * and the wide string written will always be null-terminated.
 *
 * @param dest	Destination buffer.
 * @param dlen	Length of destination buffer (number of wchars).
 * @param src	Source string.
 */
int str_to_utf16(uint16_t *dest, size_t dlen, const char *src)
{
	size_t offset;
	size_t di;
	uint16_t c;

	assert(dlen > 0);

	offset = 0;
	di = 0;

	do {
		if (di >= dlen - 1)
			return EOVERFLOW;

		c = str_decode(src, &offset, STR_NO_LIMIT);
		dest[di++] = c;
	} while (c != '\0');

	dest[dlen - 1] = '\0';
	return EOK;
}


/**
 * @}
 */ 
