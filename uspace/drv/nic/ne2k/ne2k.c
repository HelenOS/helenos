/*
 * Copyright (c) 2011 Martin Decky
 * Copyright (c) 2011 Radim Vansa
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

/**
 * @addtogroup drv_ne2k
 * @brief Novell NE2000 NIC driver
 * @{
 */
/**
 * @file
 * @brief Bridge between NICF, DDF and business logic for the NIC
 */

#include <stdio.h>
#include <errno.h>
#include <device/hw_res.h>
#include <stdlib.h>
#include <str_error.h>
#include <async.h>
#include "dp8390.h"

#define NAME  "ne2k"

/** Return the ISR from the interrupt call.
 *
 * @param[in] call The interrupt call.
 *
 */
#define IRQ_GET_ISR(call)  ((int) IPC_GET_ARG2(call))

/** Return the TSR from the interrupt call.
 *
 * @param[in] call The interrupt call.
 *
 */
#define IRQ_GET_TSR(call)  ((int) IPC_GET_ARG3(call))

#define DRIVER_DATA(dev) ((nic_t *) ddf_dev_data_get(dev))
#define NE2K(device) ((ne2k_t *) nic_get_specific(DRIVER_DATA(device)))

static irq_pio_range_t ne2k_ranges_prototype[] = {
	{
		.base = 0,
		.size = NE2K_IO_SIZE,
	}
};

/** NE2000 kernel interrupt command sequence.
 *
 */
static irq_cmd_t ne2k_cmds_prototype[] = {
	{
		/* Read Interrupt Status Register */
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,
		.dstarg = 2
	},
	{
		/* Mask supported interrupt causes */
		.cmd = CMD_AND,
		.value = (ISR_PRX | ISR_PTX | ISR_RXE | ISR_TXE | ISR_OVW |
		    ISR_CNT | ISR_RDC),
		.srcarg = 2,
		.dstarg = 3,
	},
	{
		/* Predicate for accepting the interrupt */
		.cmd = CMD_PREDICATE,
		.value = 4,
		.srcarg = 3
	},
	{
		/*
		 * Mask future interrupts via
		 * Interrupt Mask Register
		 */
		.cmd = CMD_PIO_WRITE_8,
		.addr = NULL,
		.value = 0
	},
	{
		/* Acknowledge the current interrupt */
		.cmd = CMD_PIO_WRITE_A_8,
		.addr = NULL,
		.srcarg = 3
	},
	{
		/* Read Transmit Status Register */
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,
		.dstarg = 3
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static void ne2k_interrupt_handler(ipc_call_t *, ddf_dev_t *);

static errno_t ne2k_register_interrupt(nic_t *nic_data, cap_handle_t *handle)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);

	if (ne2k->code.cmdcount == 0) {
		irq_pio_range_t *ne2k_ranges;
		irq_cmd_t *ne2k_cmds;

		ne2k_ranges = malloc(sizeof(ne2k_ranges_prototype));
		if (!ne2k_ranges)
			return ENOMEM;
		memcpy(ne2k_ranges, ne2k_ranges_prototype,
		    sizeof(ne2k_ranges_prototype));
		ne2k_ranges[0].base = (uintptr_t) ne2k->base_port;

		ne2k_cmds = malloc(sizeof(ne2k_cmds_prototype));
		if (!ne2k_cmds) {
			free(ne2k_ranges);
			return ENOMEM;
		}
		memcpy(ne2k_cmds, ne2k_cmds_prototype,
		    sizeof(ne2k_cmds_prototype));
		ne2k_cmds[0].addr = ne2k->base_port + DP_ISR;
		ne2k_cmds[3].addr = ne2k->base_port + DP_IMR;
		ne2k_cmds[4].addr = ne2k_cmds[0].addr;
		ne2k_cmds[5].addr = ne2k->base_port + DP_TSR;

		ne2k->code.rangecount = sizeof(ne2k_ranges_prototype) /
		    sizeof(irq_pio_range_t);
		ne2k->code.ranges = ne2k_ranges;

		ne2k->code.cmdcount = sizeof(ne2k_cmds_prototype) /
		    sizeof(irq_cmd_t);
		ne2k->code.cmds = ne2k_cmds;
	}

	return register_interrupt_handler(nic_get_ddf_dev(nic_data),
		ne2k->irq, ne2k_interrupt_handler, &ne2k->code, handle);
}

static ddf_dev_ops_t ne2k_dev_ops;

