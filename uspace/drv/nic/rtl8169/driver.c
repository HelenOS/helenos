/*
 * Copyright (c) 2014 Agnieszka Tabaka
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

/* XXX Fix this */
#define _DDF_DATA_IMPLANT

#include <assert.h>
#include <errno.h>
#include <align.h>
#include <byteorder.h>
#include <libarch/barrier.h>

#include <as.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>
#include <io/log.h>
#include <nic.h>
#include <pci_dev_iface.h>

#include <ipc/irc.h>
#include <sysinfo.h>
#include <ipc/ns.h>

#include <str.h>

#include "defs.h"
#include "driver.h"

/** Global mutex for work with shared irq structure */
FIBRIL_MUTEX_INITIALIZE(irq_reg_lock);

static int rtl8169_set_addr(ddf_fun_t *fun, const nic_address_t *addr);
static int rtl8169_get_device_info(ddf_fun_t *fun, nic_device_info_t *info);
static int rtl8169_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state);
static int rtl8169_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role);
static int rtl8169_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t role);
static int rtl8169_pause_get(ddf_fun_t *fun, nic_result_t *we_send, 
    nic_result_t *we_receive, uint16_t *time);
static int rtl8169_pause_set(ddf_fun_t *fun, int allow_send, int allow_receive, 
    uint16_t time);
static int rtl8169_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement);
static int rtl8169_autoneg_disable(ddf_fun_t *fun);
static int rtl8169_autoneg_probe(ddf_fun_t *fun, uint32_t *advertisement,
    uint32_t *their_adv, nic_result_t *result, nic_result_t *their_result);
static int rtl8169_autoneg_restart(ddf_fun_t *fun);
static int rtl8169_defective_get_mode(ddf_fun_t *fun, uint32_t *mode);
static int rtl8169_defective_set_mode(ddf_fun_t *fun, uint32_t mode);
static int rtl8169_on_activated(nic_t *nic_data);
static int rtl8169_on_stopped(nic_t *nic_data);
static void rtl8169_send_frame(nic_t *nic_data, void *data, size_t size);
static void rtl8169_irq_handler(ddf_dev_t *dev, ipc_callid_t iid,
    ipc_call_t *icall);
static inline int rtl8169_register_int_handler(nic_t *nic_data);
static inline void rtl8169_get_hwaddr(rtl8169_t *rtl8169, nic_address_t *addr);
static inline void rtl8169_set_hwaddr(rtl8169_t *rtl8169, nic_address_t *addr);

static void rtl8169_reset(rtl8169_t *rtl8169);
static int rtl8169_get_resource_info(ddf_dev_t *dev);
static int rtl8169_fill_resource_info(ddf_dev_t *dev, const hw_res_list_parsed_t *hw_resources);
static rtl8169_t *rtl8169_create_dev_data(ddf_dev_t *dev);

static int rtl8169_unicast_set(nic_t *nic_data, nic_unicast_mode_t mode,
    const nic_address_t *, size_t);
static int rtl8169_multicast_set(nic_t *nic_data, nic_multicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count);
static int rtl8169_broadcast_set(nic_t *nic_data, nic_broadcast_mode_t mode);


/** Network interface options for RTL8169 card driver */
static nic_iface_t rtl8169_nic_iface = {
	.set_address = &rtl8169_set_addr,
	.get_device_info = &rtl8169_get_device_info,
	.get_cable_state = &rtl8169_get_cable_state,
	.get_operation_mode = &rtl8169_get_operation_mode,
	.set_operation_mode = &rtl8169_set_operation_mode,

	.get_pause = &rtl8169_pause_get,
	.set_pause = &rtl8169_pause_set,

	.autoneg_enable = &rtl8169_autoneg_enable,
	.autoneg_disable = &rtl8169_autoneg_disable,
	.autoneg_probe = &rtl8169_autoneg_probe,
	.autoneg_restart = &rtl8169_autoneg_restart,

	.defective_get_mode = &rtl8169_defective_get_mode,
	.defective_set_mode = &rtl8169_defective_set_mode,
};

