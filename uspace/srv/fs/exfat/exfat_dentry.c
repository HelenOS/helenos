/*
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup exfat
 * @{
 */

/**
 * @file	exfat_dentry.c
 * @brief	Functions that work with exFAT directory entries.
 */

#include "exfat_dentry.h"
#include <ctype.h>
#include <str.h>
#include <errno.h>
#include <byteorder.h>
#include <assert.h>

exfat_dentry_clsf_t exfat_classify_dentry(const exfat_dentry_t *d)
{
	switch (d->type) {
	case EXFAT_TYPE_VOLLABEL:
		return EXFAT_DENTRY_VOLLABEL;
	case EXFAT_TYPE_BITMAP:
		return EXFAT_DENTRY_BITMAP;
	case EXFAT_TYPE_UCTABLE:
		return EXFAT_DENTRY_UCTABLE;
	case EXFAT_TYPE_GUID:
		return EXFAT_DENTRY_GUID;
	case EXFAT_TYPE_FILE:
		return EXFAT_DENTRY_FILE;
	case EXFAT_TYPE_STREAM:
		return EXFAT_DENTRY_STREAM;
	case EXFAT_TYPE_NAME:
		return EXFAT_DENTRY_NAME;
	case EXFAT_TYPE_UNUSED:
		return EXFAT_DENTRY_LAST;
	default:
		if (d->type & EXFAT_TYPE_USED)
			return EXFAT_DENTRY_SKIP;
		else
			return EXFAT_DENTRY_FREE;
	}
}

uint16_t exfat_name_hash(const uint16_t *name, const uint16_t *uctable, size_t chars)
{
	uint16_t hash = 0;
	uint16_t ch;
	while (*name) {
		if (*name < chars)
			ch = uint16_t_le2host(uctable[*name]);
		else
			ch = *name;
		name++;

		hash = ((hash << 15) | (hash >> 1)) + (ch & 0xff);
		hash = ((hash << 15) | (hash >> 1)) + (ch >> 8);
	}

	return hash;
}

void exfat_dentry_get_name(const exfat_name_dentry_t *name, size_t size, uint16_t *dst, size_t *offset)
{
	size_t i = 0;
	while (i < EXFAT_NAME_PART_LEN && *offset < size) {
		dst[*offset] = uint16_t_le2host(name->name[i]);
		i++;
		(*offset)++;
	}
	dst[*offset] = '\0';
}

void exfat_dentry_get_vollabel(const exfat_vollabel_dentry_t *vollabel,
    size_t size, uint16_t *dst)
{
	size_t i = 0;
	while (i < EXFAT_VOLLABEL_LEN && i < vollabel->size && i < size) {
		dst[i] = uint16_t_le2host(vollabel->label[i]);
		i++;
	}
	dst[i] = '\0';
}

bool exfat_valid_char(char32_t ch)
{
	if (ch >= 0x01 && ch <= 0x1F)
		return false;

	switch (ch) {
	case '/':
	case '\\':
	case '?':
	case '|':
	case '>':
	case '<':
	case '"':
	case '*':
	case ':':
		return false;
	default:
		return true;
	}
}

bool exfat_valid_name(const char *name)
{
	size_t off = 0;
	char32_t ch;

	while ((ch = str_decode(name, &off, STR_NO_LIMIT)) != 0) {
		if (!exfat_valid_char(ch))
			return false;
	}
	return true;
}

/**
 * @}
 */
