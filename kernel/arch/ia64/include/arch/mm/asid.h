/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_ASID_H_
#define KERN_ia64_ASID_H_

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef uint16_t asid_t;
typedef uint32_t rid_t;

#endif  /* __ASSEMBLER__ */

/**
 * Number of ia64 RIDs (Region Identifiers) per kernel ASID.
 * Note that some architectures may support more bits,
 * but those extra bits are not used by the kernel.
 */
#define RIDS_PER_ASID		8

#define RID_MAX			262143		/* 2^18 - 1 */
#define RID_KERNEL7		7

#define ASID2RID(asid, vrn) \
	((asid) * RIDS_PER_ASID + (vrn))

#define RID2ASID(rid) \
	((rid) / RIDS_PER_ASID)

#define ASID_MAX_ARCH		(RID_MAX / RIDS_PER_ASID)

#endif

/** @}
 */
