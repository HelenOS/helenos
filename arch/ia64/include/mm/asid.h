/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __ia64_ASID_H__
#define __ia64_ASID_H__

#include <arch/types.h>

typedef __u32 asid_t;

/**
 * This macro eliminates the stealing branch of asid_get().
 */
#define ASID_STEALING_ENABLED	0

/** Number of ia64 RIDs (Region Identifiers) per kernel ASID. */
#define RIDS_PER_ASID		7
#define RID_OVERFLOW		16777216	/* 2^24 */

#define ASID2RID(asid, vrn)	(((asid)*RIDS_PER_ASID)+(vrn))
#define RID2ASID(rid)		((rid)/RIDS_PER_ASID)

typedef __u32 rid_t;

/**
 * This macro is needed only to compile the kernel.
 * On ia64, its value is ignored.
 */
#define ASID_MAX_ARCH		0

/**
 * Value used to recognize the situation when all ASIDs were already allocated.
 */
#define ASID_OVERFLOW		(RID_OVERFLOW/RIDS_PER_ASID)

/** On ia64, this is no-op. */
#define asid_put_arch(x)

#endif
