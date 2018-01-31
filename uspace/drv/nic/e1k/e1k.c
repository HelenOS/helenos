/*
 * Copyright (c) 2011 Zdenek Bouska
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

/** @file e1k.c
 *
 * Driver for Intel Pro/1000 8254x Family of Gigabit Ethernet Controllers
 *
 */

#include <async.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <adt/list.h>
#include <align.h>
#include <byteorder.h>
#include <as.h>
#include <ddi.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>
#include <device/hw_res.h>
#include <device/hw_res_parsed.h>
#include <pci_dev_iface.h>
#include <nic.h>
#include <ops/nic.h>
#include "e1k.h"

#define NAME  "e1k"

#define E1000_DEFAULT_INTERRUPT_INTERVAL_USEC  250

/* Must be power of 8 */
#define E1000_RX_FRAME_COUNT  128
#define E1000_TX_FRAME_COUNT  128

#define E1000_RECEIVE_ADDRESS  16

/** Maximum sending frame size */
#define E1000_MAX_SEND_FRAME_SIZE  2048
/** Maximum receiving frame size */
#define E1000_MAX_RECEIVE_FRAME_SIZE  2048

/** nic_driver_data_t* -> e1000_t* cast */
#define DRIVER_DATA_NIC(nic) \
	((e1000_t *) nic_get_specific(nic))

/** ddf_fun_t * -> nic_driver_data_t* cast */
#define NIC_DATA_FUN(fun) \
	((nic_t *) ddf_dev_data_get(ddf_fun_get_dev(fun)))

/** ddf_dev_t * -> nic_driver_data_t* cast */
#define NIC_DATA_DEV(dev) \
	((nic_t *) ddf_dev_data_get(dev))

/** ddf_dev_t * -> e1000_t* cast */
#define DRIVER_DATA_DEV(dev) \
	(DRIVER_DATA_NIC(NIC_DATA_DEV(dev)))

/** ddf_fun_t * -> e1000_t* cast */
#define DRIVER_DATA_FUN(fun) \
	(DRIVER_DATA_NIC(NIC_DATA_FUN(fun)))

/** Cast pointer to uint64_t
 *
 * @param ptr Pointer to cast
 *
 * @return The uint64_t pointer representation.
 *
 */
#define PTR_TO_U64(ptr)  ((uint64_t) ((uintptr_t) (ptr)))

/** Cast the memaddr part to the void*
 *
 * @param memaddr The memaddr value
 *
 */
#define MEMADDR_TO_PTR(memaddr)  ((void *) ((size_t) (memaddr)))

#define E1000_REG_BASE(e1000) \
	((e1000)->reg_base_virt)

#define E1000_REG_ADDR(e1000, reg) \
	((uint32_t *) (E1000_REG_BASE(e1000) + reg))

#define E1000_REG_READ(e1000, reg) \
	(pio_read_32(E1000_REG_ADDR(e1000, reg)))

#define E1000_REG_WRITE(e1000, reg, value) \
	(pio_write_32(E1000_REG_ADDR(e1000, reg), value))

/** E1000 device data */
typedef struct {
	/** DDF device */
	ddf_dev_t *dev;
	/** Parent session */
	async_sess_t *parent_sess;
	/** Device configuration */
	e1000_info_t info;
	
	/** Physical registers base address */
	void *reg_base_phys;
	/** Virtual registers base address */
	void *reg_base_virt;
	
	/** Physical tx ring address */
	uintptr_t tx_ring_phys;
	/** Virtual tx ring address */
	void *tx_ring_virt;
	
	/** Ring of TX frames, physical address */
	uintptr_t *tx_frame_phys;
	/** Ring of TX frames, virtual address */
	void **tx_frame_virt;
	
	/** Physical rx ring address */
	uintptr_t rx_ring_phys;
	/** Virtual rx ring address */
	void *rx_ring_virt;
	
	/** Ring of RX frames, physical address */
	uintptr_t *rx_frame_phys;
	/** Ring of RX frames, virtual address */
	void **rx_frame_virt;
	
	/** VLAN tag */
	uint16_t vlan_tag;
	
	/** Add VLAN tag to frame */
	bool vlan_tag_add;
	
	/** Used unicast Receive Address count */
	unsigned int unicast_ra_count;
	
	/** Used milticast Receive addrress count */
	unsigned int multicast_ra_count;
	
	/** The irq assigned */
	int irq;
	
	/** Lock for CTRL register */
	fibril_mutex_t ctrl_lock;
	
	/** Lock for receiver */
	fibril_mutex_t rx_lock;
	
	/** Lock for transmitter */
	fibril_mutex_t tx_lock;
	
	/** Lock for EEPROM access */
	fibril_mutex_t eeprom_lock;
} e1000_t;

/** Global mutex for work with shared irq structure */
FIBRIL_MUTEX_INITIALIZE(irq_reg_mutex);

static errno_t e1000_get_address(e1000_t *, nic_address_t *);
static void e1000_eeprom_get_address(e1000_t *, nic_address_t *);
static errno_t e1000_set_addr(ddf_fun_t *, const nic_address_t *);

static errno_t e1000_defective_get_mode(ddf_fun_t *, uint32_t *);
static errno_t e1000_defective_set_mode(ddf_fun_t *, uint32_t);

static errno_t e1000_get_cable_state(ddf_fun_t *, nic_cable_state_t *);
static errno_t e1000_get_device_info(ddf_fun_t *, nic_device_info_t *);
static errno_t e1000_get_operation_mode(ddf_fun_t *, int *,
    nic_channel_mode_t *, nic_role_t *);
static errno_t e1000_set_operation_mode(ddf_fun_t *, int,
    nic_channel_mode_t, nic_role_t);
static errno_t e1000_autoneg_enable(ddf_fun_t *, uint32_t);
static errno_t e1000_autoneg_disable(ddf_fun_t *);
static errno_t e1000_autoneg_restart(ddf_fun_t *);

static errno_t e1000_vlan_set_tag(ddf_fun_t *, uint16_t, bool, bool);

/** Network interface options for E1000 card driver */
static nic_iface_t e1000_nic_iface;

/** Network interface options for E1000 card driver */
static nic_iface_t e1000_nic_iface = {
	.set_address = &e1000_set_addr,
	.get_device_info = &e1000_get_device_info,
	.get_cable_state = &e1000_get_cable_state,
	.get_operation_mode = &e1000_get_operation_mode,
	.set_operation_mode = &e1000_set_operation_mode,
	.autoneg_enable = &e1000_autoneg_enable,
	.autoneg_disable = &e1000_autoneg_disable,
	.autoneg_restart = &e1000_autoneg_restart,
	.vlan_set_tag = &e1000_vlan_set_tag,
	.defective_get_mode = &e1000_defective_get_mode,
	.defective_set_mode = &e1000_defective_set_mode,
};

/** Basic device operations for E1000 driver */
static ddf_dev_ops_t e1000_dev_ops;

static errno_t e1000_dev_add(ddf_dev_t *);

/** Basic driver operations for E1000 driver */
static driver_ops_t e1000_driver_ops = {
	.dev_add = e1000_dev_add
};

/** Driver structure for E1000 driver */
static driver_t e1000_driver = {
	.name = NAME,
	.driver_ops = &e1000_driver_ops
};

/* The default implementation callbacks */
static errno_t e1000_on_activating(nic_t *);
static errno_t e1000_on_stopping(nic_t *);
static void e1000_send_frame(nic_t *, void *, size_t);

/** PIO ranges used in the IRQ code. */
irq_pio_range_t e1000_irq_pio_ranges[] = {
	{
		.base = 0,
		.size =	PAGE_SIZE,	/* XXX */
	}
};

/** Commands to deal with interrupt
 *
 */
irq_cmd_t e1000_irq_commands[] = {
	{
		/* Get the interrupt status */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL,
		.dstarg = 2
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 2,
		.srcarg = 2
	},
	{
		/* Disable interrupts until interrupt routine is finished */
		.cmd = CMD_PIO_WRITE_32,
		.addr = NULL,
		.value = 0xffffffff
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** Interrupt code definition */
irq_code_t e1000_irq_code = {
	.rangecount = sizeof(e1000_irq_pio_ranges) /
	    sizeof(irq_pio_range_t),
	.ranges = e1000_irq_pio_ranges,
	.cmdcount = sizeof(e1000_irq_commands) / sizeof(irq_cmd_t),
	.cmds = e1000_irq_commands
};

/** Get the device information
 *
 * @param dev  NIC device
 * @param info Information to fill
 *
 * @return EOK
 *
 */
static errno_t e1000_get_device_info(ddf_fun_t *dev, nic_device_info_t *info)
{
	assert(dev);
	assert(info);
	
	memset(info, 0, sizeof(nic_device_info_t));
	
	info->vendor_id = 0x8086;
	str_cpy(info->vendor_name, NIC_VENDOR_MAX_LENGTH,
	    "Intel Corporation");
	str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH,
	    "Intel Pro");
	
	info->ethernet_support[ETH_10M] = ETH_10BASE_T;
	info->ethernet_support[ETH_100M] = ETH_100BASE_TX;
	info->ethernet_support[ETH_1000M] = ETH_1000BASE_T;
	
	return EOK;
}

/** Check the cable state
 *
 * @param[in]  dev   device
 * @param[out] state state to fill
 *
 * @return EOK
 *
 */
static errno_t e1000_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state)
{
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	if (E1000_REG_READ(e1000, E1000_STATUS) & (STATUS_LU))
		*state = NIC_CS_PLUGGED;
	else
		*state = NIC_CS_UNPLUGGED;
	
	return EOK;
}