irq_pio_range_t rtl8169_irq_pio_ranges[] = {
	{
		.base = 0,
		.size = RTL8169_IO_SIZE
	}
};

irq_cmd_t rtl8169_irq_commands[] = {
	{
		/* Get the interrupt status */
		.cmd = CMD_PIO_READ_16,
		.addr = NULL,
		.dstarg = 2
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 3,
		.srcarg = 2
	},
	{
		/* Mark interrupts as solved */
		.cmd = CMD_PIO_WRITE_16,
		.addr = NULL,
		.value = 0xFFFF
	},
	{
		/* Disable interrupts until interrupt routine is finished */
		.cmd = CMD_PIO_WRITE_16,
		.addr = NULL,
		.value = 0x0000
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** Interrupt code definition */
irq_code_t rtl8169_irq_code = {
	.rangecount = sizeof(rtl8169_irq_pio_ranges) / sizeof(irq_pio_range_t),
	.ranges = rtl8169_irq_pio_ranges,
	.cmdcount = sizeof(rtl8169_irq_commands) / sizeof(irq_cmd_t),
	.cmds = rtl8169_irq_commands
};

/** Basic device operations for RTL8169 driver */
static ddf_dev_ops_t rtl8169_dev_ops;

static int rtl8169_dev_add(ddf_dev_t *dev);

/** Basic driver operations for RTL8169 driver */
static driver_ops_t rtl8169_driver_ops = {
	.dev_add = &rtl8169_dev_add,
};

/** Driver structure for RTL8169 driver */
static driver_t rtl8169_driver = {
	.name = NAME,
	.driver_ops = &rtl8169_driver_ops
};

static int rtl8169_get_resource_info(ddf_dev_t *dev)
{
	assert(dev);

	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	assert(nic_data);

	hw_res_list_parsed_t hw_res_parsed;
	hw_res_list_parsed_init(&hw_res_parsed);

	/* Get hw resources form parent driver */
	int rc = nic_get_resources(nic_data, &hw_res_parsed);
	if (rc != EOK)
		return rc;

	/* Fill resources information to the device */
	int ret = rtl8169_fill_resource_info(dev, &hw_res_parsed);
	hw_res_list_parsed_clean(&hw_res_parsed);

	return ret;
}

static int rtl8169_fill_resource_info(ddf_dev_t *dev, const hw_res_list_parsed_t
    *hw_resources)
{
	assert(dev);
	assert(hw_resources);

	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_dev(dev));
	assert(rtl8169);

	if (hw_resources->irqs.count != 1) {
		ddf_msg(LVL_ERROR, "%s device: unexpected irq count", ddf_dev_get_name(dev));
		return EINVAL;
	};

	if (hw_resources->io_ranges.count != 1) {
		ddf_msg(LVL_ERROR, "%s device: unexpected io ranges count", ddf_dev_get_name(dev));
		return EINVAL;
	}

	rtl8169->irq = hw_resources->irqs.irqs[0];
	ddf_msg(LVL_DEBUG, "%s device: irq 0x%x assigned", ddf_dev_get_name(dev), rtl8169->irq);

	rtl8169->regs_phys = (void *)((size_t)RNGABS(hw_resources->io_ranges.ranges[0]));
	if (hw_resources->io_ranges.ranges[0].size < RTL8169_IO_SIZE) {
		ddf_msg(LVL_ERROR, "I/O range assigned to the device "
		    "%s is too small.", ddf_dev_get_name(dev));
		return EINVAL;
	}
	ddf_msg(LVL_DEBUG, "%s device: i/o addr %p assigned.", ddf_dev_get_name(dev), rtl8169->regs_phys);

	return EOK;
}

static int rtl8169_allocate_buffers(rtl8169_t *rtl8169)
{
	int rc;

	ddf_msg(LVL_DEBUG, "Allocating DMA buffer rings");

	/* Allocate TX ring */
	rc = dmamem_map_anonymous(TX_RING_SIZE, DMAMEM_4GiB, 
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->tx_ring_phys,
	    (void **)&rtl8169->tx_ring);

	if (rc != EOK)
		return rc;

	/* Allocate RX ring */
	rc = dmamem_map_anonymous(RX_RING_SIZE, DMAMEM_4GiB, 
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->rx_ring_phys,
	    (void **)&rtl8169->rx_ring);

	if (rc != EOK)
		return rc;

	/* Allocate TX buffers */
	rc = dmamem_map_anonymous(TX_BUFFERS_SIZE, DMAMEM_4GiB, 
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->tx_buff_phys,
	    &rtl8169->tx_buff);

	if (rc != EOK)
		return rc;

	/* Allocate RX buffers */
	rc = dmamem_map_anonymous(RX_BUFFERS_SIZE, DMAMEM_4GiB, 
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->rx_buff_phys,
	    &rtl8169->rx_buff);

	if (rc != EOK)
		return rc;

	return EOK;
}

static rtl8169_t *rtl8169_create_dev_data(ddf_dev_t *dev)
{
	assert(dev);
	assert(!nic_get_from_ddf_dev(dev));

	nic_t *nic_data = nic_create_and_bind(dev);
	if (!nic_data)
		return NULL;

	rtl8169_t *rtl8169 = malloc(sizeof(rtl8169_t));
	if (!rtl8169) {
		nic_unbind_and_destroy(dev);
		return NULL;
	}

	memset(rtl8169, 0, sizeof(rtl8169_t));

	rtl8169->nic_data = nic_data;
	nic_set_specific(nic_data, rtl8169);
	nic_set_send_frame_handler(nic_data, rtl8169_send_frame);
	nic_set_state_change_handlers(nic_data,
		rtl8169_on_activated, NULL, rtl8169_on_stopped);
	nic_set_filtering_change_handlers(nic_data,
		rtl8169_unicast_set, rtl8169_multicast_set, rtl8169_broadcast_set,
		NULL, NULL);

	fibril_mutex_initialize(&rtl8169->rx_lock);
	fibril_mutex_initialize(&rtl8169->tx_lock);

	nic_set_wol_max_caps(nic_data, NIC_WV_BROADCAST, 1);
	nic_set_wol_max_caps(nic_data, NIC_WV_LINK_CHANGE, 1);
	nic_set_wol_max_caps(nic_data, NIC_WV_MAGIC_PACKET, 1);

	return rtl8169;
}

static int rtl8169_dev_initialize(ddf_dev_t *dev)
{
	int ret;

	rtl8169_t *rtl8169 = rtl8169_create_dev_data(dev);
	if (rtl8169 == NULL) {
		ddf_msg(LVL_ERROR, "Not enough memory for initializing %s.", ddf_dev_get_name(dev));
		return ENOMEM;
	}

	ret = rtl8169_get_resource_info(dev);
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Can't obtain H/W resources information");
		goto failed;
	}

	ddf_msg(LVL_DEBUG, "The device is initialized");
	return ret;
	
failed:
	ddf_msg(LVL_ERROR, "The device initialization failed");
//	rtl8139_dev_cleanup(dev);
	return ret;

}

