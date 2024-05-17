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

#include <assert.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <align.h>
#include <byteorder.h>
#include <barrier.h>
#include <stdbool.h>

#include <as.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>
#include <device/hw_res.h>
#include <device/hw_res_parsed.h>
#include <io/log.h>
#include <nic.h>
#include <pci_dev_iface.h>

#include <str.h>

#include "defs.h"
#include "driver.h"

/** Global mutex for work with shared irq structure */
FIBRIL_MUTEX_INITIALIZE(irq_reg_lock);

static errno_t rtl8169_set_addr(ddf_fun_t *fun, const nic_address_t *addr);
static errno_t rtl8169_get_device_info(ddf_fun_t *fun, nic_device_info_t *info);
static errno_t rtl8169_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state);
static errno_t rtl8169_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role);
static errno_t rtl8169_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t role);
static errno_t rtl8169_pause_get(ddf_fun_t *fun, nic_result_t *we_send,
    nic_result_t *we_receive, uint16_t *time);
static errno_t rtl8169_pause_set(ddf_fun_t *fun, int allow_send, int allow_receive,
    uint16_t time);
static errno_t rtl8169_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement);
static errno_t rtl8169_autoneg_disable(ddf_fun_t *fun);
static errno_t rtl8169_autoneg_probe(ddf_fun_t *fun, uint32_t *advertisement,
    uint32_t *their_adv, nic_result_t *result, nic_result_t *their_result);
static errno_t rtl8169_autoneg_restart(ddf_fun_t *fun);
static errno_t rtl8169_defective_get_mode(ddf_fun_t *fun, uint32_t *mode);
static errno_t rtl8169_defective_set_mode(ddf_fun_t *fun, uint32_t mode);
static errno_t rtl8169_on_activated(nic_t *nic_data);
static errno_t rtl8169_on_stopped(nic_t *nic_data);
static void rtl8169_send_frame(nic_t *nic_data, void *data, size_t size);
static void rtl8169_irq_handler(ipc_call_t *icall, void *);
static inline errno_t rtl8169_register_int_handler(nic_t *nic_data,
    cap_irq_handle_t *handle);
static inline void rtl8169_get_hwaddr(rtl8169_t *rtl8169, nic_address_t *addr);
static inline void rtl8169_set_hwaddr(rtl8169_t *rtl8169, const nic_address_t *addr);

static void rtl8169_reset(rtl8169_t *rtl8169);
static errno_t rtl8169_get_resource_info(ddf_dev_t *dev);
static errno_t rtl8169_fill_resource_info(ddf_dev_t *dev, const hw_res_list_parsed_t *hw_resources);
static rtl8169_t *rtl8169_create_dev_data(ddf_dev_t *dev);

static errno_t rtl8169_unicast_set(nic_t *nic_data, nic_unicast_mode_t mode,
    const nic_address_t *, size_t);
static errno_t rtl8169_multicast_set(nic_t *nic_data, nic_multicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count);
static errno_t rtl8169_broadcast_set(nic_t *nic_data, nic_broadcast_mode_t mode);

static uint16_t rtl8169_mii_read(rtl8169_t *rtl8169, uint8_t addr);
static void rtl8169_mii_write(rtl8169_t *rtl8169, uint8_t addr, uint16_t value);
static void rtl8169_rx_ring_refill(rtl8169_t *rtl8169, unsigned int first,
    unsigned int last);

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

static errno_t rtl8169_dev_add(ddf_dev_t *dev);

/** Basic driver operations for RTL8169 driver */
static driver_ops_t rtl8169_driver_ops = {
	.dev_add = &rtl8169_dev_add,
};

/** Driver structure for RTL8169 driver */
static driver_t rtl8169_driver = {
	.name = NAME,
	.driver_ops = &rtl8169_driver_ops
};

static errno_t rtl8169_get_resource_info(ddf_dev_t *dev)
{
	assert(dev);

	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	assert(nic_data);

	hw_res_list_parsed_t hw_res_parsed;
	hw_res_list_parsed_init(&hw_res_parsed);

	/* Get hw resources form parent driver */
	errno_t rc = nic_get_resources(nic_data, &hw_res_parsed);
	if (rc != EOK)
		return rc;

	/* Fill resources information to the device */
	errno_t ret = rtl8169_fill_resource_info(dev, &hw_res_parsed);
	hw_res_list_parsed_clean(&hw_res_parsed);

	return ret;
}

