/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2004-2006 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#ifndef	_HR_METADATA_FOREIGN_GEOM_MIRROR_H
#define	_HR_METADATA_FOREIGN_GEOM_MIRROR_H

/* needed HelenOS headers */
#include <crypto.h>
#include <mem.h>
#include <str.h>

/* new typedefs */
typedef unsigned char u_char;
typedef unsigned int u_int;
#define bcopy(src, dst, len) memcpy(dst, src, len)

/* needed FreeBSD <sys/endian.h> header */
#include "sys_endian.h"

/* here continues the stripped down original header */

#define	G_MIRROR_MAGIC		"GEOM::MIRROR"

#define	G_MIRROR_BALANCE_NONE		0
#define	G_MIRROR_BALANCE_ROUND_ROBIN	1
#define	G_MIRROR_BALANCE_LOAD		2
#define	G_MIRROR_BALANCE_SPLIT		3
#define	G_MIRROR_BALANCE_PREFER		4
#define	G_MIRROR_BALANCE_MIN		G_MIRROR_BALANCE_NONE
#define	G_MIRROR_BALANCE_MAX		G_MIRROR_BALANCE_PREFER

#define	G_MIRROR_DISK_FLAG_DIRTY		0x0000000000000001ULL
#define	G_MIRROR_DISK_FLAG_SYNCHRONIZING	0x0000000000000002ULL
#define	G_MIRROR_DISK_FLAG_FORCE_SYNC		0x0000000000000004ULL
#define	G_MIRROR_DISK_FLAG_INACTIVE		0x0000000000000008ULL
#define	G_MIRROR_DISK_FLAG_HARDCODED		0x0000000000000010ULL
#define	G_MIRROR_DISK_FLAG_BROKEN		0x0000000000000020ULL
#define	G_MIRROR_DISK_FLAG_CANDELETE		0x0000000000000040ULL

#define	G_MIRROR_DISK_FLAG_MASK		(G_MIRROR_DISK_FLAG_DIRTY |	\
					 G_MIRROR_DISK_FLAG_SYNCHRONIZING | \
					 G_MIRROR_DISK_FLAG_FORCE_SYNC | \
					 G_MIRROR_DISK_FLAG_INACTIVE | \
					 G_MIRROR_DISK_FLAG_CANDELETE)

#define	G_MIRROR_DEVICE_FLAG_NOAUTOSYNC	0x0000000000000001ULL
#define	G_MIRROR_DEVICE_FLAG_NOFAILSYNC	0x0000000000000002ULL

#define	G_MIRROR_DEVICE_FLAG_DESTROY	0x0100000000000000ULL
#define	G_MIRROR_DEVICE_FLAG_DRAIN	0x0200000000000000ULL
#define	G_MIRROR_DEVICE_FLAG_CLOSEWAIT	0x0400000000000000ULL
#define	G_MIRROR_DEVICE_FLAG_TASTING	0x0800000000000000ULL
#define	G_MIRROR_DEVICE_FLAG_WIPE	0x1000000000000000ULL

struct g_mirror_metadata {
	char		md_magic[16];	/* Magic value. */
	uint32_t	md_version;	/* Version number. */
	char		md_name[16];	/* Mirror name. */
	uint32_t	md_mid;		/* Mirror unique ID. */
	uint32_t	md_did;		/* Disk unique ID. */
	uint8_t		md_all;		/* Number of disks in mirror. */
	uint32_t	md_genid;	/* Generation ID. */
	uint32_t	md_syncid;	/* Synchronization ID. */
	uint8_t		md_priority;	/* Disk priority. */
	uint32_t	md_slice;	/* Slice size. */
	uint8_t		md_balance;	/* Balance type. */
	uint64_t	md_mediasize;	/* Size of the smallest disk in mirror. */
	uint32_t	md_sectorsize;	/* Sector size. */
	uint64_t	md_sync_offset;	/* Synchronized offset. */
	uint64_t	md_mflags;	/* Additional mirror flags. */
	uint64_t	md_dflags;	/* Additional disk flags. */
	char		md_provider[16]; /* Hardcoded provider. */
	uint64_t	md_provsize;	/* Provider's size. */
	u_char		md_hash[16];	/* MD5 hash. */
};

static __inline void
mirror_metadata_encode(struct g_mirror_metadata *md, u_char *data)
{
	uint8_t md5_hash[16];

	bcopy(md->md_magic, data, 16);
	le32enc(data + 16, md->md_version);
	bcopy(md->md_name, data + 20, 16);
	le32enc(data + 36, md->md_mid);
	le32enc(data + 40, md->md_did);
	*(data + 44) = md->md_all;
	le32enc(data + 45, md->md_genid);
	le32enc(data + 49, md->md_syncid);
	*(data + 53) = md->md_priority;
	le32enc(data + 54, md->md_slice);
	*(data + 58) = md->md_balance;
	le64enc(data + 59, md->md_mediasize);
	le32enc(data + 67, md->md_sectorsize);
	le64enc(data + 71, md->md_sync_offset);
	le64enc(data + 79, md->md_mflags);
	le64enc(data + 87, md->md_dflags);
	bcopy(md->md_provider, data + 95, 16);
	le64enc(data + 111, md->md_provsize);

	errno_t rc = create_hash(data, 119, md5_hash, HASH_MD5);
	assert(rc == EOK);
	bcopy(md5_hash, data + 119, 16);
}

