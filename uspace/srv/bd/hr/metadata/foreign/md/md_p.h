/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * md_p.h : physical layout of Linux RAID devices
 *        Copyright (C) 1996-98 Ingo Molnar, Gadi Oxman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#ifndef _HR_METADATA_FOREIGN_MD_H
#define _HR_METADATA_FOREIGN_MD_H

typedef uint64_t __le64;
typedef uint32_t __le32;
typedef uint16_t __le16;
typedef uint8_t __u8;

/* in 512 blocks */
#define MD_OFFSET 8
#define MD_SIZE 2

/* XXX: this is actually not used when assembling */
#define MD_DATA_OFFSET 2048

#define MD_MAGIC 0xa92b4efc

#define MD_DISK_SYNC 2

/*
 * The version-1 superblock :
 * All numeric fields are little-endian.
 *
 * total size: 256 bytes plus 2 per device.
 *  1K allows 384 devices.
 */
struct mdp_superblock_1 {
	/* constant array information - 128 bytes */
	__le32	magic;		/* MD_SB_MAGIC: 0xa92b4efc - little endian */
	__le32	major_version;	/* 1 */
	__le32	feature_map;	/* bit 0 set if 'bitmap_offset' is meaningful */
	__le32	pad0;		/* always set to 0 when writing */

	__u8	set_uuid[16];	/* user-space generated. */
	char	set_name[32];	/* set and interpreted by user-space */

	__le64	ctime;		/* lo 40 bits are seconds, top 24 are microseconds or 0 */
	__le32	level;		/* 0,1,4,5, -1 (linear) */
	__le32	layout;		/* only for raid5 and raid10 currently */
	__le64	size;		/* used size of component devices, in 512byte sectors */

	__le32	chunksize;	/* in 512byte sectors */
	__le32	raid_disks;
	union {
		__le32	bitmap_offset;
		/*
		 * sectors after start of superblock that bitmap starts
		 * NOTE: signed, so bitmap can be before superblock
		 * only meaningful of feature_map[0] is set.
		 */

		/* only meaningful when feature_map[MD_FEATURE_PPL] is set */
		struct {
			__le16 offset; /* sectors from start of superblock that ppl starts (signed) */
			__le16 size; /* ppl size in sectors */
		} ppl;
	};

	/* These are only valid with feature bit '4' */
	__le32	new_level;	/* new level we are reshaping to		*/
	__le64	reshape_position;	/* next address in array-space for reshape */
	__le32	delta_disks;	/* change in number of raid_disks		*/
	__le32	new_layout;	/* new layout					*/
	__le32	new_chunk;	/* new chunk size (512byte sectors)		*/
	__le32  new_offset;
	/*
	 * signed number to add to data_offset in new
	 * layout.  0 == no-change.  This can be
	 * different on each device in the array.
	 */

	/* constant this-device information - 64 bytes */
	__le64	data_offset;	/* sector start of data, often 0 */
	__le64	data_size;	/* sectors in this device that can be used for data */
	__le64	super_offset;	/* sector start of this superblock */
	union {
		__le64	recovery_offset;/* sectors before this offset (from data_offset) have been recovered */
		__le64	journal_tail;/* journal tail of journal device (from data_offset) */
	};
	__le32	dev_number;	/* permanent identifier of this  device - not role in raid */
	__le32	cnt_corrected_read; /* number of read errors that were corrected by re-writing */
	__u8	device_uuid[16]; /* user-space setable, ignored by kernel */
	__u8	devflags;	/* per-device flags.  Only two defined... */
#define	WriteMostly1	1	/* mask for writemostly flag in above */
#define	FailFast1	2	/* Should avoid retries and fixups and just fail */
	/*
	 * Bad block log.  If there are any bad blocks the feature flag is set.
	 * If offset and size are non-zero, that space is reserved and available
	 */
	__u8	bblog_shift;	/* shift from sectors to block size */
	__le16	bblog_size;	/* number of sectors reserved for list */
	__le32	bblog_offset;
	/*
	 * sector offset from superblock to bblog,
	 * signed - not unsigned
	 */

	/* array state information - 64 bytes */
	__le64	utime;		/* 40 bits second, 24 bits microseconds */
	__le64	events;		/* incremented when superblock updated */
	__le64	resync_offset;	/* data before this offset (from data_offset) known to be in sync */
	__le32	sb_csum;	/* checksum up to devs[max_dev] */
	__le32	max_dev;	/* size of devs[] array to consider */
	__u8	pad3[64 - 32];	/* set to 0 when writing */

	/*
	 * device state information. Indexed by dev_number.
	 * 2 bytes per device
	 * Note there are no per-device state flags. State information is rolled
	 * into the 'roles' value.  If a device is spare or faulty, then it doesn't
	 * have a meaningful role.
	 */
	__le16	dev_roles[];	/* role in array, or 0xffff for a spare, or 0xfffe for faulty */
};

/*
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2001-2016 Neil Brown <neilb@suse.com>
 *
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Author: Neil Brown
 *    Email: <neilb@suse.de>
 */

/* from mdadm - mdadm.h*/
#define ALGORITHM_LEFT_ASYMMETRIC	0
#define ALGORITHM_RIGHT_ASYMMETRIC	1
#define ALGORITHM_LEFT_SYMMETRIC	2

/* from mdadm - super1.c */
static inline unsigned int calc_sb_1_csum(struct mdp_superblock_1 * sb)
{
	unsigned int disk_csum, csum;
	unsigned long long newcsum;
	int size = sizeof(*sb) + uint32_t_le2host(sb->max_dev)*2;
	unsigned int *isuper = (unsigned int *)sb;

/* make sure I can count... */
	if (offsetof(struct mdp_superblock_1,data_offset) != 128 ||
	    offsetof(struct mdp_superblock_1, utime) != 192 ||
	    sizeof(struct mdp_superblock_1) != 256) {
		fprintf(stderr, "WARNING - superblock isn't sized correctly\n");
	}

	disk_csum = sb->sb_csum;
	sb->sb_csum = 0;
	newcsum = 0;
	for (; size >= 4; size -= 4) {
		newcsum += uint32_t_le2host(*isuper);
		isuper++;
	}

	if (size == 2)
		newcsum += uint32_t_le2host(*(unsigned short*) isuper);

	csum = (newcsum & 0xffffffff) + (newcsum >> 32);
	sb->sb_csum = disk_csum;
	return host2uint32_t_le(csum);
}

#endif

/** @}
 */