static errno_t rtl8169_fill_resource_info(ddf_dev_t *dev, const hw_res_list_parsed_t
    *hw_resources)
{
	assert(dev);
	assert(hw_resources);

	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_dev(dev));
	assert(rtl8169);

	if (hw_resources->irqs.count != 1) {
		ddf_msg(LVL_ERROR, "%s device: unexpected irq count", ddf_dev_get_name(dev));
		return EINVAL;
	}

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

static errno_t rtl8169_allocate_buffers(rtl8169_t *rtl8169)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Allocating DMA buffer rings");

	/* Allocate TX ring */
	rtl8169->tx_ring = AS_AREA_ANY;
	rc = dmamem_map_anonymous(TX_RING_SIZE, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->tx_ring_phys,
	    (void **)&rtl8169->tx_ring);

	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG, "TX ring address: phys=0x%#" PRIxn ", virt=%p",
	    rtl8169->tx_ring_phys, rtl8169->tx_ring);

	memset(rtl8169->tx_ring, 0, TX_RING_SIZE);

	/* Allocate RX ring */
	rtl8169->rx_ring = AS_AREA_ANY;
	rc = dmamem_map_anonymous(RX_RING_SIZE, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->rx_ring_phys,
	    (void **)&rtl8169->rx_ring);

	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG, "RX ring address: phys=0x%#" PRIxn ", virt=%p",
	    rtl8169->rx_ring_phys, rtl8169->rx_ring);

	memset(rtl8169->rx_ring, 0, RX_RING_SIZE);

	/* Allocate TX buffers */
	rtl8169->tx_buff = AS_AREA_ANY;
	rc = dmamem_map_anonymous(TX_BUFFERS_SIZE, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->tx_buff_phys,
	    &rtl8169->tx_buff);

	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG, "TX buffers base address: phys=0x%#" PRIxn " virt=%p",
	    rtl8169->tx_buff_phys, rtl8169->tx_buff);

	/* Allocate RX buffers */
	rtl8169->rx_buff = AS_AREA_ANY;
	rc = dmamem_map_anonymous(RX_BUFFERS_SIZE, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &rtl8169->rx_buff_phys,
	    &rtl8169->rx_buff);

	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG, "RX buffers base address: phys=0x%#" PRIxn ", virt=%p",
	    rtl8169->rx_buff_phys, rtl8169->rx_buff);

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

static void rtl8169_dev_cleanup(ddf_dev_t *dev)
{
	assert(dev);

	if (ddf_dev_data_get(dev))
		nic_unbind_and_destroy(dev);
}

static errno_t rtl8169_dev_initialize(ddf_dev_t *dev)
{
	errno_t ret;

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
	rtl8169_dev_cleanup(dev);
	return ret;

}

inline static errno_t rtl8169_register_int_handler(nic_t *nic_data,
    cap_irq_handle_t *handle)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	rtl8169_irq_code.ranges[0].base = (uintptr_t) rtl8169->regs;
	rtl8169_irq_code.cmds[0].addr = rtl8169->regs + ISR;
	rtl8169_irq_code.cmds[2].addr = rtl8169->regs + ISR;
	rtl8169_irq_code.cmds[3].addr = rtl8169->regs + IMR;
	errno_t rc = register_interrupt_handler(nic_get_ddf_dev(nic_data),
	    rtl8169->irq, rtl8169_irq_handler, (void *)rtl8169,
	    &rtl8169_irq_code, handle);

	return rc;
}