static void ne2k_dev_cleanup(ddf_dev_t *dev)
{
	if (ddf_dev_data_get(dev) != NULL) {
		ne2k_t *ne2k = NE2K(dev);
		if (ne2k) {
			free(ne2k->code.ranges);
			free(ne2k->code.cmds);
		}
		nic_unbind_and_destroy(dev);
	}
}

static errno_t ne2k_dev_init(nic_t *nic_data)
{
	/* Get HW resources */
	hw_res_list_parsed_t hw_res_parsed;
	hw_res_list_parsed_init(&hw_res_parsed);

	errno_t rc = nic_get_resources(nic_data, &hw_res_parsed);

	if (rc != EOK)
		goto failed;

	if (hw_res_parsed.irqs.count == 0) {
		rc = EINVAL;
		goto failed;
	}

	if (hw_res_parsed.io_ranges.count == 0) {
		rc = EINVAL;
		goto failed;
	}

	if (hw_res_parsed.io_ranges.ranges[0].size < NE2K_IO_SIZE) {
		rc = EINVAL;
		goto failed;
	}

	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);
	ne2k->irq = hw_res_parsed.irqs.irqs[0];

	addr_range_t regs = hw_res_parsed.io_ranges.ranges[0];
	ne2k->base_port = RNGABSPTR(regs);

	hw_res_list_parsed_clean(&hw_res_parsed);

	/* Enable programmed I/O */
	if (pio_enable_range(&regs, &ne2k->port) != EOK)
		return EADDRNOTAVAIL;

	ne2k->data_port = ne2k->port + NE2K_DATA;
	ne2k->receive_configuration = RCR_AB | RCR_AM;
	ne2k->probed = false;
	ne2k->up = false;

	/* Find out whether the device is present. */
	if (ne2k_probe(ne2k) != EOK)
		return ENOENT;

	ne2k->probed = true;

	if (ne2k_register_interrupt(nic_data, NULL) != EOK)
		return EINVAL;

	return EOK;

failed:
	hw_res_list_parsed_clean(&hw_res_parsed);
	return rc;
}

void ne2k_interrupt_handler(ipc_call_t *call, ddf_dev_t *dev)
{
	nic_t *nic_data = DRIVER_DATA(dev);
	ne2k_interrupt(nic_data, IRQ_GET_ISR(*call), IRQ_GET_TSR(*call));
}

static errno_t ne2k_on_activating(nic_t *nic_data)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);

	if (!ne2k->up) {
		errno_t rc = ne2k_up(ne2k);
		if (rc != EOK)
			return rc;

		rc = hw_res_enable_interrupt(ne2k->parent_sess, ne2k->irq);
		if (rc != EOK) {
			ne2k_down(ne2k);
			return rc;
		}
	}
	return EOK;
}

static errno_t ne2k_on_stopping(nic_t *nic_data)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);

	(void) hw_res_disable_interrupt(ne2k->parent_sess, ne2k->irq);
	ne2k->receive_configuration = RCR_AB | RCR_AM;
	ne2k_down(ne2k);
	return EOK;
}

static errno_t ne2k_set_address(ddf_fun_t *fun, const nic_address_t *address)
{
	nic_t *nic_data = DRIVER_DATA(ddf_fun_get_dev(fun));
	errno_t rc = nic_report_address(nic_data, address);
	if (rc != EOK) {
		return EINVAL;
	}
	/* Note: some frame with previous physical address may slip to NIL here
	 * (for a moment the filtering is not exact), but ethernet should be OK with
	 * that. Some frames may also be lost, but this is not a problem.
	 */
	ne2k_set_physical_address((ne2k_t *) nic_get_specific(nic_data), address);
	return EOK;
}

static errno_t ne2k_on_unicast_mode_change(nic_t *nic_data,
	nic_unicast_mode_t new_mode,
	const nic_address_t *address_list, size_t address_count)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);
	switch (new_mode) {
	case NIC_UNICAST_BLOCKED:
		ne2k_set_promisc_phys(ne2k, false);
		nic_report_hw_filtering(nic_data, 0, -1, -1);
		return EOK;
	case NIC_UNICAST_DEFAULT:
		ne2k_set_promisc_phys(ne2k, false);
		nic_report_hw_filtering(nic_data, 1, -1, -1);
		return EOK;
	case NIC_UNICAST_LIST:
		ne2k_set_promisc_phys(ne2k, true);
		nic_report_hw_filtering(nic_data, 0, -1, -1);
		return EOK;
	case NIC_UNICAST_PROMISC:
		ne2k_set_promisc_phys(ne2k, true);
		nic_report_hw_filtering(nic_data, 1, -1, -1);
		return EOK;
	default:
		return ENOTSUP;
	}
}

