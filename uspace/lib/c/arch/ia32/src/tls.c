/*
 * Copyright (c) 2006 Ondrej Palkovsky
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
#include <sys/types.h>
#include <align.h>

tcb_t *tls_alloc_arch(void **data, size_t size)
{
	return tls_alloc_variant_2(data, size);
}

void tls_free_arch(tcb_t *tcb, size_t size)
{
	tls_free_variant_2(tcb, size);
}

/*
 * Rtld TLS support
 */

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

void __attribute__ ((__regparm__ (1)))
    *___tls_get_addr(tls_index *ti);

void __attribute__ ((__regparm__ (1)))
    *___tls_get_addr(tls_index *ti)
{
	size_t tls_size;
	uint8_t *tls;

	/* Calculate size of TLS block */
	tls_size = ALIGN_UP(&_tbss_end - &_tdata_start, &_tls_alignment);

	/* The TLS block is just before TCB */
	tls = (uint8_t *)__tcb_get() - tls_size;

	return tls + ti->ti_offset;
}

/** @}
 */
