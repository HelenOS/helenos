/*
 * Copyright (c) 2016 Jakub Jermar
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

#include <arch.h>
#include <arch/arch.h>
#include <mm/slab.h>
#include <sysinfo/sysinfo.h>
#include <config.h>
#include <arch/proc/thread.h>
#include <arch/trap/regwin.h>
#include <debug.h>

#define SPARC64_ARCH_OP(op)	ARCH_STRUCT_OP(sparc64_ops, op)

static void sparc64_pre_mm_init(void);
static void sparc64_post_mm_init(void);
static void sparc64_post_cpu_init(void);
static void sparc64_pre_smp_init(void);
static void sparc64_post_smp_init(void);

arch_ops_t sparc64_arch_ops = {
	.pre_mm_init = sparc64_pre_mm_init,
	.post_mm_init = sparc64_post_mm_init,
	.post_cpu_init = sparc64_post_cpu_init,
	.pre_smp_init = sparc64_pre_smp_init,
	.post_smp_init = sparc64_post_smp_init,
};

arch_ops_t *arch_ops = &sparc64_arch_ops;

void sparc64_pre_mm_init(void)
{
	SPARC64_ARCH_OP(pre_mm_init);
}

void sparc64_post_mm_init(void)
{
	SPARC64_ARCH_OP(post_mm_init);

	if (config.cpu_active == 1) {
		static_assert(UWB_SIZE <= UWB_ALIGNMENT, "");
		/* Create slab cache for the userspace window buffers */
		uwb_cache = slab_cache_create("uwb_cache", UWB_SIZE,
		    UWB_ALIGNMENT, NULL, NULL, SLAB_CACHE_MAGDEFERRED);

		/* Copy boot arguments */
		ofw_tree_node_t *options = ofw_tree_lookup("/options");
		if (options) {
			ofw_tree_property_t *prop;

			prop = ofw_tree_getprop(options, "boot-args");
			if (prop && prop->value) {
				str_ncpy(bargs, CONFIG_BOOT_ARGUMENTS_BUFLEN,
				    prop->value, prop->size);
			}
		}
	}
}

void sparc64_post_cpu_init(void)
{
	SPARC64_ARCH_OP(post_cpu_init);
}

void sparc64_pre_smp_init(void)
{
	SPARC64_ARCH_OP(pre_smp_init);
}

void sparc64_post_smp_init(void)
{
	SPARC64_ARCH_OP(post_smp_init);
}

/** @}
 */