static errno_t ne2k_on_multicast_mode_change(nic_t *nic_data,
	nic_multicast_mode_t new_mode,
	const nic_address_t *address_list, size_t address_count)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);
	switch (new_mode) {
	case NIC_MULTICAST_BLOCKED:
		ne2k_set_accept_mcast(ne2k, false);
		nic_report_hw_filtering(nic_data, -1, 1, -1);
		return EOK;
	case NIC_MULTICAST_LIST:
		ne2k_set_accept_mcast(ne2k, true);
		ne2k_set_mcast_hash(ne2k,
			nic_mcast_hash(address_list, address_count));
		nic_report_hw_filtering(nic_data, -1, 0, -1);
		return EOK;
	case NIC_MULTICAST_PROMISC:
		ne2k_set_accept_mcast(ne2k, true);
		ne2k_set_mcast_hash(ne2k, 0xFFFFFFFFFFFFFFFFllu);
		nic_report_hw_filtering(nic_data, -1, 1, -1);
		return EOK;
	default:
		return ENOTSUP;
	}
}

static errno_t ne2k_on_broadcast_mode_change(nic_t *nic_data,
	nic_broadcast_mode_t new_mode)
{
	ne2k_t *ne2k = (ne2k_t *) nic_get_specific(nic_data);
	switch (new_mode) {
	case NIC_BROADCAST_BLOCKED:
		ne2k_set_accept_bcast(ne2k, false);
		return EOK;
	case NIC_BROADCAST_ACCEPTED:
		ne2k_set_accept_bcast(ne2k, true);
		return EOK;
	default:
		return ENOTSUP;
	}
}

static errno_t ne2k_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun;

	/* Allocate driver data for the device. */
	nic_t *nic_data = nic_create_and_bind(dev);
	if (nic_data == NULL)
		return ENOMEM;

	nic_set_send_frame_handler(nic_data, ne2k_send);
	nic_set_state_change_handlers(nic_data,
		ne2k_on_activating, NULL, ne2k_on_stopping);
	nic_set_filtering_change_handlers(nic_data,
		ne2k_on_unicast_mode_change, ne2k_on_multicast_mode_change,
		ne2k_on_broadcast_mode_change, NULL, NULL);

	ne2k_t *ne2k = malloc(sizeof(ne2k_t));
	if (NULL != ne2k) {
		memset(ne2k, 0, sizeof(ne2k_t));
		nic_set_specific(nic_data, ne2k);
	} else {
		nic_unbind_and_destroy(dev);
		return ENOMEM;
	}

	ne2k->dev = dev;
	ne2k->parent_sess = ddf_dev_parent_sess_get(dev);
	if (ne2k->parent_sess == NULL) {
		ne2k_dev_cleanup(dev);
		return ENOMEM;
	}

	errno_t rc = ne2k_dev_init(nic_data);
	if (rc != EOK) {
		ne2k_dev_cleanup(dev);
		return rc;
	}

	rc = nic_report_address(nic_data, &ne2k->mac);
	if (rc != EOK) {
		ne2k_dev_cleanup(dev);
		return rc;
	}

	fun = ddf_fun_create(nic_get_ddf_dev(nic_data), fun_exposed, "port0");
	if (fun == NULL) {
		ne2k_dev_cleanup(dev);
		return ENOMEM;
	}

	nic_set_ddf_fun(nic_data, fun);
	ddf_fun_set_ops(fun, &ne2k_dev_ops);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_fun_destroy(fun);
		ne2k_dev_cleanup(dev);
		return rc;
	}

	rc = ddf_fun_add_to_category(fun, DEVICE_CATEGORY_NIC);
	if (rc != EOK) {
		ddf_fun_unbind(fun);
		ddf_fun_destroy(fun);
		return rc;
	}

	return EOK;
}

static nic_iface_t ne2k_nic_iface = {
	.set_address = ne2k_set_address
};

static driver_ops_t ne2k_driver_ops = {
	.dev_add = ne2k_dev_add
};

static driver_t ne2k_driver = {
	.name = NAME,
	.driver_ops = &ne2k_driver_ops
};

int main(int argc, char *argv[])
{
	printf("%s: HelenOS NE 2000 network adapter driver\n", NAME);

	nic_driver_init(NAME);
	nic_driver_implement(&ne2k_driver_ops, &ne2k_dev_ops, &ne2k_nic_iface);

	return ddf_driver_main(&ne2k_driver);
}

/** @}
 */