static __inline int
mirror_metadata_decode_v3v4(const u_char *data, struct g_mirror_metadata *md)
{
	uint8_t md5_hash[16];

	bcopy(data + 20, md->md_name, 16);
	md->md_mid = le32dec(data + 36);
	md->md_did = le32dec(data + 40);
	md->md_all = *(data + 44);
	md->md_genid = le32dec(data + 45);
	md->md_syncid = le32dec(data + 49);
	md->md_priority = *(data + 53);
	md->md_slice = le32dec(data + 54);
	md->md_balance = *(data + 58);
	md->md_mediasize = le64dec(data + 59);
	md->md_sectorsize = le32dec(data + 67);
	md->md_sync_offset = le64dec(data + 71);
	md->md_mflags = le64dec(data + 79);
	md->md_dflags = le64dec(data + 87);
	bcopy(data + 95, md->md_provider, 16);
	md->md_provsize = le64dec(data + 111);
	bcopy(data + 119, md->md_hash, 16);

	errno_t rc = create_hash(data, 119, md5_hash, HASH_MD5);
	assert(rc == EOK);
	if (memcmp(md->md_hash, md5_hash, 16) != 0)
		return (EINVAL);

	return (0);
}
static __inline int
mirror_metadata_decode(const u_char *data, struct g_mirror_metadata *md)
{
	int error;

	bcopy(data, md->md_magic, 16);
	if (str_lcmp(md->md_magic, G_MIRROR_MAGIC, 16) != 0)
		return (EINVAL);

	md->md_version = le32dec(data + 16);
	switch (md->md_version) {
	case 4:
		error = mirror_metadata_decode_v3v4(data, md);
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

static __inline const char *
balance_name(u_int balance)
{
	static const char *algorithms[] = {
		[G_MIRROR_BALANCE_NONE] = "none",
		[G_MIRROR_BALANCE_ROUND_ROBIN] = "round-robin",
		[G_MIRROR_BALANCE_LOAD] = "load",
		[G_MIRROR_BALANCE_SPLIT] = "split",
		[G_MIRROR_BALANCE_PREFER] = "prefer",
		[G_MIRROR_BALANCE_MAX + 1] = "unknown"
	};

	if (balance > G_MIRROR_BALANCE_MAX)
		balance = G_MIRROR_BALANCE_MAX + 1;

	return (algorithms[balance]);
}

static __inline int
balance_id(const char *name)
{
	static const char *algorithms[] = {
		[G_MIRROR_BALANCE_NONE] = "none",
		[G_MIRROR_BALANCE_ROUND_ROBIN] = "round-robin",
		[G_MIRROR_BALANCE_LOAD] = "load",
		[G_MIRROR_BALANCE_SPLIT] = "split",
		[G_MIRROR_BALANCE_PREFER] = "prefer"
	};
	int n;

	for (n = G_MIRROR_BALANCE_MIN; n <= G_MIRROR_BALANCE_MAX; n++) {
		if (str_cmp(name, algorithms[n]) == 0)
			return (n);
	}
	return (-1);
}

static __inline void
mirror_metadata_dump(const struct g_mirror_metadata *md)
{
	static const char hex[] = "0123456789abcdef";
	char hash[16 * 2 + 1];
	u_int i;

	printf("     magic: %s\n", md->md_magic);
	printf("   version: %u\n", (u_int)md->md_version);
	printf("      name: %s\n", md->md_name);
	printf("       mid: %u\n", (u_int)md->md_mid);
	printf("       did: %u\n", (u_int)md->md_did);
	printf("       all: %u\n", (u_int)md->md_all);
	printf("     genid: %u\n", (u_int)md->md_genid);
	printf("    syncid: %u\n", (u_int)md->md_syncid);
	printf("  priority: %u\n", (u_int)md->md_priority);
	printf("     slice: %u\n", (u_int)md->md_slice);
	printf("   balance: %s\n", balance_name((u_int)md->md_balance));
	printf(" mediasize: %jd\n", (intmax_t)md->md_mediasize);
	printf("sectorsize: %u\n", (u_int)md->md_sectorsize);
	printf("syncoffset: %jd\n", (intmax_t)md->md_sync_offset);
	printf("    mflags:");
	if (md->md_mflags == 0)
		printf(" NONE");
	else {
		if ((md->md_mflags & G_MIRROR_DEVICE_FLAG_NOFAILSYNC) != 0)
			printf(" NOFAILSYNC");
		if ((md->md_mflags & G_MIRROR_DEVICE_FLAG_NOAUTOSYNC) != 0)
			printf(" NOAUTOSYNC");
	}
	printf("\n");
	printf("    dflags:");
	if (md->md_dflags == 0)
		printf(" NONE");
	else {
		if ((md->md_dflags & G_MIRROR_DISK_FLAG_DIRTY) != 0)
			printf(" DIRTY");
		if ((md->md_dflags & G_MIRROR_DISK_FLAG_SYNCHRONIZING) != 0)
			printf(" SYNCHRONIZING");
		if ((md->md_dflags & G_MIRROR_DISK_FLAG_FORCE_SYNC) != 0)
			printf(" FORCE_SYNC");
		if ((md->md_dflags & G_MIRROR_DISK_FLAG_INACTIVE) != 0)
			printf(" INACTIVE");
	}
	printf("\n");
	printf("hcprovider: %s\n", md->md_provider);
	printf("  provsize: %ju\n", (uintmax_t)md->md_provsize);
	/* bzero(hash, sizeof(hash)); */
	memset(hash, 0, sizeof(hash));

	for (i = 0; i < 16; i++) {
		hash[i * 2] = hex[md->md_hash[i] >> 4];
		hash[i * 2 + 1] = hex[md->md_hash[i] & 0x0f];
	}
	printf("  MD5 hash: %s\n", hash);
}

#endif

/** @}
 */