static uint16_t e1000_calculate_itr_interval_from_usecs(suseconds_t useconds)
{
	return useconds * 4;
}

/** Get operation mode of the device
 *
 */
static errno_t e1000_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	uint32_t status = E1000_REG_READ(e1000, E1000_STATUS);
	
	if (status & STATUS_FD)
		*duplex = NIC_CM_FULL_DUPLEX;
	else
		*duplex = NIC_CM_HALF_DUPLEX;
	
	uint32_t speed_bits =
	    (status >> STATUS_SPEED_SHIFT) & STATUS_SPEED_ALL;
	
	if (speed_bits == STATUS_SPEED_10)
		*speed = 10;
	else if (speed_bits == STATUS_SPEED_100)
		*speed = 100;
	else if ((speed_bits == STATUS_SPEED_1000A) ||
	    (speed_bits == STATUS_SPEED_1000B))
		*speed = 1000;
	
	*role = NIC_ROLE_UNKNOWN;
	return EOK;
}

static void e1000_link_restart(e1000_t *e1000)
{
	fibril_mutex_lock(&e1000->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000, E1000_CTRL);
	
	if (ctrl & CTRL_SLU) {
		ctrl &= ~(CTRL_SLU);
		E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
		fibril_mutex_unlock(&e1000->ctrl_lock);
		
		async_usleep(10);
		
		fibril_mutex_lock(&e1000->ctrl_lock);
		ctrl = E1000_REG_READ(e1000, E1000_CTRL);
		ctrl |= CTRL_SLU;
		E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
	}
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
}

/** Set operation mode of the device
 *
 */
static errno_t e1000_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	if ((speed != 10) && (speed != 100) && (speed != 1000))
		return EINVAL;
	
	if ((duplex != NIC_CM_HALF_DUPLEX) && (duplex != NIC_CM_FULL_DUPLEX))
		return EINVAL;
	
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	
	fibril_mutex_lock(&e1000->ctrl_lock);
	uint32_t ctrl = E1000_REG_READ(e1000, E1000_CTRL);
	
	ctrl |= CTRL_FRCSPD;
	ctrl |= CTRL_FRCDPLX;
	ctrl &= ~(CTRL_ASDE);
	
	if (duplex == NIC_CM_FULL_DUPLEX)
		ctrl |= CTRL_FD;
	else
		ctrl &= ~(CTRL_FD);
	
	ctrl &= ~(CTRL_SPEED_MASK);
	if (speed == 1000)
		ctrl |= CTRL_SPEED_1000 << CTRL_SPEED_SHIFT;
	else if (speed == 100)
		ctrl |= CTRL_SPEED_100 << CTRL_SPEED_SHIFT;
	else
		ctrl |= CTRL_SPEED_10 << CTRL_SPEED_SHIFT;
	
	E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
	
	e1000_link_restart(e1000);
	
	return EOK;
}

/** Enable auto-negotiation
 *
 * @param dev           Device to update
 * @param advertisement Ignored on E1000
 *
 * @return EOK if advertisement mode set successfully
 *
 */
static errno_t e1000_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement)
{
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	
	fibril_mutex_lock(&e1000->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000, E1000_CTRL);
	
	ctrl &= ~(CTRL_FRCSPD);
	ctrl &= ~(CTRL_FRCDPLX);
	ctrl |= CTRL_ASDE;
	
	E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
	
	e1000_link_restart(e1000);
	
	return EOK;
}

/** Disable auto-negotiation
 *
 * @param dev Device to update
 *
 * @return EOK
 *
 */
static errno_t e1000_autoneg_disable(ddf_fun_t *fun)
{
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	
	fibril_mutex_lock(&e1000->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000, E1000_CTRL);
	
	ctrl |= CTRL_FRCSPD;
	ctrl |= CTRL_FRCDPLX;
	ctrl &= ~(CTRL_ASDE);
	
	E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
	
	e1000_link_restart(e1000);
	
	return EOK;
}

/** Restart auto-negotiation
 *
 * @param dev Device to update
 *
 * @return EOK if advertisement mode set successfully
 *
 */
static errno_t e1000_autoneg_restart(ddf_fun_t *dev)
{
	return e1000_autoneg_enable(dev, 0);
}

/** Get state of acceptance of weird frames
 *
 * @param      device Device to check
 * @param[out] mode   Current mode
 *
 */
static errno_t e1000_defective_get_mode(ddf_fun_t *fun, uint32_t *mode)
{
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	
	*mode = 0;
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	if (rctl & RCTL_SBP)
		*mode = NIC_DEFECTIVE_BAD_CRC | NIC_DEFECTIVE_SHORT;
	
	return EOK;
};

/** Set acceptance of weird frames
 *
 * @param device Device to update
 * @param mode   Mode to set
 *
 * @return ENOTSUP if the mode is not supported
 * @return EOK of mode was set
 *
 */
static errno_t e1000_defective_set_mode(ddf_fun_t *fun, uint32_t mode)
{
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	errno_t rc = EOK;
	
	fibril_mutex_lock(&e1000->rx_lock);
	
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	bool short_mode = (mode & NIC_DEFECTIVE_SHORT ? true : false);
	bool bad_mode = (mode & NIC_DEFECTIVE_BAD_CRC ? true : false);
	
	if (short_mode && bad_mode)
		rctl |= RCTL_SBP;
	else if ((!short_mode) && (!bad_mode))
		rctl &= ~RCTL_SBP;
	else
		rc = ENOTSUP;
	
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
	
	fibril_mutex_unlock(&e1000->rx_lock);
	return rc;
};

/** Write receive address to RA registr
 *
 * @param e1000      E1000 data structure
 * @param position   RA register position
 * @param address    Ethernet address
 * @param set_av_bit Set the Addtess Valid bit
 *
 */
static void e1000_write_receive_address(e1000_t *e1000, unsigned int position,
    const nic_address_t * address, bool set_av_bit)
{
	uint8_t *mac0 = (uint8_t *) address->address;
	uint8_t *mac1 = (uint8_t *) address->address + 1;
	uint8_t *mac2 = (uint8_t *) address->address + 2;
	uint8_t *mac3 = (uint8_t *) address->address + 3;
	uint8_t *mac4 = (uint8_t *) address->address + 4;
	uint8_t *mac5 = (uint8_t *) address->address + 5;
	
	uint32_t rah;
	uint32_t ral;
	
	ral = ((*mac3) << 24) | ((*mac2) << 16) | ((*mac1) << 8) | (*mac0);
	rah = ((*mac5) << 8) | ((*mac4));
	
	if (set_av_bit)
		rah |= RAH_AV;
	else
		rah |= E1000_REG_READ(e1000, E1000_RAH_ARRAY(position)) & RAH_AV;
	
	E1000_REG_WRITE(e1000, E1000_RAH_ARRAY(position), rah);
	E1000_REG_WRITE(e1000, E1000_RAL_ARRAY(position), ral);
}

/** Disable receive address in RA registr
 *
 *  Clear Address Valid bit
 *
 * @param e1000    E1000 data structure
 * @param position RA register position
 *
 */
static void e1000_disable_receive_address(e1000_t *e1000, unsigned int position)
{
	uint32_t rah = E1000_REG_READ(e1000, E1000_RAH_ARRAY(position));
	rah = rah & ~RAH_AV;
	E1000_REG_WRITE(e1000, E1000_RAH_ARRAY(position), rah);
}

/** Clear all unicast addresses from RA registers
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_clear_unicast_receive_addresses(e1000_t *e1000)
{
	for (unsigned int ra_num = 1;
	    ra_num <= e1000->unicast_ra_count;
	    ra_num++)
		e1000_disable_receive_address(e1000, ra_num);
	
	e1000->unicast_ra_count = 0;
}

/** Clear all multicast addresses from RA registers
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_clear_multicast_receive_addresses(e1000_t *e1000)
{
	unsigned int first_multicast_ra_num =
	    E1000_RECEIVE_ADDRESS - e1000->multicast_ra_count;
	
	for (unsigned int ra_num = E1000_RECEIVE_ADDRESS - 1;
	    ra_num >= first_multicast_ra_num;
	    ra_num--)
		e1000_disable_receive_address(e1000, ra_num);
	
	e1000->multicast_ra_count = 0;
}

/** Return receive address filter positions count usable for unicast
 *
 * @param e1000 E1000 data structure
 *
 * @return receive address filter positions count usable for unicast
 *
 */
