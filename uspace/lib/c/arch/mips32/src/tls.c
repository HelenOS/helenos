/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcmips32
 * @{
 */
/** @file
 * @ingroup libcmips32
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