static errno_t rtl8169_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun;
	nic_address_t nic_addr;
	errno_t rc;

	assert(dev);
	ddf_msg(LVL_NOTE, "RTL8169_dev_add %s (handle = %zu)",
	    ddf_dev_get_name(dev), ddf_dev_get_handle(dev));

	/* Init structures */
	rc = rtl8169_dev_initialize(dev);
	if (rc != EOK)
		return rc;

	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	rtl8169->dev = dev;
	rtl8169->parent_sess = ddf_dev_parent_sess_get(dev);
	if (rtl8169->parent_sess == NULL)
		return EIO;

	/* Get PCI VID & PID */
	rc = pci_config_space_read_16(rtl8169->parent_sess, PCI_VENDOR_ID,
	    &rtl8169->pci_vid);
	if (rc != EOK)
		return rc;

	rc = pci_config_space_read_16(rtl8169->parent_sess, PCI_DEVICE_ID,
	    &rtl8169->pci_pid);
	if (rc != EOK)
		return rc;

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

	cap_irq_handle_t irq_handle;
	rc = rtl8169_register_int_handler(nic_data, &irq_handle);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to register IRQ handler (%s)", str_error_name(rc));
		goto err_irq;
	}

	ddf_msg(LVL_DEBUG, "Interrupt handler installed");

	uint8_t cr_value = pio_read_8(rtl8169->regs + CR);
	pio_write_8(rtl8169->regs + CR, cr_value | CR_TE | CR_RE);

	fun = ddf_fun_create(nic_get_ddf_dev(nic_data), fun_exposed, "port0");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating device function");
		goto err_srv;
	}

	nic_set_ddf_fun(nic_data, fun);
	ddf_fun_set_ops(fun, &rtl8169_dev_ops);

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
	unregister_interrupt_handler(dev, irq_handle);
err_irq:
err_pio:
err_destroy:
	rtl8169_dev_cleanup(dev);
	return rc;

	return EOK;
}

static errno_t rtl8169_set_addr(ddf_fun_t *fun, const nic_address_t *addr)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);
	errno_t rc;

	fibril_mutex_lock(&rtl8169->rx_lock);
	fibril_mutex_lock(&rtl8169->tx_lock);

	rc = nic_report_address(nic_data, addr);
	if (rc != EOK)
		return rc;

	rtl8169_set_hwaddr(rtl8169, addr);

	fibril_mutex_unlock(&rtl8169->rx_lock);
	fibril_mutex_unlock(&rtl8169->tx_lock);

	return EOK;
}

static errno_t rtl8169_get_device_info(ddf_fun_t *fun, nic_device_info_t *info)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	str_cpy(info->vendor_name, NIC_VENDOR_MAX_LENGTH, "Unknown");
	str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH, "Unknown");

	if (rtl8169->pci_vid == PCI_VID_REALTEK)
		str_cpy(info->vendor_name, NIC_VENDOR_MAX_LENGTH, "Realtek");

	if (rtl8169->pci_vid == PCI_VID_DLINK)
		str_cpy(info->vendor_name, NIC_VENDOR_MAX_LENGTH, "D-Link");

	if (rtl8169->pci_pid == 0x8168)
		str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH, "RTL8168");

	if (rtl8169->pci_pid == 0x8169)
		str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH, "RTL8169");

	if (rtl8169->pci_pid == 0x8110)
		str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH, "RTL8110");

	return EOK;
}

static errno_t rtl8169_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_fun(fun));
	uint8_t phystatus = pio_read_8(rtl8169->regs + PHYSTATUS);

	if (phystatus & PHYSTATUS_LINK)
		*state = NIC_CS_PLUGGED;
	else
		*state = NIC_CS_UNPLUGGED;

	return EOK;
}

static errno_t rtl8169_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_fun(fun));
	uint8_t phystatus = pio_read_8(rtl8169->regs + PHYSTATUS);

	*duplex = phystatus & PHYSTATUS_FDX ?
	    NIC_CM_FULL_DUPLEX : NIC_CM_HALF_DUPLEX;

	if (phystatus & PHYSTATUS_10M)
		*speed = 10;

	if (phystatus & PHYSTATUS_100M)
		*speed = 100;

	if (phystatus & PHYSTATUS_1000M)
		*speed = 1000;

	*role = NIC_ROLE_UNKNOWN;
	return EOK;
}

static errno_t rtl8169_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_fun(fun));
	uint16_t bmcr;

	if (speed != 10 && speed != 100 && speed != 1000)
		return EINVAL;

	if (duplex != NIC_CM_HALF_DUPLEX && duplex != NIC_CM_FULL_DUPLEX)
		return EINVAL;

	bmcr = rtl8169_mii_read(rtl8169, MII_BMCR);
	bmcr &= ~(BMCR_DUPLEX | BMCR_SPD_100 | BMCR_SPD_1000);

	/* Disable autonegotiation */
	bmcr &= ~BMCR_AN_ENABLE;

	if (duplex == NIC_CM_FULL_DUPLEX)
		bmcr |= BMCR_DUPLEX;

	if (speed == 100)
		bmcr |= BMCR_SPD_100;

	if (speed == 1000)
		bmcr |= BMCR_SPD_1000;

	rtl8169_mii_write(rtl8169, MII_BMCR, bmcr);
	return EOK;
}

