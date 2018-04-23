/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2016 Jiri Svoboda
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
