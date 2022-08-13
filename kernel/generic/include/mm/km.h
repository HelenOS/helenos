/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */
/** @file
 */

#ifndef KERN_KM_H_
#define KERN_KM_H_

#include <typedefs.h>
#include <mm/frame.h>

#define KM_NATURAL_ALIGNMENT	-1U

extern void km_identity_init(void);
extern void km_non_identity_init(void);

extern void km_non_identity_span_add(uintptr_t, size_t);

extern uintptr_t km_page_alloc(size_t, size_t);
extern void km_page_free(uintptr_t, size_t);

extern bool km_is_non_identity(uintptr_t);

extern uintptr_t km_map(uintptr_t, size_t, size_t, unsigned int);
extern void km_unmap(uintptr_t, size_t);

extern uintptr_t km_temporary_page_get(uintptr_t *, frame_flags_t);
extern void km_temporary_page_put(uintptr_t);

#endif

/** @}
 */
