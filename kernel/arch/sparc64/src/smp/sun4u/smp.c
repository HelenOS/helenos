/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

	if (waitq_sleep_timeout(&ap_completion_wq, 1000000,
	    SYNCH_FLAGS_NONE, NULL) == ETIMEOUT)
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