inline static int rtl8169_register_int_handler(nic_t *nic_data)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	rtl8169_irq_code.ranges[0].base = (uintptr_t) rtl8169->regs;
	rtl8169_irq_code.cmds[0].addr = rtl8169->regs + ISR;
	rtl8169_irq_code.cmds[2].addr = rtl8169->regs + ISR;
	rtl8169_irq_code.cmds[3].addr = rtl8169->regs + IMR;
	int rc = register_interrupt_handler(nic_get_ddf_dev(nic_data),
	    rtl8169->irq, rtl8169_irq_handler, &rtl8169_irq_code);

	return rc;
}

static int rtl8169_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun;
	nic_address_t nic_addr;
	int rc;

	assert(dev);
	ddf_msg(LVL_NOTE, "RTL8169_dev_add %s (handle = %zu)",
	    ddf_dev_get_name(dev), ddf_dev_get_handle(dev));


	/* Init structures */
	rc = rtl8169_dev_initialize(dev);
	if (rc != EOK)
		return rc;

	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	/* Map register space */
	rc = pio_enable(rtl8169->regs_phys, RTL8169_IO_SIZE, &rtl8169->regs);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot map register space for device %s.",
		    ddf_dev_get_name(dev));
		goto err_destroy;
	}

	/* Read MAC address and print it */
	rtl8169_get_hwaddr(rtl8169, &nic_addr);
	ddf_msg(LVL_NOTE, "MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
	    nic_addr.address[0], nic_addr.address[1],
	    nic_addr.address[2], nic_addr.address[3],
	    nic_addr.address[4], nic_addr.address[5]);

	rc = nic_report_address(nic_data, &nic_addr);
	if (rc != EOK)
		goto err_pio;

	rc = rtl8169_register_int_handler(nic_data);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to register IRQ handler (%d)", rc);
		goto err_irq;

	}

	ddf_msg(LVL_DEBUG, "Interrupt handler installed");

	uint8_t cr_value = pio_read_8(rtl8169->regs + CR);
	pio_write_8(rtl8169->regs + CR, cr_value | CR_TE | CR_RE);

	rc = nic_connect_to_services(nic_data);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to connect to services (%d)", rc);
		goto err_irq;
	}

	fun = ddf_fun_create(nic_get_ddf_dev(nic_data), fun_exposed, "port0");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating device function");
		goto err_srv;
	}

	nic_set_ddf_fun(nic_data, fun);
	ddf_fun_set_ops(fun, &rtl8169_dev_ops);
	ddf_fun_data_implant(fun, nic_data);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding device function");
		goto err_fun_create;
	}

	rc = ddf_fun_add_to_category(fun, DEVICE_CATEGORY_NIC);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function to category");
		goto err_fun_bind;
	}
	
	ddf_msg(LVL_NOTE, "The %s device has been successfully initialized.",
	    ddf_dev_get_name(dev));
	return EOK;

