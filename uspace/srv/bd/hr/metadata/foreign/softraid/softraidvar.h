/* $OpenBSD: softraidvar.h,v 1.176 2022/12/19 15:27:06 kn Exp $ */
/*
 * Copyright (c) 2006 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2008 Chris Kuethe <ckuethe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#ifndef _HR_METADATA_FOREIGN_SOFTRAID_H
#define _HR_METADATA_FOREIGN_SOFTRAID_H

/* HelenOS specific includes, retypes */

#include <crypto.h>

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

/* copied from <sys/param.h> */
#define	_DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1 << _DEV_BSHIFT)

/* here continues stripped down and slightly modified softraidvat.h */

#define MD5_DIGEST_LENGTH 16

#define SR_META_VERSION		6	/* bump when sr_metadata changes */
#define SR_META_SIZE		64	/* save space at chunk beginning */
#define SR_META_OFFSET		16	/* skip 8192 bytes at chunk beginning */

#define SR_BOOT_OFFSET		(SR_META_OFFSET + SR_META_SIZE)
#define SR_BOOT_LOADER_SIZE	320 /* Size of boot loader storage. */
#define SR_BOOT_LOADER_OFFSET	SR_BOOT_OFFSET
#define SR_BOOT_BLOCKS_SIZE	128 /* Size of boot block storage. */
#define SR_BOOT_BLOCKS_OFFSET	(SR_BOOT_LOADER_OFFSET + SR_BOOT_LOADER_SIZE)
#define SR_BOOT_SIZE		(SR_BOOT_LOADER_SIZE + SR_BOOT_BLOCKS_SIZE)

#define SR_HEADER_SIZE		(SR_META_SIZE + SR_BOOT_SIZE)
#define SR_DATA_OFFSET		(SR_META_OFFSET + SR_HEADER_SIZE)

#define SR_UUID_MAX		16
struct sr_uuid {
	u_int8_t		sui_id[SR_UUID_MAX];
} __attribute__((packed));

struct sr_metadata {
	struct sr_meta_invariant {
		/* do not change order of ssd_magic, ssd_version */
		u_int64_t	ssd_magic;	/* magic id */
#define	SR_MAGIC		0x4d4152436372616dLLU
		u_int32_t	ssd_version;	/* meta data version */
		u_int32_t	ssd_vol_flags;	/* volume specific flags. */
		struct sr_uuid	ssd_uuid;	/* unique identifier */

		/* chunks */
		u_int32_t	ssd_chunk_no;	/* number of chunks */
		u_int32_t	ssd_chunk_id;	/* chunk identifier */

		/* optional */
		u_int32_t	ssd_opt_no;	/* nr of optional md elements */
		u_int32_t	ssd_secsize;

		/* volume metadata */
		u_int32_t	ssd_volid;	/* volume id */
		u_int32_t	ssd_level;	/* raid level */
		int64_t		ssd_size;	/* virt disk size in blocks */
		char		ssd_vendor[8];	/* scsi vendor */
		char		ssd_product[16];/* scsi product */
		char		ssd_revision[4];/* scsi revision */
		/* optional volume members */
		u_int32_t	ssd_strip_size;	/* strip size */
	} _sdd_invariant;
#define ssdi			_sdd_invariant
	/* MD5 of invariant metadata */
	u_int8_t		ssd_checksum[MD5_DIGEST_LENGTH];
	char			ssd_devname[32];/* /dev/XXXXX */
	u_int32_t		ssd_meta_flags;
#define	SR_META_DIRTY		0x1
	u_int32_t		ssd_data_blkno;
	u_int64_t		ssd_ondisk;	/* on disk version counter */
	int64_t			ssd_rebuild;	/* last block of rebuild */
} __attribute__((packed));

struct sr_meta_chunk {
	struct sr_meta_chunk_invariant {
		u_int32_t	scm_volid;	/* vd we belong to */
		u_int32_t	scm_chunk_id;	/* chunk id */
		char		scm_devname[32];/* /dev/XXXXX */
		int64_t		scm_size;	/* size of partition in blocks */
		int64_t		scm_coerced_size; /* coerced sz of part in blk */
		struct sr_uuid	scm_uuid;	/* unique identifier */
	} _scm_invariant;
#define scmi			_scm_invariant
	/* MD5 of invariant chunk metadata */
	u_int8_t		scm_checksum[MD5_DIGEST_LENGTH];
	u_int32_t		scm_status;	/* use bio bioc_disk status */
} __attribute__((packed));

struct sr_meta_opt_hdr {
	u_int32_t	som_type;	/* optional metadata type. */
	u_int32_t	som_length;	/* optional metadata length. */
	u_int8_t	som_checksum[MD5_DIGEST_LENGTH];
} __attribute__((packed));

#define	SR_MD_RAID0		0
#define	SR_MD_RAID1		1
#define	SR_MD_RAID5		2
#define	SR_MD_CACHE		3
#define	SR_MD_CRYPTO		4
/* AOE was 5 and 6. */
/* SR_MD_RAID4 was 7. */
#define	SR_MD_RAID6		8
#define	SR_MD_CONCAT		9
#define	SR_MD_RAID1C		10

/* functions to export from softraid.c to hr_softraid.c */
void	 sr_checksum_print(const u_int8_t *);
char	*sr_uuid_format(const struct sr_uuid *);
void	 sr_uuid_print(const struct sr_uuid *, int);
void	 sr_meta_print(const struct sr_metadata *);

#endif

/** @}
 */