static errno_t rtl8169_pause_get(ddf_fun_t *fun, nic_result_t *we_send,
    nic_result_t *we_receive, uint16_t *time)
{
	return EOK;
}

static errno_t rtl8169_pause_set(ddf_fun_t *fun, int allow_send, int allow_receive,
    uint16_t time)
{
	return EOK;
}

static errno_t rtl8169_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_fun(fun));
	uint16_t bmcr = rtl8169_mii_read(rtl8169, MII_BMCR);
	uint16_t anar = ANAR_SELECTOR;

	if (advertisement & ETH_AUTONEG_10BASE_T_FULL)
		anar |= ANAR_10_FD;
	if (advertisement & ETH_AUTONEG_10BASE_T_HALF)
		anar |= ANAR_10_HD;
	if (advertisement & ETH_AUTONEG_100BASE_TX_FULL)
		anar |= ANAR_100TX_FD;
	if (advertisement & ETH_AUTONEG_100BASE_TX_HALF)
		anar |= ANAR_100TX_HD;
	if (advertisement & ETH_AUTONEG_PAUSE_SYMETRIC)
		anar |= ANAR_PAUSE;

	bmcr |= BMCR_AN_ENABLE;
	rtl8169_mii_write(rtl8169, MII_BMCR, bmcr);
	rtl8169_mii_write(rtl8169, MII_ANAR, anar);

	return EOK;
}

static errno_t rtl8169_autoneg_disable(ddf_fun_t *fun)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_fun(fun));
	uint16_t bmcr = rtl8169_mii_read(rtl8169, MII_BMCR);

	bmcr &= ~BMCR_AN_ENABLE;
	rtl8169_mii_write(rtl8169, MII_BMCR, bmcr);

	return EOK;
}

static errno_t rtl8169_autoneg_probe(ddf_fun_t *fun, uint32_t *advertisement,
    uint32_t *their_adv, nic_result_t *result, nic_result_t *their_result)
{
	return EOK;
}

static errno_t rtl8169_autoneg_restart(ddf_fun_t *fun)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_get_from_ddf_fun(fun));
	uint16_t bmcr = rtl8169_mii_read(rtl8169, MII_BMCR);

	bmcr |= BMCR_AN_ENABLE;
	rtl8169_mii_write(rtl8169, MII_BMCR, bmcr);
	return EOK;
}

static errno_t rtl8169_defective_get_mode(ddf_fun_t *fun, uint32_t *mode)
{
	return EOK;
}

static errno_t rtl8169_defective_set_mode(ddf_fun_t *fun, uint32_t mode)
{
	return EOK;
}

static void rtl8169_rx_ring_refill(rtl8169_t *rtl8169, unsigned int first,
    unsigned int last)
{
	rtl8169_descr_t *descr;
	uint64_t buff_phys;
	unsigned int i = first;

	while (true) {
		descr = &rtl8169->rx_ring[i];
		buff_phys = rtl8169->rx_buff_phys + (BUFFER_SIZE * i);
		descr->control = BUFFER_SIZE | CONTROL_OWN;
		descr->buf_low = buff_phys & 0xffffffff;
		descr->buf_high = (buff_phys >> 32) & 0xffffffff;

		if (i == RX_BUFFERS_COUNT - 1)
			descr->control |= CONTROL_EOR;

		if (i == last)
			break;

		i = (i + 1) % RX_BUFFERS_COUNT;
	}
}