err_fun_bind:
	ddf_fun_unbind(fun);
err_fun_create:
	ddf_fun_destroy(fun);
err_srv:
	/* XXX Disconnect from services */
err_irq:
	//unregister_interrupt_handler(dev, rtl8169->irq);
err_pio:
err_destroy:
	//rtl8169_dev_cleanup(dev);
	return rc;

	return EOK;
}

static int rtl8169_set_addr(ddf_fun_t *fun, const nic_address_t *addr)
{
	return EOK;
}

static int rtl8169_get_device_info(ddf_fun_t *fun, nic_device_info_t *info)
{
	return EOK;
}

static int rtl8169_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state)
{
	return EOK;
}

static int rtl8169_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	return EOK;
}

static int rtl8169_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	return EOK;
}

static int rtl8169_pause_get(ddf_fun_t *fun, nic_result_t *we_send, 
    nic_result_t *we_receive, uint16_t *time)
{
	return EOK;
}

static int rtl8169_pause_set(ddf_fun_t *fun, int allow_send, int allow_receive, 
    uint16_t time)
{
	return EOK;
}

static int rtl8169_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement)
{
	return EOK;
}

static int rtl8169_autoneg_disable(ddf_fun_t *fun)
{
	return EOK;
}

static int rtl8169_autoneg_probe(ddf_fun_t *fun, uint32_t *advertisement,
    uint32_t *their_adv, nic_result_t *result, nic_result_t *their_result)
{
	return EOK;
}

static int rtl8169_autoneg_restart(ddf_fun_t *fun)
{
	return EOK;
}

static int rtl8169_defective_get_mode(ddf_fun_t *fun, uint32_t *mode)
{
	return EOK;
}

static int rtl8169_defective_set_mode(ddf_fun_t *fun, uint32_t mode)
{
	return EOK;
}

