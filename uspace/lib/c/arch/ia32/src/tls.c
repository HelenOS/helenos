/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcamd64 amd64
 * @ingroup lc
 * @{
 */
/** @file
 * @ingroup libcia32
 */

#include <tls.h>
#include <stddef.h>
#include <stdint.h>
#include <align.h>

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

tcb_t *tls_alloc_arch(size_t size, size_t align)
{
	return tls_alloc_variant_2(size, align);
}

void tls_free_arch(tcb_t *tcb, size_t size, size_t align)
{
	tls_free_variant_2(tcb, size, align);
}

/*
 * Rtld TLS support
 */

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

void __attribute__((__regparm__(1)))
    *___tls_get_addr(tls_index *ti);

void __attribute__((__regparm__(1)))
    *___tls_get_addr(tls_index *ti)
{
	uint8_t *tls;

#ifdef CONFIG_RTLD
	if (runtime_env != NULL) {
		return rtld_tls_get_addr(runtime_env, __tcb_get(),
		    ti->ti_module, ti->ti_offset);
	}
#endif
	/* Get address of static TLS block */
	tls = tls_get();
	return tls + ti->ti_offset;
}

/** @}
 */
