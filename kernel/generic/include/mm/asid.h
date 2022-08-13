/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
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