static int rtl8169_on_activated(nic_t *nic_data)
{
	ddf_msg(LVL_NOTE, "Activating device");

	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	/* Reset card */
	pio_write_8(rtl8169->regs + CONFIG0, 0);
	rtl8169_reset(rtl8169);

	/* Allocate buffers */
	rtl8169_allocate_buffers(rtl8169);

	/* Write address of descriptor as start of TX ring */
	pio_write_32(rtl8169->regs + TNPDS, rtl8169->tx_ring_phys & 0xffffffff);
	pio_write_32(rtl8169->regs + TNPDS + 4, (rtl8169->tx_ring_phys >> 32) & 0xffffffff);
	rtl8169->tx_head = 0;
	rtl8169->tx_tail = 0;
	rtl8169->tx_ring[15].control = CONTROL_EOR;

	/* Enable TX and RX */
	uint8_t cr = pio_read_8(rtl8169->regs + CR);
	cr |= CR_TE | CR_RE;
	pio_write_8(rtl8169->regs + CR, cr);

	pio_write_16(rtl8169->regs + IMR, 0xffff);
	nic_enable_interrupt(nic_data, rtl8169->irq);

	return EOK;
}

static int rtl8169_on_stopped(nic_t *nic_data)
{
	ddf_msg(LVL_NOTE, "Stopping device");
	return EOK;
}

inline static void rtl8169_reset(rtl8169_t *rtl8169)
{
	pio_write_8(rtl8169->regs + CR, CR_RST);
	memory_barrier();
	while (pio_read_8(rtl8169->regs + CR) & CR_RST) {
		usleep(1);
		read_barrier();
	}
}

static void rtl8169_link_change(ddf_dev_t *dev)
{
	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	uint8_t phystatus = pio_read_8(rtl8169->regs + PHYSTATUS);

	if (phystatus & PHYSTATUS_LINK) {
		ddf_msg(LVL_NOTE, "%s: Link up", ddf_dev_get_name(dev));

		int speed;
		const char *fdx = phystatus & PHYSTATUS_FDX ? "full duplex" : "half duplex";

		if (phystatus & PHYSTATUS_10M)
			speed = 10;

		if (phystatus & PHYSTATUS_100M)
			speed = 100;

		if (phystatus & PHYSTATUS_1000M)
			speed = 1000;

		ddf_msg(LVL_NOTE, "%s: Speed %dMbit/s, %s", ddf_dev_get_name(dev), speed, fdx);
	} else {
		ddf_msg(LVL_NOTE, "%s: Link down", ddf_dev_get_name(dev));
	}

}

static int rtl8169_unicast_set(nic_t *nic_data, nic_unicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count)
{
	return EOK;
}

static int rtl8169_multicast_set(nic_t *nic_data, nic_multicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count)
{
	return EOK;
}

static int rtl8169_broadcast_set(nic_t *nic_data, nic_broadcast_mode_t mode)
{
	return EOK;
}

static void rtl8169_transmit_done(ddf_dev_t *dev)
{
	unsigned int tail, head;
	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);
	rtl8169_descr_t *descr;

	ddf_msg(LVL_NOTE, "rtl8169_transmit_done()");

	fibril_mutex_lock(&rtl8169->tx_lock);

	head = rtl8169->tx_head;
	tail = rtl8169->tx_tail;

	while (tail != head) {
		descr = &rtl8169->tx_ring[tail];
		tail = (tail + 1) % TX_BUFFERS_COUNT;

		ddf_msg(LVL_NOTE, "TX status for descr %d: 0x%08x", tail, descr->control);
	}

	rtl8169->tx_tail = tail;

	fibril_mutex_unlock(&rtl8169->tx_lock);
}

static void rtl8169_irq_handler(ddf_dev_t *dev, ipc_callid_t iid,
    ipc_call_t *icall)
{
	assert(dev);
	assert(icall);

	uint16_t isr = (uint16_t) IPC_GET_ARG2(*icall);
	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	//ddf_msg(LVL_NOTE, "rtl8169_irq_handler(): isr=0x%04x", isr);

	/* Packet underrun or link change */
	if (isr & INT_PUN)
		rtl8169_link_change(dev);

	/* Frame(s) successfully transmitted */
	if (isr & INT_TOK)
		rtl8169_transmit_done(dev);

	/* Transmit error */
	if (isr & INT_TER)
		rtl8169_transmit_done(dev);

	if (isr & INT_SERR)
		ddf_msg(LVL_ERROR, "System error interrupt");

	if (isr & INT_TDU)
		ddf_msg(LVL_ERROR, "Transmit descriptor unavailable");

	if (isr & INT_RER)
		ddf_msg(LVL_NOTE, "RX error interrupt");

	if (isr & INT_ROK)
		ddf_msg(LVL_NOTE, "RX OK interrupt");

	pio_write_16(rtl8169->regs + ISR, 0xffff);
	pio_write_16(rtl8169->regs + IMR, 0xffff);
}

