/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm64
 */
/** @file
 * @brief Thread-local storage.
 */

#include <tls.h>
#include <stddef.h>

tcb_t *tls_alloc_arch(size_t size, size_t align)
{
	return tls_alloc_variant_1(size, align);
}

void tls_free_arch(tcb_t *tcb, size_t size, size_t align)
{
	tls_free_variant_1(tcb, size, align);
}

/** @}
 */
