/*
 * Copyright (c) 2005 Jakub Jermar
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