static errno_t rtl8169_on_activated(nic_t *nic_data)
{
	errno_t rc;
	uint64_t tmp;

	ddf_msg(LVL_NOTE, "Activating device");

	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	/* Reset card */
	pio_write_8(rtl8169->regs + CONFIG0, 0);
	rtl8169_reset(rtl8169);

	/* Allocate buffers */
	rc = rtl8169_allocate_buffers(rtl8169);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error allocating buffers: %s", str_error_name(rc));
		return 0;
	}

	/* Initialize RX ring */
	rtl8169_rx_ring_refill(rtl8169, 0, RX_BUFFERS_COUNT - 1);

	/* Write address of descriptor as start of TX ring */
	tmp = rtl8169->tx_ring_phys;
	pio_write_32(rtl8169->regs + TNPDS, tmp & 0xffffffff);
	pio_write_32(rtl8169->regs + TNPDS + 4, (tmp >> 32) & 0xffffffff);
	rtl8169->tx_head = 0;
	rtl8169->tx_tail = 0;
	rtl8169->tx_ring[15].control = CONTROL_EOR;

	/* Write RX ring address */
	tmp = rtl8169->rx_ring_phys;
	pio_write_32(rtl8169->regs + RDSAR, tmp & 0xffffffff);
	pio_write_32(rtl8169->regs + RDSAR + 4, (tmp >> 32) & 0xffffffff);
	rtl8169->rx_head = 0;
	rtl8169->rx_tail = 0;

	/* Clear pending interrupts */
	pio_write_16(rtl8169->regs + ISR, 0xffff);

	/* Enable TX and RX */
	uint8_t cr = pio_read_8(rtl8169->regs + CR);
	cr |= CR_TE | CR_RE;
	pio_write_8(rtl8169->regs + CR, cr);
	pio_write_32(rtl8169->regs + MAR0, 0xffffffff);
	pio_write_32(rtl8169->regs + MAR0 + 4, 0xffffffff);

	/* Configure Receive Control Register */
	uint32_t rcr = pio_read_32(rtl8169->regs + RCR);
	rtl8169->rcr_ucast = RCR_ACCEPT_PHYS_MATCH;
	rcr |= RCR_ACCEPT_PHYS_MATCH | RCR_ACCEPT_ERROR | RCR_ACCEPT_RUNT;
	pio_write_32(rtl8169->regs + RCR, rcr);
	pio_write_16(rtl8169->regs + RMS, BUFFER_SIZE);

	pio_write_16(rtl8169->regs + IMR, 0xffff);
	/* XXX Check return value */
	hw_res_enable_interrupt(rtl8169->parent_sess, rtl8169->irq);

	return EOK;
}

static errno_t rtl8169_on_stopped(nic_t *nic_data)
{
	ddf_msg(LVL_NOTE, "Stopping device");
	return EOK;
}