static unsigned int get_free_unicast_address_count(e1000_t *e1000)
{
	return E1000_RECEIVE_ADDRESS - 1 - e1000->multicast_ra_count;
}

/** Return receive address filter positions count usable for multicast
 *
 * @param e1000 E1000 data structure
 *
 * @return receive address filter positions count usable for multicast
 *
 */
static unsigned int get_free_multicast_address_count(e1000_t *e1000)
{
	return E1000_RECEIVE_ADDRESS - 1 - e1000->unicast_ra_count;
}

/** Write unicast receive addresses to receive address filter registers
 *
 * @param e1000    E1000 data structure
 * @param addr     Pointer to address array
 * @param addr_cnt Address array count
 *
 */
static void e1000_add_unicast_receive_addresses(e1000_t *e1000,
    const nic_address_t *addr, size_t addr_cnt)
{
	assert(addr_cnt <= get_free_unicast_address_count(e1000));
	
	nic_address_t *addr_iterator = (nic_address_t *) addr;
	
	/* ra_num = 0 is primary address */
	for (unsigned int ra_num = 1;
	    ra_num <= addr_cnt;
	    ra_num++) {
		e1000_write_receive_address(e1000, ra_num, addr_iterator, true);
		addr_iterator++;
	}
}

/** Write multicast receive addresses to receive address filter registers
 *
 * @param e1000    E1000 data structure
 * @param addr     Pointer to address array
 * @param addr_cnt Address array count
 *
 */
static void e1000_add_multicast_receive_addresses(e1000_t *e1000,
    const nic_address_t *addr, size_t addr_cnt)
{
	assert(addr_cnt <= get_free_multicast_address_count(e1000));
	
	nic_address_t *addr_iterator = (nic_address_t *) addr;
	
	unsigned int first_multicast_ra_num = E1000_RECEIVE_ADDRESS - addr_cnt;
	for (unsigned int ra_num = E1000_RECEIVE_ADDRESS - 1;
	    ra_num >= first_multicast_ra_num;
	    ra_num--) {
		e1000_write_receive_address(e1000, ra_num, addr_iterator, true);
		addr_iterator++;
	}
}

/** Disable receiving frames for default address
 *
 * @param e1000 E1000 data structure
 *
 */
static void disable_ra0_address_filter(e1000_t *e1000)
{
	uint32_t rah0 = E1000_REG_READ(e1000, E1000_RAH_ARRAY(0));
	rah0 = rah0 & ~RAH_AV;
	E1000_REG_WRITE(e1000, E1000_RAH_ARRAY(0), rah0);
}

/** Enable receiving frames for default address
 *
 * @param e1000 E1000 data structure
 *
 */
static void enable_ra0_address_filter(e1000_t *e1000)
{
	uint32_t rah0 = E1000_REG_READ(e1000, E1000_RAH_ARRAY(0));
	rah0 = rah0 | RAH_AV;
	E1000_REG_WRITE(e1000, E1000_RAH_ARRAY(0), rah0);
}

/** Disable unicast promiscuous mode
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_disable_unicast_promisc(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl & ~RCTL_UPE;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Enable unicast promiscuous mode
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_enable_unicast_promisc(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl | RCTL_UPE;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Disable multicast promiscuous mode
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_disable_multicast_promisc(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl & ~RCTL_MPE;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Enable multicast promiscuous mode
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_enable_multicast_promisc(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl | RCTL_MPE;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Enable accepting of broadcast frames
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_enable_broadcast_accept(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl | RCTL_BAM;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Disable accepting of broadcast frames
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_disable_broadcast_accept(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl & ~RCTL_BAM;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Enable VLAN filtering according to VFTA registers
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_enable_vlan_filter(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl | RCTL_VFE;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Disable VLAN filtering
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_disable_vlan_filter(e1000_t *e1000)
{
	uint32_t rctl = E1000_REG_READ(e1000, E1000_RCTL);
	rctl = rctl & ~RCTL_VFE;
	E1000_REG_WRITE(e1000, E1000_RCTL, rctl);
}

/** Set multicast frames acceptance mode
 *
 * @param nic      NIC device to update
 * @param mode     Mode to set
 * @param addr     Address list (used in mode = NIC_MULTICAST_LIST)
 * @param addr_cnt Length of address list (used in mode = NIC_MULTICAST_LIST)
 *
 * @return EOK
 *
 */
static errno_t e1000_on_multicast_mode_change(nic_t *nic, nic_multicast_mode_t mode,
    const nic_address_t *addr, size_t addr_cnt)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	errno_t rc = EOK;
	
	fibril_mutex_lock(&e1000->rx_lock);
	
	switch (mode) {
	case NIC_MULTICAST_BLOCKED:
		e1000_clear_multicast_receive_addresses(e1000);
		e1000_disable_multicast_promisc(e1000);
		nic_report_hw_filtering(nic, -1, 1, -1);
		break;
	case NIC_MULTICAST_LIST:
		e1000_clear_multicast_receive_addresses(e1000);
		if (addr_cnt > get_free_multicast_address_count(e1000)) {
			/*
			 * Future work: fill MTA table
			 * Not strictly neccessary, it only saves some compares
			 * in the NIC library.
			 */
			e1000_enable_multicast_promisc(e1000);
			nic_report_hw_filtering(nic, -1, 0, -1);
		} else {
			e1000_disable_multicast_promisc(e1000);
			e1000_add_multicast_receive_addresses(e1000, addr, addr_cnt);
			nic_report_hw_filtering(nic, -1, 1, -1);
		}
		break;
	case NIC_MULTICAST_PROMISC:
		e1000_enable_multicast_promisc(e1000);
		e1000_clear_multicast_receive_addresses(e1000);
		nic_report_hw_filtering(nic, -1, 1, -1);
		break;
	default:
		rc = ENOTSUP;
		break;
	}
	
	fibril_mutex_unlock(&e1000->rx_lock);
	return rc;
}

/** Set unicast frames acceptance mode
 *
 * @param nic      NIC device to update
 * @param mode     Mode to set
 * @param addr     Address list (used in mode = NIC_MULTICAST_LIST)
 * @param addr_cnt Length of address list (used in mode = NIC_MULTICAST_LIST)
 *
 * @return EOK
 *
 */
static errno_t e1000_on_unicast_mode_change(nic_t *nic, nic_unicast_mode_t mode,
    const nic_address_t *addr, size_t addr_cnt)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	errno_t rc = EOK;
	
	fibril_mutex_lock(&e1000->rx_lock);
	
	switch (mode) {
	case NIC_UNICAST_BLOCKED:
		disable_ra0_address_filter(e1000);
		e1000_clear_unicast_receive_addresses(e1000);
		e1000_disable_unicast_promisc(e1000);
		nic_report_hw_filtering(nic, 1, -1, -1);
		break;
	case NIC_UNICAST_DEFAULT:
		enable_ra0_address_filter(e1000);
		e1000_clear_unicast_receive_addresses(e1000);
		e1000_disable_unicast_promisc(e1000);
		nic_report_hw_filtering(nic, 1, -1, -1);
		break;
	case NIC_UNICAST_LIST:
		enable_ra0_address_filter(e1000);
		e1000_clear_unicast_receive_addresses(e1000);
		if (addr_cnt > get_free_unicast_address_count(e1000)) {
			e1000_enable_unicast_promisc(e1000);
			nic_report_hw_filtering(nic, 0, -1, -1);
		} else {
			e1000_disable_unicast_promisc(e1000);
			e1000_add_unicast_receive_addresses(e1000, addr, addr_cnt);
			nic_report_hw_filtering(nic, 1, -1, -1);
		}
		break;
	case NIC_UNICAST_PROMISC:
		e1000_enable_unicast_promisc(e1000);
		enable_ra0_address_filter(e1000);
		e1000_clear_unicast_receive_addresses(e1000);
		nic_report_hw_filtering(nic, 1, -1, -1);
		break;
	default:
		rc = ENOTSUP;
		break;
	}
	
	fibril_mutex_unlock(&e1000->rx_lock);
	return rc;
}

/** Set broadcast frames acceptance mode
 *
 * @param nic  NIC device to update
 * @param mode Mode to set
 *
 * @return EOK
 *
 */
static errno_t e1000_on_broadcast_mode_change(nic_t *nic, nic_broadcast_mode_t mode)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	errno_t rc = EOK;
	
	fibril_mutex_lock(&e1000->rx_lock);
	
	switch (mode) {
	case NIC_BROADCAST_BLOCKED:
		e1000_disable_broadcast_accept(e1000);
		break;
	case NIC_BROADCAST_ACCEPTED:
		e1000_enable_broadcast_accept(e1000);
		break;
	default:
		rc = ENOTSUP;
		break;
	}
	
	fibril_mutex_unlock(&e1000->rx_lock);
	return rc;
}