static void rtl8169_send_frame(nic_t *nic_data, void *data, size_t size)
{
	rtl8169_descr_t *descr, *prev;
	unsigned int head, tail;
	void *buff;
	uintptr_t buff_phys;
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	if (size > RTL8169_FRAME_MAX_LENGTH) {
		ddf_msg(LVL_ERROR, "Send frame: frame too long, %zu bytes",
		    size);
		nic_report_send_error(nic_data, NIC_SEC_OTHER, 1);
	}

	fibril_mutex_lock(&rtl8169->tx_lock);

	ddf_msg(LVL_NOTE, "send_frame()");
	ddf_msg(LVL_NOTE, "tx ring virtual at %p", rtl8169->tx_ring);
	ddf_msg(LVL_NOTE, "tx ring physical at 0x%08lx", rtl8169->tx_ring_phys);
	ddf_msg(LVL_NOTE, "tx_head=%d tx_tail=%d", rtl8169->tx_head, rtl8169->tx_tail);

	head = rtl8169->tx_head;
	tail = rtl8169->tx_tail;

	if ((tail + 1) % TX_BUFFERS_COUNT == head) {
		/* Queue is full */
		nic_set_tx_busy(nic_data, 1);
	}

	/* Calculate address of next free buffer and descriptor */
	buff = rtl8169->tx_buff + (BUFFER_SIZE * head);
	buff_phys = rtl8169->tx_buff_phys + (BUFFER_SIZE * head);

	/* Copy frame */
	memcpy(buff, data, size);

	/* Setup descriptor */
	descr = &rtl8169->tx_ring[head];
	prev = &rtl8169->tx_ring[(head - 1) % TX_BUFFERS_COUNT];

	ddf_msg(LVL_NOTE, "current_descr=%p, prev_descr=%p", descr, prev);

	descr->control = CONTROL_OWN | CONTROL_FS | CONTROL_LS;
	descr->control |= size & 0xffff;
	descr->vlan = 0;
	descr->buf_low = buff_phys & 0xffffffff;
	descr->buf_high = (buff_phys >> 32) & 0xffffffff;
	rtl8169->tx_head = (head + 1) % TX_BUFFERS_COUNT;

	/* Lift EOR flag from previous descriptor */
	prev->control &= ~CONTROL_EOR;

	write_barrier();

	/* Notify NIC of pending packets */
	pio_write_8(rtl8169->regs + TPPOLL, TPPOLL_NPQ);

	fibril_mutex_unlock(&rtl8169->tx_lock);
}

static inline void rtl8169_get_hwaddr(rtl8169_t *rtl8169, nic_address_t *addr)
{
	int i;

	assert(rtl8169);
	assert(addr);

	for (i = 0; i < 6; i++)
		addr->address[i] = pio_read_8(rtl8169->regs + MAC0 + i);
}

static inline void rtl8169_set_hwaddr(rtl8169_t *rtl8169, nic_address_t *addr)
{
	int i;

	assert(rtl8169);
	assert(addr);

	for (i = 0; i < 6; i++)
		addr->address[i] = pio_read_8(rtl8169->regs + MAC0 + i);
}

/** Main function of RTL8169 driver
*
 *  Just initialize the driver structures and
 *  put it into the device drivers interface
 */
int main(void)
{
	int rc = nic_driver_init(NAME);
	if (rc != EOK)
		return rc;
	nic_driver_implement(
		&rtl8169_driver_ops, &rtl8169_dev_ops, &rtl8169_nic_iface);

	ddf_log_init(NAME);
	ddf_msg(LVL_NOTE, "HelenOS RTL8169 driver started");
	return ddf_driver_main(&rtl8169_driver);
}
