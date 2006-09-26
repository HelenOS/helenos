/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <arch/asm.h>
#include <cpu.h>
#include <arch.h>
#include <print.h>
#include <arch/register.h>
#include <genarch/ofw/ofw_tree.h>
#include <arch/types.h>
#include <arch/drivers/tick.h>

/** Perform sparc64 specific initialization of the processor structure for the current processor. */
void cpu_arch_init(void)
{
	ofw_tree_node_t *node;
	uint32_t mid;
	uint32_t clock_frequency = 0;
	upa_config_t upa_config;
	
	upa_config.value = upa_config_read();
	node = ofw_tree_find_child_by_device_type(ofw_tree_lookup("/"), "cpu");
	while (node) {
		ofw_tree_property_t *prop;
		
		prop = ofw_tree_getprop(node, "upa-portid");
		if (prop && prop->value) {
			mid = *((uint32_t *) prop->value);
			if (mid == upa_config.mid) {
				prop = ofw_tree_getprop(node, "clock-frequency");
				if (prop && prop->value)
					clock_frequency = *((uint32_t *) prop->value);
			}
		}
		node = ofw_tree_find_peer_by_device_type(node, "cpu");
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
 * @param m Processor structure of the CPU for which version information is to be printed.
 */
void cpu_print_report(cpu_t *m)
{
	char *manuf, *impl;

	switch (CPU->arch.ver.manuf) {
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

	printf("cpu%d: manuf=%s, impl=%s, mask=%d (%dMHz)\n",
		CPU->id, manuf, impl, CPU->arch.ver.mask, CPU->arch.clock_frequency/1000000);
}

/** @}
 */