/** Check if receiving is enabled
 *
 * @param e1000 E1000 data structure
 *
 * @return true if receiving is enabled
 *
 */
static bool e1000_is_rx_enabled(e1000_t *e1000)
{
	if (E1000_REG_READ(e1000, E1000_RCTL) & (RCTL_EN))
		return true;
	
	return false;
}

/** Enable receiving
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_enable_rx(e1000_t *e1000)
{
	/* Set Receive Enable Bit */
	E1000_REG_WRITE(e1000, E1000_RCTL,
	    E1000_REG_READ(e1000, E1000_RCTL) | (RCTL_EN));
}

/** Disable receiving
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_disable_rx(e1000_t *e1000)
{
	/* Clear Receive Enable Bit */
	E1000_REG_WRITE(e1000, E1000_RCTL,
	    E1000_REG_READ(e1000, E1000_RCTL) & ~(RCTL_EN));
}

/** Set VLAN mask
 *
 * @param nic       NIC device to update
 * @param vlan_mask VLAN mask
 *
 */
static void e1000_on_vlan_mask_change(nic_t *nic,
    const nic_vlan_mask_t *vlan_mask)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	fibril_mutex_lock(&e1000->rx_lock);
	
	if (vlan_mask) {
		/*
		 * Disable receiving, so that frame matching
		 * partially written VLAN is not received.
		 */
		bool rx_enabled = e1000_is_rx_enabled(e1000);
		if (rx_enabled)
			e1000_disable_rx(e1000);
		
		for (unsigned int i = 0; i < NIC_VLAN_BITMAP_SIZE; i += 4) {
			uint32_t bitmap_part =
			    ((uint32_t) vlan_mask->bitmap[i]) |
			    (((uint32_t) vlan_mask->bitmap[i + 1]) << 8) |
			    (((uint32_t) vlan_mask->bitmap[i + 2]) << 16) |
			    (((uint32_t) vlan_mask->bitmap[i + 3]) << 24);
			E1000_REG_WRITE(e1000, E1000_VFTA_ARRAY(i / 4), bitmap_part);
		}
		
		e1000_enable_vlan_filter(e1000);
		if (rx_enabled)
			e1000_enable_rx(e1000);
	} else
		e1000_disable_vlan_filter(e1000);
	
	fibril_mutex_unlock(&e1000->rx_lock);
}

/** Set VLAN mask
 *
 * @param device E1000 device
 * @param tag    VLAN tag
 *
 * @return EOK
 * @return ENOTSUP
 *
 */
static errno_t e1000_vlan_set_tag(ddf_fun_t *fun, uint16_t tag, bool add,
    bool strip)
{
	/* VLAN CFI bit cannot be set */
	if (tag & VLANTAG_CFI)
		return ENOTSUP;
	
	/*
	 * CTRL.VME is neccessary for both strip and add
	 * but CTRL.VME means stripping tags on receive.
	 */
	if (!strip && add)
		return ENOTSUP;
	
	e1000_t *e1000 = DRIVER_DATA_FUN(fun);
	
	e1000->vlan_tag = tag;
	e1000->vlan_tag_add = add;
	
	fibril_mutex_lock(&e1000->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000, E1000_CTRL);
	if (strip)
		ctrl |= CTRL_VME;
	else
		ctrl &= ~CTRL_VME;
	
	E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
	return EOK;
}

/** Fill receive descriptor with new empty buffer
 *
 * Store frame in e1000->rx_frame_phys
 *
 * @param nic    NIC data stricture
 * @param offset Receive descriptor offset
 *
 */
static void e1000_fill_new_rx_descriptor(nic_t *nic, size_t offset)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	e1000_rx_descriptor_t *rx_descriptor = (e1000_rx_descriptor_t *)
	    (e1000->rx_ring_virt + offset * sizeof(e1000_rx_descriptor_t));
	
	rx_descriptor->phys_addr = PTR_TO_U64(e1000->rx_frame_phys[offset]);
	rx_descriptor->length = 0;
	rx_descriptor->checksum = 0;
	rx_descriptor->status = 0;
	rx_descriptor->errors = 0;
	rx_descriptor->special = 0;
}

/** Clear receive descriptor
 *
 * @param e1000  E1000 data
 * @param offset Receive descriptor offset
 *
 */
static void e1000_clear_rx_descriptor(e1000_t *e1000, unsigned int offset)
{
	e1000_rx_descriptor_t *rx_descriptor = (e1000_rx_descriptor_t *)
	    (e1000->rx_ring_virt + offset * sizeof(e1000_rx_descriptor_t));
	
	rx_descriptor->length = 0;
	rx_descriptor->checksum = 0;
	rx_descriptor->status = 0;
	rx_descriptor->errors = 0;
	rx_descriptor->special = 0;
}

/** Clear receive descriptor
 *
 * @param nic    NIC data
 * @param offset Receive descriptor offset
 *
 */
static void e1000_clear_tx_descriptor(nic_t *nic, unsigned int offset)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	e1000_tx_descriptor_t *tx_descriptor = (e1000_tx_descriptor_t *)
	    (e1000->tx_ring_virt + offset * sizeof(e1000_tx_descriptor_t));
	
	tx_descriptor->phys_addr = 0;
	tx_descriptor->length = 0;
	tx_descriptor->checksum_offset = 0;
	tx_descriptor->command = 0;
	tx_descriptor->status = 0;
	tx_descriptor->checksum_start_field = 0;
	tx_descriptor->special = 0;
}

/** Increment tail pointer for receive or transmit ring
 *
 * @param tail              Old Tail
 * @param descriptors_count Ring length
 *
 * @return New tail
 *
 */
static uint32_t e1000_inc_tail(uint32_t tail, uint32_t descriptors_count)
{
	if (tail + 1 == descriptors_count)
		return 0;
	else
		return tail + 1;
}

/** Receive frames
 *
 * @param nic NIC data
 *
 */
static void e1000_receive_frames(nic_t *nic)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	fibril_mutex_lock(&e1000->rx_lock);
	
	uint32_t *tail_addr = E1000_REG_ADDR(e1000, E1000_RDT);
	uint32_t next_tail = e1000_inc_tail(*tail_addr, E1000_RX_FRAME_COUNT);
	
	e1000_rx_descriptor_t *rx_descriptor = (e1000_rx_descriptor_t *)
	    (e1000->rx_ring_virt + next_tail * sizeof(e1000_rx_descriptor_t));
	
	while (rx_descriptor->status & 0x01) {
		uint32_t frame_size = rx_descriptor->length - E1000_CRC_SIZE;
		
		nic_frame_t *frame = nic_alloc_frame(nic, frame_size);
		if (frame != NULL) {
			memcpy(frame->data, e1000->rx_frame_virt[next_tail], frame_size);
			nic_received_frame(nic, frame);
		} else {
			ddf_msg(LVL_ERROR, "Memory allocation failed. Frame dropped.");
		}
		
		e1000_fill_new_rx_descriptor(nic, next_tail);
		
		*tail_addr = e1000_inc_tail(*tail_addr, E1000_RX_FRAME_COUNT);
		next_tail = e1000_inc_tail(*tail_addr, E1000_RX_FRAME_COUNT);
		
		rx_descriptor = (e1000_rx_descriptor_t *)
		    (e1000->rx_ring_virt + next_tail * sizeof(e1000_rx_descriptor_t));
	}
	
	fibril_mutex_unlock(&e1000->rx_lock);
}

/** Enable E1000 interupts
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_enable_interrupts(e1000_t *e1000)
{
	E1000_REG_WRITE(e1000, E1000_IMS, ICR_RXT0);
}

/** Disable E1000 interupts
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_disable_interrupts(e1000_t *e1000)
{
	E1000_REG_WRITE(e1000, E1000_IMS, 0);
}

/** Interrupt handler implementation
 *
 * This function is called from e1000_interrupt_handler()
 * and e1000_poll()
 *
 * @param nic NIC data
 * @param icr ICR register value
 *
 */
static void e1000_interrupt_handler_impl(nic_t *nic, uint32_t icr)
{
	if (icr & ICR_RXT0)
		e1000_receive_frames(nic);
}

/** Handle device interrupt
 *
 * @param icall IPC call structure
 * @param dev   E1000 device
 *
 */
static void e1000_interrupt_handler(ipc_call_t *icall,
    ddf_dev_t *dev)
{
	uint32_t icr = (uint32_t) IPC_GET_ARG2(*icall);
	nic_t *nic = NIC_DATA_DEV(dev);
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	e1000_interrupt_handler_impl(nic, icr);
	e1000_enable_interrupts(e1000);
}

/** Register interrupt handler for the card in the system
 *
 * Note: The global irq_reg_mutex is locked because of work with global
 * structure.
 *
 * @param nic Driver data
 *
 * @param[out] handle  IRQ capability handle if the handler was registered
 *
 * @return An error code otherwise
 *
 */
