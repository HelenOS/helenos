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

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#include <smp/smp.h>
#include <genarch/ofw/ofw_tree.h>
#include <cpu.h>
#include <arch/cpu_family.h>
#include <arch/cpu.h>
#include <arch.h>
#include <config.h>
#include <macros.h>
#include <stdint.h>
#include <synch/waitq.h>
#include <log.h>
#include <arch/cpu_node.h>

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
	unsigned int cnt = 0;

	if (is_us() || is_us_iii()) {
		node = ofw_tree_find_child_by_device_type(cpus_parent(), "cpu");
		while (node) {
			cnt++;
			node = ofw_tree_find_peer_by_device_type(node, "cpu");
		}
	} else if (is_us_iv()) {
		node = ofw_tree_find_child(cpus_parent(), "cmp");
		while (node) {
			cnt += 2;
			node = ofw_tree_find_peer_by_name(node, "cmp");
		}
	}

	config.cpu_count = max(1, cnt);
}

/**
 * Wakes up the CPU which is represented by the "node" OFW tree node.
 * If "node" represents the current CPU, calling the function has
 * no effect.
 */
static void wakeup_cpu(ofw_tree_node_t *node)
{
	uint32_t mid;
	ofw_tree_property_t *prop;

	/* 'upa-portid' for US, 'portid' for US-III, 'cpuid' for US-IV */
	prop = ofw_tree_getprop(node, "upa-portid");
	if ((!prop) || (!prop->value))
		prop = ofw_tree_getprop(node, "portid");
	if ((!prop) || (!prop->value))
		prop = ofw_tree_getprop(node, "cpuid");

	if (!prop || prop->value == NULL)
		return;

	mid = *((uint32_t *) prop->value);
	if (CPU->arch.mid == mid)
		return;

	waking_up_mid = mid;

	if (semaphore_down_timeout(&ap_completion_semaphore, 1000000) != EOK)
		log(LF_ARCH, LVL_NOTE, "%s: waiting for processor (mid = %" PRIu32
		    ") timed out", __func__, mid);
}

/** Wake application processors up. */
void kmp(void *arg)
{
	ofw_tree_node_t *node;
	int i;

	if (is_us() || is_us_iii()) {
		node = ofw_tree_find_child_by_device_type(cpus_parent(), "cpu");
		for (i = 0; node;
		    node = ofw_tree_find_peer_by_device_type(node, "cpu"), i++)
			wakeup_cpu(node);
	} else if (is_us_iv()) {
		node = ofw_tree_find_child(cpus_parent(), "cmp");
		while (node) {
			wakeup_cpu(ofw_tree_find_child(node, "cpu@0"));
			wakeup_cpu(ofw_tree_find_child(node, "cpu@1"));
			node = ofw_tree_find_peer_by_name(node, "cmp");
		}
	}
}

/** @}
 */
