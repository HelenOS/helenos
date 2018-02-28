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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_REGDEF_H_
#define KERN_sparc64_REGDEF_H_

#define PSTATE_IE_BIT	(1 << 1)
#define PSTATE_AM_BIT	(1 << 3)

#define PSTATE_AG_BIT	(1 << 0)
#define PSTATE_IG_BIT	(1 << 11)
#define PSTATE_MG_BIT	(1 << 10)

#define PSTATE_PRIV_BIT	(1 << 2)
#define PSTATE_PEF_BIT	(1 << 4)

#define TSTATE_PSTATE_SHIFT	8
#define TSTATE_PRIV_BIT		(PSTATE_PRIV_BIT << TSTATE_PSTATE_SHIFT)
#define TSTATE_IE_BIT		(PSTATE_IE_BIT << TSTATE_PSTATE_SHIFT)
#define TSTATE_PEF_BIT		(PSTATE_PEF_BIT << TSTATE_PSTATE_SHIFT)

#define TSTATE_CWP_MASK		0x1f

#define WSTATE_NORMAL(n)	(n)
#define WSTATE_OTHER(n)		((n) << 3)

/*
 * The following definitions concern the UPA_CONFIG register on US and the
 * FIREPLANE_CONFIG register on US3.
 */
#define ICBUS_CONFIG_MID_SHIFT    17

#endif

/** @}
 */