inline static errno_t e1000_register_int_handler(nic_t *nic, cap_handle_t *handle)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	/* Lock the mutex in whole driver while working with global structure */
	fibril_mutex_lock(&irq_reg_mutex);
	
	e1000_irq_code.ranges[0].base = (uintptr_t) e1000->reg_base_phys;
	e1000_irq_code.cmds[0].addr = e1000->reg_base_phys + E1000_ICR;
	e1000_irq_code.cmds[2].addr = e1000->reg_base_phys + E1000_IMC;
	
	errno_t rc = register_interrupt_handler(nic_get_ddf_dev(nic), e1000->irq,
	    e1000_interrupt_handler, &e1000_irq_code, handle);
	
	fibril_mutex_unlock(&irq_reg_mutex);
	return rc;
}

/** Force receiving all frames in the receive buffer
 *
 * @param nic NIC data
 *
 */
static void e1000_poll(nic_t *nic)
{
	assert(nic);
	
	e1000_t *e1000 = nic_get_specific(nic);
	assert(e1000);
	
	uint32_t icr = E1000_REG_READ(e1000, E1000_ICR);
	e1000_interrupt_handler_impl(nic, icr);
}

/** Calculates ITR register interrupt from timeval structure
 *
 * @param period Period
 *
 */
static uint16_t e1000_calculate_itr_interval(const struct timeval *period)
{
	// TODO: use also tv_sec
	return e1000_calculate_itr_interval_from_usecs(period->tv_usec);
}

/** Set polling mode
 *
 * @param device  Device to set
 * @param mode    Mode to set
 * @param period  Period for NIC_POLL_PERIODIC
 *
 * @return EOK if succeed
 * @return ENOTSUP if the mode is not supported
 *
 */
static errno_t e1000_poll_mode_change(nic_t *nic, nic_poll_mode_t mode,
    const struct timeval *period)
{
	assert(nic);
	
	e1000_t *e1000 = nic_get_specific(nic);
	assert(e1000);
	
	switch (mode) {
	case NIC_POLL_IMMEDIATE:
		E1000_REG_WRITE(e1000, E1000_ITR, 0);
		e1000_enable_interrupts(e1000);
		break;
	case NIC_POLL_ON_DEMAND:
		e1000_disable_interrupts(e1000);
		break;
	case NIC_POLL_PERIODIC:
		assert(period);
		uint16_t itr_interval = e1000_calculate_itr_interval(period);
		E1000_REG_WRITE(e1000, E1000_ITR, (uint32_t) itr_interval);
		e1000_enable_interrupts(e1000);
		break;
	default:
		return ENOTSUP;
	}
	
	return EOK;
}

/** Initialize receive registers
 *
 * @param e1000 E1000 data structure
 *
 */
static void e1000_initialize_rx_registers(e1000_t *e1000)
{
	E1000_REG_WRITE(e1000, E1000_RDLEN, E1000_RX_FRAME_COUNT * 16);
	E1000_REG_WRITE(e1000, E1000_RDH, 0);
	
	/* It is not posible to let HW use all descriptors */
	E1000_REG_WRITE(e1000, E1000_RDT, E1000_RX_FRAME_COUNT - 1);
	
	/* Set Broadcast Enable Bit */
	E1000_REG_WRITE(e1000, E1000_RCTL, RCTL_BAM);
}

/** Initialize receive structure
 *
 * @param nic NIC data
 *
 * @return EOK if succeed
 * @return An error code otherwise
 *
 */
static errno_t e1000_initialize_rx_structure(nic_t *nic)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	fibril_mutex_lock(&e1000->rx_lock);
	
	e1000->rx_ring_virt = AS_AREA_ANY;
	errno_t rc = dmamem_map_anonymous(
	    E1000_RX_FRAME_COUNT * sizeof(e1000_rx_descriptor_t),
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &e1000->rx_ring_phys, &e1000->rx_ring_virt);
	if (rc != EOK)
		return rc;
	
	E1000_REG_WRITE(e1000, E1000_RDBAH,
	    (uint32_t) (PTR_TO_U64(e1000->rx_ring_phys) >> 32));
	E1000_REG_WRITE(e1000, E1000_RDBAL,
	    (uint32_t) PTR_TO_U64(e1000->rx_ring_phys));
	
	e1000->rx_frame_phys = (uintptr_t *)
	    calloc(E1000_RX_FRAME_COUNT, sizeof(uintptr_t));
	e1000->rx_frame_virt =
	    calloc(E1000_RX_FRAME_COUNT, sizeof(void *));
	if ((e1000->rx_frame_phys == NULL) || (e1000->rx_frame_virt == NULL)) {
		rc = ENOMEM;
		goto error;
	}
	
	for (size_t i = 0; i < E1000_RX_FRAME_COUNT; i++) {
		uintptr_t frame_phys;
		void *frame_virt = AS_AREA_ANY;
		
		rc = dmamem_map_anonymous(E1000_MAX_SEND_FRAME_SIZE,
		    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
		    &frame_phys, &frame_virt);
		if (rc != EOK)
			goto error;
		
		e1000->rx_frame_phys[i] = frame_phys;
		e1000->rx_frame_virt[i] = frame_virt;
	}
	
	/* Write descriptor */
	for (size_t i = 0; i < E1000_RX_FRAME_COUNT; i++)
		e1000_fill_new_rx_descriptor(nic, i);
	
	e1000_initialize_rx_registers(e1000);
	
	fibril_mutex_unlock(&e1000->rx_lock);
	return EOK;
	
error:
	for (size_t i = 0; i < E1000_RX_FRAME_COUNT; i++) {
		if (e1000->rx_frame_virt[i] != NULL) {
			dmamem_unmap_anonymous(e1000->rx_frame_virt[i]);
			e1000->rx_frame_phys[i] = 0;
			e1000->rx_frame_virt[i] = NULL;
		}
	}
	
	if (e1000->rx_frame_phys != NULL) {
		free(e1000->rx_frame_phys);
		e1000->rx_frame_phys = NULL;
	}
	
	if (e1000->rx_frame_virt != NULL) {
		free(e1000->rx_frame_virt);
		e1000->rx_frame_virt = NULL;
	}
	
	return rc;
}

/** Uninitialize receive structure
 *
 * @param nic NIC data
 *
 */
static void e1000_uninitialize_rx_structure(nic_t *nic)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	/* Write descriptor */
	for (unsigned int offset = 0; offset < E1000_RX_FRAME_COUNT; offset++) {
		dmamem_unmap_anonymous(e1000->rx_frame_virt[offset]);
		e1000->rx_frame_phys[offset] = 0;
		e1000->rx_frame_virt[offset] = NULL;
	}
	
	free(e1000->rx_frame_virt);
	
	e1000->rx_frame_phys = NULL;
	e1000->rx_frame_virt = NULL;
	
	dmamem_unmap_anonymous(e1000->rx_ring_virt);
}

/** Clear receive descriptor ring
 *
 * @param e1000 E1000 data
 *
 */
static void e1000_clear_rx_ring(e1000_t *e1000)
{
	/* Write descriptor */
	for (unsigned int offset = 0;
	    offset < E1000_RX_FRAME_COUNT;
	    offset++)
		e1000_clear_rx_descriptor(e1000, offset);
}

/** Initialize filters
 *
 * @param e1000 E1000 data
 *
 */
static void e1000_initialize_filters(e1000_t *e1000)
{
	/* Initialize address filter */
	e1000->unicast_ra_count = 0;
	e1000->multicast_ra_count = 0;
	e1000_clear_unicast_receive_addresses(e1000);
}

/** Initialize VLAN
 *
 * @param e1000 E1000 data
 *
 */
static void e1000_initialize_vlan(e1000_t *e1000)
{
	e1000->vlan_tag_add = false;
}

/** Fill MAC address from EEPROM to RA[0] register
 *
 * @param e1000 E1000 data
 *
 */
static void e1000_fill_mac_from_eeprom(e1000_t *e1000)
{
	/* MAC address from eeprom to RA[0] */
	nic_address_t address;
	e1000_eeprom_get_address(e1000, &address);
	e1000_write_receive_address(e1000, 0, &address, true);
}

/** Initialize other registers
 *
 * @param dev E1000 data.
 *
 * @return EOK if succeed
 * @return An error code otherwise
 *
 */
static void e1000_initialize_registers(e1000_t *e1000)
{
	E1000_REG_WRITE(e1000, E1000_ITR,
	    e1000_calculate_itr_interval_from_usecs(
	    E1000_DEFAULT_INTERRUPT_INTERVAL_USEC));
	E1000_REG_WRITE(e1000, E1000_FCAH, 0);
	E1000_REG_WRITE(e1000, E1000_FCAL, 0);
	E1000_REG_WRITE(e1000, E1000_FCT, 0);
	E1000_REG_WRITE(e1000, E1000_FCTTV, 0);
	E1000_REG_WRITE(e1000, E1000_VET, VET_VALUE);
	E1000_REG_WRITE(e1000, E1000_CTRL, CTRL_ASDE);
}

/** Initialize transmit registers
 *
 * @param e1000 E1000 data.
 *
 */