inline static void rtl8169_reset(rtl8169_t *rtl8169)
{
	pio_write_8(rtl8169->regs + CR, CR_RST);
	memory_barrier();
	while (pio_read_8(rtl8169->regs + CR) & CR_RST) {
		fibril_usleep(1);
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

/** Notify NIC framework about HW filtering state when promisc mode was disabled
 *
 *  @param nic_data     The NIC data
 *  @param mcast_mode   Current multicast mode
 *  @param was_promisc  Sign if the promiscuous mode was active before disabling
 */
inline static void rtl8169_rcx_promics_rem(nic_t *nic_data,
    nic_multicast_mode_t mcast_mode, uint8_t was_promisc)
{
	assert(nic_data);

	if (was_promisc != 0) {
		if (mcast_mode == NIC_MULTICAST_LIST)
			nic_report_hw_filtering(nic_data, 1, 0, -1);
		else
			nic_report_hw_filtering(nic_data, 1, 1, -1);
	} else {
		nic_report_hw_filtering(nic_data, 1, -1, -1);
	}
}

static errno_t rtl8169_unicast_set(nic_t *nic_data, nic_unicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);
	uint32_t rcr = pio_read_32(rtl8169->regs + RCR);
	uint8_t was_promisc = rcr & RCR_ACCEPT_ALL_PHYS;
	nic_multicast_mode_t mcast_mode;

	nic_query_multicast(nic_data, &mcast_mode, 0, NULL, NULL);

	ddf_msg(LVL_DEBUG, "Unicast RX filter mode: %d", mode);

	switch (mode) {
	case NIC_UNICAST_BLOCKED:
		rtl8169->rcr_ucast = 0;
		rtl8169_rcx_promics_rem(nic_data, mcast_mode, was_promisc);
		break;
	case NIC_UNICAST_DEFAULT:
		rtl8169->rcr_ucast = RCR_ACCEPT_PHYS_MATCH;
		rtl8169_rcx_promics_rem(nic_data, mcast_mode, was_promisc);
		break;
	case NIC_UNICAST_LIST:
		rtl8169->rcr_ucast = RCR_ACCEPT_PHYS_MATCH | RCR_ACCEPT_ALL_PHYS;

		if (mcast_mode == NIC_MULTICAST_PROMISC)
			nic_report_hw_filtering(nic_data, 0, 1, -1);
		else
			nic_report_hw_filtering(nic_data, 0, 0, -1);
		break;
	case NIC_UNICAST_PROMISC:
		rtl8169->rcr_ucast = RCR_ACCEPT_PHYS_MATCH | RCR_ACCEPT_ALL_PHYS;

		if (mcast_mode == NIC_MULTICAST_PROMISC)
			nic_report_hw_filtering(nic_data, 1, 1, -1);
		else
			nic_report_hw_filtering(nic_data, 1, 0, -1);
		break;
	default:
		return ENOTSUP;
	}

	fibril_mutex_lock(&rtl8169->rx_lock);

	rcr &= ~(RCR_ACCEPT_PHYS_MATCH | RCR_ACCEPT_ALL_PHYS);
	pio_write_32(rtl8169->regs + RCR, rcr | rtl8169->rcr_ucast | rtl8169->rcr_mcast);
	ddf_msg(LVL_DEBUG, "new RCR value: 0x%08x", rcr | rtl8169->rcr_ucast | rtl8169->rcr_mcast);

	fibril_mutex_unlock(&rtl8169->rx_lock);
	return EOK;
}

static errno_t rtl8169_multicast_set(nic_t *nic_data, nic_multicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);
	uint32_t rcr = pio_read_32(rtl8169->regs + RCR);
	uint64_t mask;

	ddf_msg(LVL_DEBUG, "Multicast RX filter mode: %d", mode);

	switch (mode) {
	case NIC_MULTICAST_BLOCKED:
		rtl8169->rcr_mcast = 0;
		if ((rtl8169->rcr_ucast & RCR_ACCEPT_ALL_PHYS) != 0)
			nic_report_hw_filtering(nic_data, -1, 0, -1);
		else
			nic_report_hw_filtering(nic_data, -1, 1, -1);
		break;
	case NIC_MULTICAST_LIST:
		mask = nic_mcast_hash(addr, addr_count);
		pio_write_32(rtl8169->regs + MAR0, (uint32_t)mask);
		pio_write_32(rtl8169->regs + MAR0 + 4, (uint32_t)(mask >> 32));
		rtl8169->rcr_mcast = RCR_ACCEPT_MULTICAST;
		nic_report_hw_filtering(nic_data, -1, 0, -1);
		break;
	case NIC_MULTICAST_PROMISC:
		pio_write_32(rtl8169->regs + MAR0, 0xffffffffULL);
		pio_write_32(rtl8169->regs + MAR0 + 4, (uint32_t)(0xffffffffULL >> 32));
		rtl8169->rcr_mcast = RCR_ACCEPT_MULTICAST;
		nic_report_hw_filtering(nic_data, -1, 1, -1);
		break;
	default:
		return ENOTSUP;
	}

	fibril_mutex_lock(&rtl8169->rx_lock);

	rcr &= ~(RCR_ACCEPT_PHYS_MATCH | RCR_ACCEPT_ALL_PHYS);
	pio_write_32(rtl8169->regs + RCR, rcr | rtl8169->rcr_ucast | rtl8169->rcr_mcast);
	ddf_msg(LVL_DEBUG, "new RCR value: 0x%08x", rcr | rtl8169->rcr_ucast | rtl8169->rcr_mcast);

	fibril_mutex_unlock(&rtl8169->rx_lock);
	return EOK;
}

static errno_t rtl8169_broadcast_set(nic_t *nic_data, nic_broadcast_mode_t mode)
{
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	/* Configure Receive Control Register */
	uint32_t rcr = pio_read_32(rtl8169->regs + RCR);

	ddf_msg(LVL_DEBUG, "Broadcast RX filter mode: %d", mode);

	switch (mode) {
	case NIC_BROADCAST_BLOCKED:
		rcr &= ~RCR_ACCEPT_BROADCAST;
		break;
	case NIC_BROADCAST_ACCEPTED:
		rcr |= RCR_ACCEPT_BROADCAST;
		break;
	default:
		return ENOTSUP;
	}

	pio_write_32(rtl8169->regs + RCR, rcr);
	ddf_msg(LVL_DEBUG, " new RCR value: 0x%08x", rcr);

	return EOK;
}

