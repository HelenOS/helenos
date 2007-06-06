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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#include <smp/smp.h>
#include <genarch/ofw/ofw_tree.h>
#include <cpu.h>
#include <arch/cpu.h>
#include <arch.h>
#include <config.h>
#include <macros.h>
#include <arch/types.h>
#include <synch/synch.h>
#include <synch/waitq.h>
#include <print.h>

/**
 * This global variable is used to pick-up application processors
 * from their active loop in start.S. When a processor looping in
 * start.S sees that this variable contains its MID, it can
 * proceed with its initialization.
 *
 * This variable is modified only by the bootstrap processor.
 * Other processors access it read-only.
 */
volatile uint64_t waking_up_mid = (uint64_t) -1;

/** Determine number of processors. */
void smp_init(void)
{
	ofw_tree_node_t *node;
	count_t cnt = 0;
	
	node = ofw_tree_find_child_by_device_type(ofw_tree_lookup("/"), "cpu");
	while (node) {
		cnt++;
		node = ofw_tree_find_peer_by_device_type(node, "cpu");
	}
	
	config.cpu_count = max(1, cnt);
}

/** Wake application processors up. */
void kmp(void *arg)
{
	ofw_tree_node_t *node;
	int i;
	
	node = ofw_tree_find_child_by_device_type(ofw_tree_lookup("/"), "cpu");
	for (i = 0; node; node = ofw_tree_find_peer_by_device_type(node, "cpu"), i++) {
		uint32_t mid;
		ofw_tree_property_t *prop;
		
		prop = ofw_tree_getprop(node, "upa-portid");
		if (!prop || !prop->value)
			continue;
		
		mid = *((uint32_t *) prop->value);
		if (CPU->arch.mid == mid) {
			/*
			 * Skip the current CPU.
			 */
			continue;
		}

		/*
		 * Processor with ID == mid can proceed with its initialization.
		 */
		waking_up_mid = mid;
		
		if (waitq_sleep_timeout(&ap_completion_wq, 1000000, SYNCH_FLAGS_NONE) == ESYNCH_TIMEOUT)
			printf("%s: waiting for processor (mid = %d) timed out\n",
			    __func__, mid);
	}
}

/** @}
 */