static void e1000_initialize_tx_registers(e1000_t *e1000)
{
	E1000_REG_WRITE(e1000, E1000_TDLEN, E1000_TX_FRAME_COUNT * 16);
	E1000_REG_WRITE(e1000, E1000_TDH, 0);
	E1000_REG_WRITE(e1000, E1000_TDT, 0);
	
	E1000_REG_WRITE(e1000, E1000_TIPG,
	    10 << TIPG_IPGT_SHIFT |
	    8 << TIPG_IPGR1_SHIFT |
	    6 << TIPG_IPGR2_SHIFT);
	
	E1000_REG_WRITE(e1000, E1000_TCTL,
	    0x0F << TCTL_CT_SHIFT /* Collision Threshold */ |
	    0x40 << TCTL_COLD_SHIFT /* Collision Distance */ |
	    TCTL_PSP /* Pad Short Packets */);
}

/** Initialize transmit structure
 *
 * @param e1000 E1000 data.
 *
 */
static errno_t e1000_initialize_tx_structure(e1000_t *e1000)
{
	size_t i;
	
	fibril_mutex_lock(&e1000->tx_lock);
	
	e1000->tx_ring_phys = 0;
	e1000->tx_ring_virt = AS_AREA_ANY;
	
	e1000->tx_frame_phys = NULL;
	e1000->tx_frame_virt = NULL;
	
	errno_t rc = dmamem_map_anonymous(
	    E1000_TX_FRAME_COUNT * sizeof(e1000_tx_descriptor_t),
	    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &e1000->tx_ring_phys, &e1000->tx_ring_virt);
	if (rc != EOK)
		goto error;
	
	memset(e1000->tx_ring_virt, 0,
	    E1000_TX_FRAME_COUNT * sizeof(e1000_tx_descriptor_t));
	
	e1000->tx_frame_phys = (uintptr_t *)
	    calloc(E1000_TX_FRAME_COUNT, sizeof(uintptr_t));
	e1000->tx_frame_virt =
	    calloc(E1000_TX_FRAME_COUNT, sizeof(void *));

	if ((e1000->tx_frame_phys == NULL) || (e1000->tx_frame_virt == NULL)) {
		rc = ENOMEM;
		goto error;
	}
	
	for (i = 0; i < E1000_TX_FRAME_COUNT; i++) {
		e1000->tx_frame_virt[i] = AS_AREA_ANY;
		rc = dmamem_map_anonymous(E1000_MAX_SEND_FRAME_SIZE,
		    DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE,
		    0, &e1000->tx_frame_phys[i], &e1000->tx_frame_virt[i]);
		if (rc != EOK)
			goto error;
	}
	
	E1000_REG_WRITE(e1000, E1000_TDBAH,
	    (uint32_t) (PTR_TO_U64(e1000->tx_ring_phys) >> 32));
	E1000_REG_WRITE(e1000, E1000_TDBAL,
	    (uint32_t) PTR_TO_U64(e1000->tx_ring_phys));
	
	e1000_initialize_tx_registers(e1000);
	
	fibril_mutex_unlock(&e1000->tx_lock);
	return EOK;
	
error:
	if (e1000->tx_ring_virt != NULL) {
		dmamem_unmap_anonymous(e1000->tx_ring_virt);
		e1000->tx_ring_virt = NULL;
	}
	
	if ((e1000->tx_frame_phys != NULL) && (e1000->tx_frame_virt != NULL)) {
		for (i = 0; i < E1000_TX_FRAME_COUNT; i++) {
			if (e1000->tx_frame_virt[i] != NULL) {
				dmamem_unmap_anonymous(e1000->tx_frame_virt[i]);
				e1000->tx_frame_phys[i] = 0;
				e1000->tx_frame_virt[i] = NULL;
			}
		}
	}
	
	if (e1000->tx_frame_phys != NULL) {
		free(e1000->tx_frame_phys);
		e1000->tx_frame_phys = NULL;
	}
	
	if (e1000->tx_frame_virt != NULL) {
		free(e1000->tx_frame_virt);
		e1000->tx_frame_virt = NULL;
	}
	
	return rc;
}

/** Uninitialize transmit structure
 *
 * @param nic NIC data
 *
 */
static void e1000_uninitialize_tx_structure(e1000_t *e1000)
{
	size_t i;
	
	for (i = 0; i < E1000_TX_FRAME_COUNT; i++) {
		dmamem_unmap_anonymous(e1000->tx_frame_virt[i]);
		e1000->tx_frame_phys[i] = 0;
		e1000->tx_frame_virt[i] = NULL;
	}
	
	if (e1000->tx_frame_phys != NULL) {
		free(e1000->tx_frame_phys);
		e1000->tx_frame_phys = NULL;
	}
	
	if (e1000->tx_frame_virt != NULL) {
		free(e1000->tx_frame_virt);
		e1000->tx_frame_virt = NULL;
	}
	
	dmamem_unmap_anonymous(e1000->tx_ring_virt);
}

/** Clear transmit descriptor ring
 *
 * @param nic NIC data
 *
 */
static void e1000_clear_tx_ring(nic_t *nic)
{
	/* Write descriptor */
	for (unsigned int offset = 0;
	    offset < E1000_TX_FRAME_COUNT;
	    offset++)
		e1000_clear_tx_descriptor(nic, offset);
}

/** Enable transmit
 *
 * @param e1000 E1000 data
 *
 */
static void e1000_enable_tx(e1000_t *e1000)
{
	/* Set Transmit Enable Bit */
	E1000_REG_WRITE(e1000, E1000_TCTL,
	    E1000_REG_READ(e1000, E1000_TCTL) | (TCTL_EN));
}

/** Disable transmit
 *
 * @param e1000 E1000 data
 *
 */
static void e1000_disable_tx(e1000_t *e1000)
{
	/* Clear Transmit Enable Bit */
	E1000_REG_WRITE(e1000, E1000_TCTL,
	    E1000_REG_READ(e1000, E1000_TCTL) & ~(TCTL_EN));
}

/** Reset E1000 device
 *
 * @param e1000 The E1000 data
 *
 */
static errno_t e1000_reset(nic_t *nic)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	E1000_REG_WRITE(e1000, E1000_CTRL, CTRL_RST);
	
	/* Wait for the reset */
	async_usleep(20);
	
	/* check if RST_BIT cleared */
	if (E1000_REG_READ(e1000, E1000_CTRL) & (CTRL_RST))
		return EINVAL;
	
	e1000_initialize_registers(e1000);
	e1000_initialize_rx_registers(e1000);
	e1000_initialize_tx_registers(e1000);
	e1000_fill_mac_from_eeprom(e1000);
	e1000_initialize_filters(e1000);
	e1000_initialize_vlan(e1000);
	
	return EOK;
}

/** Activate the device to receive and transmit frames
 *
 * @param nic NIC driver data
 *
 * @return EOK if activated successfully
 * @return Error code otherwise
 *
 */
static errno_t e1000_on_activating(nic_t *nic)
{
	assert(nic);
	
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	fibril_mutex_lock(&e1000->rx_lock);
	fibril_mutex_lock(&e1000->tx_lock);
	fibril_mutex_lock(&e1000->ctrl_lock);
	
	e1000_enable_interrupts(e1000);
	
	errno_t rc = hw_res_enable_interrupt(e1000->parent_sess, e1000->irq);
	if (rc != EOK) {
		e1000_disable_interrupts(e1000);
		fibril_mutex_unlock(&e1000->ctrl_lock);
		fibril_mutex_unlock(&e1000->tx_lock);
		fibril_mutex_unlock(&e1000->rx_lock);
		return rc;
	}
	
	e1000_clear_rx_ring(e1000);
	e1000_enable_rx(e1000);
	
	e1000_clear_tx_ring(nic);
	e1000_enable_tx(e1000);
	
	uint32_t ctrl = E1000_REG_READ(e1000, E1000_CTRL);
	ctrl |= CTRL_SLU;
	E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
	fibril_mutex_unlock(&e1000->tx_lock);
	fibril_mutex_unlock(&e1000->rx_lock);
	
	return EOK;
}

/** Callback for NIC_STATE_DOWN change
 *
 * @param nic NIC driver data
 *
 * @return EOK if succeed
 * @return Error code otherwise
 *
 */
static errno_t e1000_on_down_unlocked(nic_t *nic)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	uint32_t ctrl = E1000_REG_READ(e1000, E1000_CTRL);
	ctrl &= ~CTRL_SLU;
	E1000_REG_WRITE(e1000, E1000_CTRL, ctrl);
	
	e1000_disable_tx(e1000);
	e1000_disable_rx(e1000);
	
	hw_res_disable_interrupt(e1000->parent_sess, e1000->irq);
	e1000_disable_interrupts(e1000);
	
	/*
	 * Wait for the for the end of all data
	 * transfers to descriptors.
	 */
	async_usleep(100);
	
	return EOK;
}

