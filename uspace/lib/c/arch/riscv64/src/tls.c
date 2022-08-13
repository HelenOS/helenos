/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <tls.h>
#include <stddef.h>

tcb_t *tls_alloc_arch(size_t size, size_t align)
{
	return tls_alloc_variant_2(size, align);
}

void tls_free_arch(tcb_t *tcb, size_t size, size_t align)
{
	tls_free_variant_2(tcb, size, align);
}

/** @}
 */