static void rtl8169_transmit_done(ddf_dev_t *dev)
{
	unsigned int tail, head;
	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);
	rtl8169_descr_t *descr;
	int sent = 0;

	ddf_msg(LVL_DEBUG, "rtl8169_transmit_done()");

	fibril_mutex_lock(&rtl8169->tx_lock);

	head = rtl8169->tx_head;
	tail = rtl8169->tx_tail;

	while (tail != head) {
		descr = &rtl8169->tx_ring[tail];
		descr->control &= (~CONTROL_OWN);
		write_barrier();
		ddf_msg(LVL_DEBUG, "TX status for descr %d: 0x%08x", tail, descr->control);

		tail = (tail + 1) % TX_BUFFERS_COUNT;
		sent++;
	}

	if (sent != 0)
		nic_set_tx_busy(nic_data, 0);

	rtl8169->tx_tail = tail;

	fibril_mutex_unlock(&rtl8169->tx_lock);
}

static void rtl8169_receive_done(ddf_dev_t *dev)
{
	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);
	rtl8169_descr_t *descr;
	nic_frame_list_t *frames = nic_alloc_frame_list();
	nic_frame_t *frame;
	void *buffer;
	unsigned int tail, fsidx = 0;
	int frame_size;

	ddf_msg(LVL_DEBUG, "rtl8169_receive_done()");

	fibril_mutex_lock(&rtl8169->rx_lock);

	tail = rtl8169->rx_tail;

	while (true) {
		descr = &rtl8169->rx_ring[tail];

		if (descr->control & CONTROL_OWN)
			break;

		if (descr->control & RXSTATUS_RES) {
			ddf_msg(LVL_WARN, "error at slot %d: 0x%08x\n", tail, descr->control);
			tail = (tail + 1) % RX_BUFFERS_COUNT;
			continue;
		}

		if (descr->control & CONTROL_FS)
			fsidx = tail;

		if (descr->control & CONTROL_LS) {
			ddf_msg(LVL_DEBUG, "received message at slot %d, control 0x%08x", tail, descr->control);

			if (fsidx != tail)
				ddf_msg(LVL_WARN, "single frame spanning multiple descriptors");

			frame_size = descr->control & 0x1fff;
			buffer = rtl8169->rx_buff + (BUFFER_SIZE * tail);
			frame = nic_alloc_frame(nic_data, frame_size);
			memcpy(frame->data, buffer, frame_size);
			nic_frame_list_append(frames, frame);
		}

		tail = (tail + 1) % RX_BUFFERS_COUNT;
	}

	rtl8169_rx_ring_refill(rtl8169, rtl8169->rx_tail, tail);

	rtl8169->rx_tail = tail;

	fibril_mutex_unlock(&rtl8169->rx_lock);

	nic_received_frame_list(nic_data, frames);

}

/** RTL8169 IRQ handler.
 *
 * @param icall IRQ event notification
 * @param arg Argument (rtl8169_t *)
 */
static void rtl8169_irq_handler(ipc_call_t *icall, void *arg)
{
	uint16_t isr = (uint16_t) ipc_get_arg2(icall) & INT_KNOWN;
	rtl8169_t *rtl8169 = (rtl8169_t *)arg;

	ddf_msg(LVL_DEBUG, "rtl8169_irq_handler(): isr=0x%04x", isr);
	pio_write_16(rtl8169->regs + IMR, 0xffff);

	while (isr != 0) {
		ddf_msg(LVL_DEBUG, "irq handler: remaining isr=0x%04x", isr);

		/* Packet underrun or link change */
		if (isr & INT_PUN) {
			rtl8169_link_change(rtl8169->dev);
			pio_write_16(rtl8169->regs + ISR, INT_PUN);
		}

		/* Transmit notification */
		if (isr & (INT_TER | INT_TOK | INT_TDU)) {
			rtl8169_transmit_done(rtl8169->dev);
			pio_write_16(rtl8169->regs + ISR, (INT_TER | INT_TOK | INT_TDU));
		}

		/* Receive underrun */
		if (isr & INT_RXOVW) {
			/* just ack.. */
			pio_write_16(rtl8169->regs + ISR, INT_RXOVW);
		}

		if (isr & INT_SERR) {
			ddf_msg(LVL_ERROR, "System error interrupt");
			pio_write_16(rtl8169->regs + ISR, INT_SERR);
		}

		if (isr & (INT_RER | INT_ROK)) {
			rtl8169_receive_done(rtl8169->dev);
			pio_write_16(rtl8169->regs + ISR, (INT_RER | INT_ROK));
		}

		isr = pio_read_16(rtl8169->regs + ISR) & INT_KNOWN;
	}

	pio_write_16(rtl8169->regs + ISR, 0xffff);
}