/** Callback for NIC_STATE_DOWN change
 *
 * @param nic NIC driver data
 *
 * @return EOK if succeed
 * @return Error code otherwise
 *
 */
static errno_t e1000_on_down(nic_t *nic)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	fibril_mutex_lock(&e1000->rx_lock);
	fibril_mutex_lock(&e1000->tx_lock);
	fibril_mutex_lock(&e1000->ctrl_lock);
	
	errno_t rc = e1000_on_down_unlocked(nic);
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
	fibril_mutex_unlock(&e1000->tx_lock);
	fibril_mutex_unlock(&e1000->rx_lock);
	
	return rc;
}

/** Callback for NIC_STATE_STOPPED change
 *
 * @param nic NIC driver data
 *
 * @return EOK if succeed
 * @return Error code otherwise
 *
 */
static errno_t e1000_on_stopping(nic_t *nic)
{
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	fibril_mutex_lock(&e1000->rx_lock);
	fibril_mutex_lock(&e1000->tx_lock);
	fibril_mutex_lock(&e1000->ctrl_lock);
	
	errno_t rc = e1000_on_down_unlocked(nic);
	if (rc == EOK)
		rc = e1000_reset(nic);
	
	fibril_mutex_unlock(&e1000->ctrl_lock);
	fibril_mutex_unlock(&e1000->tx_lock);
	fibril_mutex_unlock(&e1000->rx_lock);
	
	return rc;
}

/** Create driver data structure
 *
 * @return Intialized device data structure or NULL
 *
 */
static e1000_t *e1000_create_dev_data(ddf_dev_t *dev)
{
	nic_t *nic = nic_create_and_bind(dev);
	if (!nic)
		return NULL;
	
	e1000_t *e1000 = malloc(sizeof(e1000_t));
	if (!e1000) {
		nic_unbind_and_destroy(dev);
		return NULL;
	}
	
	memset(e1000, 0, sizeof(e1000_t));
	e1000->dev = dev;
	
	nic_set_specific(nic, e1000);
	nic_set_send_frame_handler(nic, e1000_send_frame);
	nic_set_state_change_handlers(nic, e1000_on_activating,
	    e1000_on_down, e1000_on_stopping);
	nic_set_filtering_change_handlers(nic,
	    e1000_on_unicast_mode_change, e1000_on_multicast_mode_change,
	    e1000_on_broadcast_mode_change, NULL, e1000_on_vlan_mask_change);
	nic_set_poll_handlers(nic, e1000_poll_mode_change, e1000_poll);
	
	fibril_mutex_initialize(&e1000->ctrl_lock);
	fibril_mutex_initialize(&e1000->rx_lock);
	fibril_mutex_initialize(&e1000->tx_lock);
	fibril_mutex_initialize(&e1000->eeprom_lock);
	
	return e1000;
}

/** Delete driver data structure
 *
 * @param data E1000 device data structure
 *
 */
inline static void e1000_delete_dev_data(ddf_dev_t *dev)
{
	assert(dev);
	
	if (ddf_dev_data_get(dev) != NULL)
		nic_unbind_and_destroy(dev);
}

/** Clean up the E1000 device structure.
 *
 * @param dev Device structure.
 *
 */
static void e1000_dev_cleanup(ddf_dev_t *dev)
{
	assert(dev);
	
	e1000_delete_dev_data(dev);
}

/** Fill the irq and io_addr part of device data structure
 *
 * The hw_resources must be obtained before calling this function
 *
 * @param dev          Device structure
 * @param hw_resources Hardware resources obtained from the parent device
 *
 * @return EOK if succeed
 * @return An error code otherwise
 *
 */
static errno_t e1000_fill_resource_info(ddf_dev_t *dev,
    const hw_res_list_parsed_t *hw_resources)
{
	e1000_t *e1000 = DRIVER_DATA_DEV(dev);
	
	if (hw_resources->irqs.count != 1)
		return EINVAL;
	
	e1000->irq = hw_resources->irqs.irqs[0];
	e1000->reg_base_phys =
	    MEMADDR_TO_PTR(RNGABS(hw_resources->mem_ranges.ranges[0]));
	
	return EOK;
}

/** Obtain information about hardware resources of the device
 *
 * The device must be connected to the parent
 *
 * @param dev Device structure
 *
 * @return EOK if succeed
 * @return An error code otherwise
 *
 */
static errno_t e1000_get_resource_info(ddf_dev_t *dev)
{
	assert(dev != NULL);
	assert(NIC_DATA_DEV(dev) != NULL);
	
	hw_res_list_parsed_t hw_res_parsed;
	hw_res_list_parsed_init(&hw_res_parsed);
	
	/* Get hw resources form parent driver */
	errno_t rc = nic_get_resources(NIC_DATA_DEV(dev), &hw_res_parsed);
	if (rc != EOK)
		return rc;
	
	/* Fill resources information to the device */
	rc = e1000_fill_resource_info(dev, &hw_res_parsed);
	hw_res_list_parsed_clean(&hw_res_parsed);
	
	return rc;
}

/** Initialize the E1000 device structure
 *
 * @param dev Device information
 *
 * @return EOK if succeed
 * @return An error code otherwise
 *
 */
static errno_t e1000_device_initialize(ddf_dev_t *dev)
{
	/* Allocate driver data for the device. */
	e1000_t *e1000 = e1000_create_dev_data(dev);
	if (e1000 == NULL) {
		ddf_msg(LVL_ERROR, "Unable to allocate device softstate");
		return ENOMEM;
	}
	
	e1000->parent_sess = ddf_dev_parent_sess_get(dev);
	if (e1000->parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Failed connecting parent device.");
		return EIO;
	}
	
	/* Obtain and fill hardware resources info */
	errno_t rc = e1000_get_resource_info(dev);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot obtain hardware resources");
		e1000_dev_cleanup(dev);
		return rc;
	}
	
	uint16_t device_id;
	rc = pci_config_space_read_16(ddf_dev_parent_sess_get(dev), PCI_DEVICE_ID,
	    &device_id);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot access PCI configuration space");
		e1000_dev_cleanup(dev);
		return rc;
	}
	
	e1000_board_t board;
	switch (device_id) {
	case 0x100e:
	case 0x1015:
	case 0x1016:
	case 0x1017:
		board = E1000_82540;
		break;
	case 0x1013:
	case 0x1018:
	case 0x1078:
		board = E1000_82541;
		break;
	case 0x1076:
	case 0x1077:
	case 0x107c:
		board = E1000_82541REV2;
		break;
	case 0x100f:
	case 0x1011:
	case 0x1026:
	case 0x1027:
	case 0x1028:
		board = E1000_82545;
		break;
	case 0x1010:
	case 0x1012:
	case 0x101d:
	case 0x1079:
	case 0x107a:
	case 0x107b:
		board = E1000_82546;
		break;
	case 0x1019:
	case 0x101a:
		board = E1000_82547;
		break;
	case 0x10b9:
		board = E1000_82572;
		break;
	case 0x1096:
		board = E1000_80003ES2;
		break;
	default:
		ddf_msg(LVL_ERROR, "Device not supported (%#" PRIx16 ")",
		    device_id);
		e1000_dev_cleanup(dev);
		return ENOTSUP;
	}
	
	switch (board) {
	case E1000_82540:
	case E1000_82541:
	case E1000_82541REV2:
	case E1000_82545:
	case E1000_82546:
		e1000->info.eerd_start = 0x01;
		e1000->info.eerd_done = 0x10;
		e1000->info.eerd_address_offset = 8;
		e1000->info.eerd_data_offset = 16;
		break;
	case E1000_82547:
	case E1000_82572:
	case E1000_80003ES2:
		e1000->info.eerd_start = 0x01;
		e1000->info.eerd_done = 0x02;
		e1000->info.eerd_address_offset = 2;
		e1000->info.eerd_data_offset = 16;
		break;
	}
	
	return EOK;
}

/** Enable the I/O ports of the device.
 *
 * @param dev E1000 device.
 *
 * @return EOK if successed
 * @return An error code otherwise
 *
 */
static errno_t e1000_pio_enable(ddf_dev_t *dev)
{
	e1000_t *e1000 = DRIVER_DATA_DEV(dev);
	
	errno_t rc = pio_enable(e1000->reg_base_phys, 8 * PAGE_SIZE,
	    &e1000->reg_base_virt);
	if (rc != EOK)
		return EADDRNOTAVAIL;
	
	return EOK;
}

/** Probe and initialize the newly added device.
 *
 * @param dev E1000 device.
 *
 */
