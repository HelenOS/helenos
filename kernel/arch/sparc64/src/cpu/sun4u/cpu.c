/*
 * Copyright (c) 2005 Jakub Jermar
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

#include <arch/cpu_family.h>
#include <cpu.h>
#include <arch.h>
#include <genarch/ofw/ofw_tree.h>
#include <arch/drivers/tick.h>
#include <print.h>
#include <arch/cpu_node.h>

/**
 * Finds out the clock frequency of the current CPU.
 *
 * @param node	node representing the current CPU in the OFW tree
 * @return 	clock frequency if "node" is the current CPU and no error
 *		occurs,	-1 if "node" is not the current CPU or on error
 */
static int find_cpu_frequency(ofw_tree_node_t *node)
{
	ofw_tree_property_t *prop;
	uint32_t mid;

	/* 'upa-portid' for US, 'portid' for US-III, 'cpuid' for US-IV */
	prop = ofw_tree_getprop(node, "upa-portid");
	if ((!prop) || (!prop->value))
		prop = ofw_tree_getprop(node, "portid");
	if ((!prop) || (!prop->value))
		prop = ofw_tree_getprop(node, "cpuid");

	if (prop && prop->value) {
		mid = *((uint32_t *) prop->value);
		if (mid == CPU->arch.mid) {
			prop = ofw_tree_getprop(node, "clock-frequency");
			if (prop && prop->value) {
				return *((uint32_t *) prop->value);
			}
		}
	}

	return -1;
}

/** Perform sparc64 specific initialization of the processor structure for the
 * current processor.
 */
void cpu_arch_init(void)
{
	ofw_tree_node_t *node;
	uint32_t clock_frequency = 0;

	CPU->arch.mid = read_mid();

	/*
	 * Detect processor frequency.
	 */
	if (is_us() || is_us_iii()) {
		node = ofw_tree_find_child_by_device_type(cpus_parent(), "cpu");
		while (node) {
			int f = find_cpu_frequency(node);
			if (f != -1)
				clock_frequency = (uint32_t) f;
			node = ofw_tree_find_peer_by_device_type(node, "cpu");
		}
	} else if (is_us_iv()) {
		node = ofw_tree_find_child(cpus_parent(), "cmp");
		while (node) {
			int f;
			f = find_cpu_frequency(
				ofw_tree_find_child(node, "cpu@0"));
			if (f != -1)
				clock_frequency = (uint32_t) f;
			f = find_cpu_frequency(
				ofw_tree_find_child(node, "cpu@1"));
			if (f != -1)
				clock_frequency = (uint32_t) f;
			node = ofw_tree_find_peer_by_name(node, "cmp");
		}
	}

	CPU->arch.clock_frequency = clock_frequency;
	tick_init();
}

/** Read version information from the current processor. */
void cpu_identify(void)
{
	CPU->arch.ver.value = ver_read();
}

/** Print version information for a processor.
 *
 * This function is called by the bootstrap processor.
 *
 * @param m Processor structure of the CPU for which version information is to
 * 	be printed.
 */
void cpu_print_report(cpu_t *m)
{
	const char *manuf;
	const char *impl;

	switch (m->arch.ver.manuf) {
	case MANUF_FUJITSU:
		manuf = "Fujitsu";
		break;
	case MANUF_ULTRASPARC:
		manuf = "UltraSPARC";
		break;
	case MANUF_SUN:
	    	manuf = "Sun";
		break;
	default:
		manuf = "Unknown";
		break;
	}

	switch (CPU->arch.ver.impl) {
	case IMPL_ULTRASPARCI:
		impl = "UltraSPARC I";
		break;
	case IMPL_ULTRASPARCII:
		impl = "UltraSPARC II";
		break;
	case IMPL_ULTRASPARCII_I:
		impl = "UltraSPARC IIi";
		break;
	case IMPL_ULTRASPARCII_E:
		impl = "UltraSPARC IIe";
		break;
	case IMPL_ULTRASPARCIII:
		impl = "UltraSPARC III";
		break;
	case IMPL_ULTRASPARCIII_PLUS:
		impl = "UltraSPARC III+";
		break;
	case IMPL_ULTRASPARCIII_I:
		impl = "UltraSPARC IIIi";
		break;
	case IMPL_ULTRASPARCIV:
		impl = "UltraSPARC IV";
		break;
	case IMPL_ULTRASPARCIV_PLUS:
		impl = "UltraSPARC IV+";
		break;
	case IMPL_SPARC64V:
		impl = "SPARC 64V";
		break;
	default:
		impl = "Unknown";
		break;
	}

	printf("cpu%d: manuf=%s, impl=%s, mask=%d (%d MHz)\n", m->id, manuf,
		impl, m->arch.ver.mask, m->arch.clock_frequency / 1000000);
}

/** @}
 */