static void rtl8169_send_frame(nic_t *nic_data, void *data, size_t size)
{
	rtl8169_descr_t *descr, *prev;
	unsigned int head, tail;
	void *buff;
	uint64_t buff_phys;
	rtl8169_t *rtl8169 = nic_get_specific(nic_data);

	if (size > RTL8169_FRAME_MAX_LENGTH) {
		ddf_msg(LVL_ERROR, "Send frame: frame too long, %zu bytes",
		    size);
		nic_report_send_error(nic_data, NIC_SEC_OTHER, 1);
	}

	fibril_mutex_lock(&rtl8169->tx_lock);

	ddf_msg(LVL_DEBUG, "send_frame: size: %zu, tx_head=%d tx_tail=%d",
	    size, rtl8169->tx_head, rtl8169->tx_tail);

	head = rtl8169->tx_head;
	tail = rtl8169->tx_tail;

	if ((head + 1) % TX_BUFFERS_COUNT == tail) {
		/* Queue is full */
		ddf_msg(LVL_WARN, "TX queue full!");
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

	ddf_msg(LVL_DEBUG, "current_descr=%p, prev_descr=%p", descr, prev);

	descr->control = CONTROL_OWN | CONTROL_FS | CONTROL_LS;
	descr->control |= size & 0xffff;
	descr->vlan = 0;
	descr->buf_low = buff_phys & 0xffffffff;
	descr->buf_high = (buff_phys >> 32) & 0xffffffff;

	if (head == TX_BUFFERS_COUNT - 1)
		descr->control |= CONTROL_EOR;

	rtl8169->tx_head = (head + 1) % TX_BUFFERS_COUNT;

	ddf_msg(LVL_DEBUG, "control: 0x%08x", descr->control);

	write_barrier();

	/* Notify NIC of pending packets */
	pio_write_8(rtl8169->regs + TPPOLL, TPPOLL_NPQ);
	write_barrier();

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

static inline void rtl8169_set_hwaddr(rtl8169_t *rtl8169, const nic_address_t *addr)
{
	int i;

	assert(rtl8169);
	assert(addr);

	for (i = 0; i < 6; i++)
		pio_write_8(rtl8169->regs + MAC0 + i, addr->address[i]);
}

static uint16_t rtl8169_mii_read(rtl8169_t *rtl8169, uint8_t addr)
{
	uint32_t phyar;

	phyar = PHYAR_RW_READ |
	    ((addr & PHYAR_ADDR_MASK) << PHYAR_ADDR_SHIFT);

	pio_write_32(rtl8169->regs + PHYAR, phyar);

	do {
		phyar = pio_read_32(rtl8169->regs + PHYAR);
		fibril_usleep(20);
	} while ((phyar & PHYAR_RW_WRITE) == 0);

	return phyar & PHYAR_DATA_MASK;
}

static void rtl8169_mii_write(rtl8169_t *rtl8169, uint8_t addr, uint16_t value)
{
	uint32_t phyar;

	phyar = PHYAR_RW_WRITE |
	    ((addr & PHYAR_ADDR_MASK) << PHYAR_ADDR_SHIFT) |
	    (value & PHYAR_DATA_MASK);

	pio_write_32(rtl8169->regs + PHYAR, phyar);

	do {
		phyar = pio_read_32(rtl8169->regs + PHYAR);
		fibril_usleep(20);
	} while ((phyar & PHYAR_RW_WRITE) != 0);

	fibril_usleep(20);
}

/** Main function of RTL8169 driver
 *
 *  Just initialize the driver structures and
 *  put it into the device drivers interface
 */
int main(void)
{
	errno_t rc = nic_driver_init(NAME);
	if (rc != EOK)
		return rc;
	nic_driver_implement(
	    &rtl8169_driver_ops, &rtl8169_dev_ops, &rtl8169_nic_iface);

	ddf_log_init(NAME);
	ddf_msg(LVL_NOTE, "HelenOS RTL8169 driver started");
	return ddf_driver_main(&rtl8169_driver);
}