errno_t e1000_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun;
	
	/* Initialize device structure for E1000 */
	errno_t rc = e1000_device_initialize(dev);
	if (rc != EOK)
		return rc;
	
	/* Device initialization */
	nic_t *nic = ddf_dev_data_get(dev);
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	/* Map registers */
	rc = e1000_pio_enable(dev);
	if (rc != EOK)
		goto err_destroy;
	
	e1000_initialize_registers(e1000);
	rc = e1000_initialize_tx_structure(e1000);
	if (rc != EOK)
		goto err_pio;
	
	fibril_mutex_lock(&e1000->rx_lock);
	
	e1000_fill_mac_from_eeprom(e1000);
	e1000_initialize_filters(e1000);
	
	fibril_mutex_unlock(&e1000->rx_lock);
	
	e1000_initialize_vlan(e1000);
	
	fun = ddf_fun_create(nic_get_ddf_dev(nic), fun_exposed, "port0");
	if (fun == NULL)
		goto err_tx_structure;
	nic_set_ddf_fun(nic, fun);
	ddf_fun_set_ops(fun, &e1000_dev_ops);
	
	int irq_cap;
	rc = e1000_register_int_handler(nic, &irq_cap);
	if (rc != EOK) {
		goto err_fun_create;
	}
	
	rc = e1000_initialize_rx_structure(nic);
	if (rc != EOK)
		goto err_irq;
	
	nic_address_t e1000_address;
	e1000_get_address(e1000, &e1000_address);
	rc = nic_report_address(nic, &e1000_address);
	if (rc != EOK)
		goto err_rx_structure;
	
	struct timeval period;
	period.tv_sec = 0;
	period.tv_usec = E1000_DEFAULT_INTERRUPT_INTERVAL_USEC;
	rc = nic_report_poll_mode(nic, NIC_POLL_PERIODIC, &period);
	if (rc != EOK)
		goto err_rx_structure;
	
	rc = ddf_fun_bind(fun);
	if (rc != EOK)
		goto err_fun_bind;
	
	rc = ddf_fun_add_to_category(fun, DEVICE_CATEGORY_NIC);
	if (rc != EOK)
		goto err_add_to_cat;
	
	return EOK;
	
err_add_to_cat:
	ddf_fun_unbind(fun);
err_fun_bind:
err_rx_structure:
	e1000_uninitialize_rx_structure(nic);
err_irq:
	unregister_interrupt_handler(dev, irq_cap);
err_fun_create:
	ddf_fun_destroy(fun);
	nic_set_ddf_fun(nic, NULL);
err_tx_structure:
	e1000_uninitialize_tx_structure(e1000);
err_pio:
	// TODO: e1000_pio_disable(dev);
err_destroy:
	e1000_dev_cleanup(dev);
	return rc;
}

/** Read 16-bit value from EEPROM of E1000 adapter
 *
 * Read using the EERD register.
 *
 * @param device         E1000 device
 * @param eeprom_address 8-bit EEPROM address
 *
 * @return 16-bit value from EEPROM
 *
 */
static uint16_t e1000_eeprom_read(e1000_t *e1000, uint8_t eeprom_address)
{
	fibril_mutex_lock(&e1000->eeprom_lock);
	
	/* Write address and START bit to EERD register */
	uint32_t write_data = e1000->info.eerd_start |
	    (((uint32_t) eeprom_address) <<
	    e1000->info.eerd_address_offset);
	E1000_REG_WRITE(e1000, E1000_EERD, write_data);
	
	uint32_t eerd = E1000_REG_READ(e1000, E1000_EERD);
	while ((eerd & e1000->info.eerd_done) == 0) {
		async_usleep(1);
		eerd = E1000_REG_READ(e1000, E1000_EERD);
	}
	
	fibril_mutex_unlock(&e1000->eeprom_lock);
	
	return (uint16_t) (eerd >> e1000->info.eerd_data_offset);
}

/** Get MAC address of the E1000 adapter
 *
 * @param device  E1000 device
 * @param address Place to store the address
 * @param max_len Maximal addresss length to store
 *
 * @return EOK if succeed
 * @return An error code otherwise
 *
 */
static errno_t e1000_get_address(e1000_t *e1000, nic_address_t *address)
{
	fibril_mutex_lock(&e1000->rx_lock);
	
	uint8_t *mac0_dest = (uint8_t *) address->address;
	uint8_t *mac1_dest = (uint8_t *) address->address + 1;
	uint8_t *mac2_dest = (uint8_t *) address->address + 2;
	uint8_t *mac3_dest = (uint8_t *) address->address + 3;
	uint8_t *mac4_dest = (uint8_t *) address->address + 4;
	uint8_t *mac5_dest = (uint8_t *) address->address + 5;
	
	uint32_t rah = E1000_REG_READ(e1000, E1000_RAH_ARRAY(0));
	uint32_t ral = E1000_REG_READ(e1000, E1000_RAL_ARRAY(0));
	
	*mac0_dest = (uint8_t) ral;
	*mac1_dest = (uint8_t) (ral >> 8);
	*mac2_dest = (uint8_t) (ral >> 16);
	*mac3_dest = (uint8_t) (ral >> 24);
	*mac4_dest = (uint8_t) rah;
	*mac5_dest = (uint8_t) (rah >> 8);
	
	fibril_mutex_unlock(&e1000->rx_lock);
	return EOK;
};

/** Set card MAC address
 *
 * @param device  E1000 device
 * @param address Address
 *
 * @return EOK if succeed
 * @return An error code otherwise
 */
static errno_t e1000_set_addr(ddf_fun_t *fun, const nic_address_t *addr)
{
	nic_t *nic = NIC_DATA_FUN(fun);
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	
	fibril_mutex_lock(&e1000->rx_lock);
	fibril_mutex_lock(&e1000->tx_lock);
	
	errno_t rc = nic_report_address(nic, addr);
	if (rc == EOK)
		e1000_write_receive_address(e1000, 0, addr, false);
	
	fibril_mutex_unlock(&e1000->tx_lock);
	fibril_mutex_unlock(&e1000->rx_lock);
	
	return rc;
}

static void e1000_eeprom_get_address(e1000_t *e1000,
    nic_address_t *address)
{
	uint16_t *mac0_dest = (uint16_t *) address->address;
	uint16_t *mac2_dest = (uint16_t *) (address->address + 2);
	uint16_t *mac4_dest = (uint16_t *) (address->address + 4);
	
	*mac0_dest = e1000_eeprom_read(e1000, 0);
	*mac2_dest = e1000_eeprom_read(e1000, 1);
	*mac4_dest = e1000_eeprom_read(e1000, 2);
}

/** Send frame
 *
 * @param nic    NIC driver data structure
 * @param data   Frame data
 * @param size   Frame size in bytes
 *
 * @return EOK if succeed
 * @return Error code in the case of error
 *
 */
static void e1000_send_frame(nic_t *nic, void *data, size_t size)
{
	assert(nic);
	
	e1000_t *e1000 = DRIVER_DATA_NIC(nic);
	fibril_mutex_lock(&e1000->tx_lock);
	
	uint32_t tdt = E1000_REG_READ(e1000, E1000_TDT);
	e1000_tx_descriptor_t *tx_descriptor_addr = (e1000_tx_descriptor_t *)
	    (e1000->tx_ring_virt + tdt * sizeof(e1000_tx_descriptor_t));
	
	bool descriptor_available = false;
	
	/* Descriptor never used */
	if (tx_descriptor_addr->length == 0)
		descriptor_available = true;
	
	/* Descriptor done */
	if (tx_descriptor_addr->status & TXDESCRIPTOR_STATUS_DD)
		descriptor_available = true;
	
	if (!descriptor_available) {
		/* Frame lost */
		fibril_mutex_unlock(&e1000->tx_lock);
		return;
	}
	
	memcpy(e1000->tx_frame_virt[tdt], data, size);
	
	tx_descriptor_addr->phys_addr = PTR_TO_U64(e1000->tx_frame_phys[tdt]);
	tx_descriptor_addr->length = size;
	
	/*
	 * Report status to STATUS.DD (descriptor done),
	 * add ethernet CRC, end of packet.
	 */
	tx_descriptor_addr->command = TXDESCRIPTOR_COMMAND_RS |
	    TXDESCRIPTOR_COMMAND_IFCS |
	    TXDESCRIPTOR_COMMAND_EOP;
	
	tx_descriptor_addr->checksum_offset = 0;
	tx_descriptor_addr->status = 0;
	if (e1000->vlan_tag_add) {
		tx_descriptor_addr->special = e1000->vlan_tag;
		tx_descriptor_addr->command |= TXDESCRIPTOR_COMMAND_VLE;
	} else
		tx_descriptor_addr->special = 0;
	
	tx_descriptor_addr->checksum_start_field = 0;
	
	tdt++;
	if (tdt == E1000_TX_FRAME_COUNT)
		tdt = 0;
	
	E1000_REG_WRITE(e1000, E1000_TDT, tdt);
	
	fibril_mutex_unlock(&e1000->tx_lock);
}

int main(void)
{
	printf("%s: HelenOS E1000 network adapter driver\n", NAME);
	
	if (nic_driver_init(NAME) != EOK)
		return 1;
	
	nic_driver_implement(&e1000_driver_ops, &e1000_dev_ops,
	    &e1000_nic_iface);
	
	ddf_log_init(NAME);
	return ddf_driver_main(&e1000_driver);
}
