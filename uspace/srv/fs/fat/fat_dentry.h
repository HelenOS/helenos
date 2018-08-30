/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2011 Oleg Romanenko
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

/** @addtogroup fat
 * @{
 */

#ifndef FAT_FAT_DENTRY_H_
#define FAT_FAT_DENTRY_H_

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <unaligned.h>

#define IS_D_CHAR(ch) (isalnum(ch) || ch == '_')
#define FAT_STOP_CHARS "*?/\\\n\t|'"

#define FAT_NAME_LEN		8
#define FAT_EXT_LEN		3
#define FAT_VOLLABEL_LEN	11

#define FAT_NAME_DOT		".       "
#define FAT_NAME_DOT_DOT	"..      "
#define FAT_EXT_PAD		"   "

#define FAT_ATTR_RDONLY   0x01
#define FAT_ATTR_HIDDEN   0x02
#define FAT_ATTR_SYSTEM   0x04
#define FAT_ATTR_VOLLABEL 0x08
#define FAT_ATTR_SUBDIR   0x10
#define FAT_ATTR_ARCHIVE  0x20
#define FAT_ATTR_LFN \
    (FAT_ATTR_RDONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLLABEL)

#define FAT_LCASE_LOWER_NAME	0x08
#define FAT_LCASE_LOWER_EXT	0x10

#define FAT_PAD			' '
#define FAT_LFN_PAD	0xffff
#define FAT_SFN_CHAR '_'

#define FAT_DENTRY_UNUSED	0x00
#define FAT_DENTRY_E5_ESC	0x05
#define FAT_DENTRY_DOT		0x2e
#define FAT_DENTRY_ERASED	0xe5
#define FAT_LFN_LAST		0x40
#define FAT_LFN_ERASED		0x80

#define FAT_LFN_ORDER(d) ((d)->lfn.order)
#define FAT_IS_LFN(d) \
    ((FAT_LFN_ORDER((d)) & FAT_LFN_LAST) == FAT_LFN_LAST)
#define FAT_LFN_COUNT(d) \
    (FAT_LFN_ORDER((d)) ^ FAT_LFN_LAST)
#define FAT_LFN_PART1(d) ((d)->lfn.part1)
#define FAT_LFN_PART2(d) ((d)->lfn.part2)
#define FAT_LFN_PART3(d) ((d)->lfn.part3)
#define FAT_LFN_ATTR(d) ((d)->lfn.attr)
#define FAT_LFN_CHKSUM(d) ((d)->lfn.check_sum)

#define FAT_LFN_NAME_LEN    260                           /* characters */
#define FAT_LFN_NAME_SIZE   STR_BOUNDS(FAT_LFN_NAME_LEN)  /* bytes */
#define FAT_LFN_MAX_COUNT   20
#define FAT_LFN_PART1_SIZE  5
#define FAT_LFN_PART2_SIZE  6
#define FAT_LFN_PART3_SIZE  2
#define FAT_LFN_ENTRY_SIZE \
    (FAT_LFN_PART1_SIZE + FAT_LFN_PART2_SIZE + FAT_LFN_PART3_SIZE)

typedef enum {
	FAT_DENTRY_SKIP,
	FAT_DENTRY_LAST,
	FAT_DENTRY_FREE,
	FAT_DENTRY_VALID,
	FAT_DENTRY_LFN,
	FAT_DENTRY_VOLLABEL
} fat_dentry_clsf_t;

typedef union {
	struct {
		uint8_t		name[8];
		uint8_t		ext[3];
		uint8_t		attr;
		uint8_t		lcase;
		uint8_t		ctime_fine;
		uint16_t	ctime;
		uint16_t	cdate;
		uint16_t	adate;
		union {
			uint16_t	eaidx;		/* FAT12/FAT16 */
			uint16_t	firstc_hi;	/* FAT32 */
		} __attribute__((packed));
		uint16_t	mtime;
		uint16_t	mdate;
		union {
			uint16_t	firstc;		/* FAT12/FAT16 */
			uint16_t	firstc_lo;	/* FAT32 */
		} __attribute__((packed));
		uint32_t	size;
	} __attribute__((packed));
	struct {
		uint8_t		order;
		uint16_t	part1[FAT_LFN_PART1_SIZE];
		uint8_t		attr;
		uint8_t		type;
		uint8_t		check_sum;
		uint16_t	part2[FAT_LFN_PART2_SIZE];
		uint16_t	firstc_lo; /* MUST be 0 */
		uint16_t	part3[FAT_LFN_PART3_SIZE];
	} __attribute__((packed)) lfn;
} __attribute__((packed)) fat_dentry_t;

extern int fat_dentry_namecmp(char *, const char *);
extern void fat_dentry_name_get(const fat_dentry_t *, char *);
extern void fat_dentry_name_set(fat_dentry_t *, const char *);
extern void fat_dentry_vollabel_get(const fat_dentry_t *, char *);
extern fat_dentry_clsf_t fat_classify_dentry(const fat_dentry_t *);
extern uint8_t fat_dentry_chksum(uint8_t *);

extern size_t fat_lfn_str_nlength(const unaligned_uint16_t *, size_t);
extern size_t fat_lfn_size(const fat_dentry_t *);
extern size_t fat_lfn_get_entry(const fat_dentry_t *, uint16_t *, size_t *);
extern size_t fat_lfn_set_entry(const uint16_t *, size_t *, size_t,
    fat_dentry_t *);

extern void str_to_ascii(char *, const char *, size_t, uint8_t);

extern bool fat_valid_name(const char *);
extern bool fat_valid_short_name(const char *);

#endif

/**
 * @}
 */
