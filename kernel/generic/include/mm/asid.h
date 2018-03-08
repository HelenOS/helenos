/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

/*
 * This is generic interface for managing
 * Address Space IDentifiers (ASIDs).
 */

#ifndef KERN_ASID_H_
#define KERN_ASID_H_

#ifndef __ASSEMBLER__

#include <arch/mm/asid.h>
#include <synch/spinlock.h>
#include <adt/list.h>
#include <mm/as.h>

#endif

#define ASID_KERNEL	0
#define ASID_INVALID	1
#define ASID_START	2
#define ASID_MAX	ASID_MAX_ARCH

#ifndef __ASSEMBLER__

#define ASIDS_ALLOCABLE	((ASID_MAX + 1) - ASID_START)

SPINLOCK_EXTERN(asidlock);
extern link_t as_with_asid_head;

#ifndef asid_get
extern asid_t asid_get(void);
#endif /* !def asid_get */

#ifndef asid_put
extern void asid_put(asid_t asid);
#endif /* !def asid_put */

#ifndef asid_install
extern void asid_install(as_t *as);
#endif /* !def asid_install */

#ifndef asid_find_free
extern asid_t asid_find_free(void);
#endif /* !def asid_find_free */

#ifndef asid_put_arch
extern void asid_put_arch(asid_t asid);
#endif /* !def asid_put_arch */

#endif

#endif

/** @}
 */
