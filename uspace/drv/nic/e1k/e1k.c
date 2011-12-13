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

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <adt/list.h>
#include <align.h>
#include <byteorder.h>
#include <sysinfo.h>
#include <ipc/irc.h>
#include <ipc/ns.h>
#include <libarch/ddi.h>
#include <as.h>
#include <ddf/interrupt.h>
#include <devman.h>
#include <device/hw_res_parsed.h>
#include <device/pci.h>
#include <nic.h>
#include <nil_remote.h>
#include <ops/nic.h>
#include <packet_client.h>
#include <packet_remote.h>
#include <net/packet_header.h>
#include "e1k.h"

#define NAME  "e1k"

#define E1000_DEFAULT_INTERRUPT_INTEVAL_USEC  250

/* Must be power of 8 */
#define E1000_RX_PACKETS_COUNT  128
#define E1000_TX_PACKETS_COUNT  128

#define E1000_RECEIVE_ADDRESS  16

/** Maximum receiving packet size */
#define E1000_MAX_RECEIVE_PACKET_SIZE  2048

/** nic_driver_data_t* -> e1000_t* cast */
#define DRIVER_DATA_NIC(nic_data) \
	((e1000_t *) nic_get_specific(nic_data))

/** device_t* -> nic_driver_data_t* cast */
#define NIC_DATA_DEV(dev) \
	((nic_t *) ((dev)->driver_data))

/** device_t* -> e1000_t* cast */
#define DRIVER_DATA_DEV(dev) \
	(DRIVER_DATA_NIC(NIC_DATA_DEV(dev)))

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

#define E1000_REG_BASE(e1000_data) \
	((e1000_data)->virt_reg_base)

#define E1000_REG_ADDR(e1000_data, reg) \
	((uint32_t *) (E1000_REG_BASE(e1000_data) + reg))

#define E1000_REG_READ(e1000_data, reg) \
	(pio_read_32(E1000_REG_ADDR(e1000_data, reg)))

#define E1000_REG_WRITE(e1000_data, reg, value) \
	(pio_write_32(E1000_REG_ADDR(e1000_data, reg), value))

/** E1000 device data */
typedef struct e1000_data {
	/** Physical registers base address */
	void *phys_reg_base;
	/** Virtual registers base address */
	void *virt_reg_base;
	/** Tx ring */
	dma_mem_t tx_ring;
	/** Packets in tx ring  */
	packet_t **tx_ring_packets;
	/** Rx ring */
	dma_mem_t rx_ring;
	/** Packets in rx ring  */
	packet_t **rx_ring_packets;
	/** VLAN tag */
	uint16_t vlan_tag;
	/** Add VLAN tag to packet */
	bool vlan_tag_add;
	/** Used unicast Receive Address count */
	unsigned int unicast_ra_count;
	/** Used milticast Receive addrress count */
	unsigned int multicast_ra_count;
	/** PCI device ID */
	uint16_t device_id;
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

static int e1000_get_address(e1000_t *, nic_address_t *);
static void e1000_eeprom_get_address(e1000_t *, nic_address_t *);
static int e1000_set_addr(ddf_fun_t *, const nic_address_t *);

static int e1000_defective_get_mode(ddf_fun_t *, uint32_t *);
static int e1000_defective_set_mode(ddf_fun_t *, uint32_t);

static int e1000_get_cable_state(ddf_fun_t *, nic_cable_state_t *);
static int e1000_get_device_info(ddf_fun_t *, nic_device_info_t *);
static int e1000_get_operation_mode(ddf_fun_t *, int *,
    nic_channel_mode_t *, nic_role_t *);
static int e1000_set_operation_mode(ddf_fun_t *, int,
    nic_channel_mode_t, nic_role_t);
static int e1000_autoneg_enable(ddf_fun_t *, uint32_t);
static int e1000_autoneg_disable(ddf_fun_t *);
static int e1000_autoneg_restart(ddf_fun_t *);

static int e1000_vlan_set_tag(ddf_fun_t *, uint16_t, bool, bool);

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

static int e1000_add_device(ddf_dev_t *);

/** Basic driver operations for E1000 driver */
static driver_ops_t e1000_driver_ops = {
	.add_device = e1000_add_device
};

/** Driver structure for E1000 driver */
static driver_t e1000_driver = {
	.name = NAME,
	.driver_ops = &e1000_driver_ops
};

/* The default implementation callbacks */
static int e1000_on_activating(nic_t *);
static int e1000_on_stopping(nic_t *);
static void e1000_write_packet(nic_t *, packet_t *);

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
		.value = 0xFFFFFFFF
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** Interrupt code definition */
irq_code_t e1000_irq_code = {
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
static int e1000_get_device_info(ddf_fun_t *dev, nic_device_info_t *info)
{
	assert(dev);
	assert(info);
	
	bzero(info, sizeof(nic_device_info_t));
	
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
static int e1000_get_cable_state(ddf_fun_t *dev, nic_cable_state_t *state)
{
	assert(dev);
	assert(DRIVER_DATA_DEV(dev));
	assert(state);
	
	e1000_t *e1000_data = DRIVER_DATA_DEV(dev);
	if (E1000_REG_READ(e1000_data, E1000_STATUS) & (STATUS_LU))
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
static int e1000_get_operation_mode(ddf_fun_t *dev, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	e1000_t *e1000_data = DRIVER_DATA_DEV(dev);
	uint32_t status = E1000_REG_READ(e1000_data, E1000_STATUS);
	
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

static void e1000_link_restart(e1000_t * e1000_data)
{
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000_data, E1000_CTRL);
	
	if (ctrl & CTRL_SLU) {
		ctrl &= ~(CTRL_SLU);
		fibril_mutex_unlock(&e1000_data->ctrl_lock);
		usleep(10);
		fibril_mutex_lock(&e1000_data->ctrl_lock);
		ctrl |= CTRL_SLU;
	}
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	
	e1000_link_restart(e1000_data);
}

/** Set operation mode of the device
 *
 */
static int e1000_set_operation_mode(ddf_fun_t *dev, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	if ((speed != 10) && (speed != 100) && (speed != 1000))
		return EINVAL;
	
	if ((duplex != NIC_CM_HALF_DUPLEX) && (duplex != NIC_CM_FULL_DUPLEX))
		return EINVAL;
	
	e1000_t *e1000_data = DRIVER_DATA_DEV(dev);
	
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	uint32_t ctrl = E1000_REG_READ(e1000_data, E1000_CTRL);
	
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
	
	E1000_REG_WRITE(e1000_data, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	
	e1000_link_restart(e1000_data);
	
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
static int e1000_autoneg_enable(ddf_fun_t *dev, uint32_t advertisement)
{
	e1000_t *e1000_data = DRIVER_DATA_DEV(dev);
	
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000_data, E1000_CTRL);
	
	ctrl &= ~(CTRL_FRCSPD);
	ctrl &= ~(CTRL_FRCDPLX);
	ctrl |= CTRL_ASDE;
	
	E1000_REG_WRITE(e1000_data, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	
	e1000_link_restart(e1000_data);
	
	return EOK;
}

/** Disable auto-negotiation
 *
 * @param dev Device to update
 *
 * @return EOK
 *
 */
static int e1000_autoneg_disable(ddf_fun_t *dev)
{
	e1000_t *e1000_data = DRIVER_DATA_DEV(dev);
	
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000_data, E1000_CTRL);
	
	ctrl |= CTRL_FRCSPD;
	ctrl |= CTRL_FRCDPLX;
	ctrl &= ~(CTRL_ASDE);
	
	E1000_REG_WRITE(e1000_data, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	
	e1000_link_restart(e1000_data);
	
	return EOK;
}

/** Restart auto-negotiation
 *
 * @param dev Device to update
 *
 * @return EOK if advertisement mode set successfully
 *
 */
static int e1000_autoneg_restart(ddf_fun_t *dev)
{
	return e1000_autoneg_enable(dev, 0);
}

/** Get state of acceptance of weird packets
 *
 * @param      device Device to check
 * @param[out] mode   Current mode
 *
 */
static int e1000_defective_get_mode(ddf_fun_t *device, uint32_t *mode)
{
	e1000_t *e1000_data = DRIVER_DATA_DEV(device);
	
	*mode = 0;
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	if (rctl & RCTL_SBP)
		*mode = NIC_DEFECTIVE_BAD_CRC | NIC_DEFECTIVE_SHORT;
	
	return EOK;
};

/** Set acceptance of weird packets
 *
 * @param device Device to update
 * @param mode   Mode to set
 *
 * @return ENOTSUP if the mode is not supported
 * @return EOK of mode was set
 *
 */
static int e1000_defective_set_mode(ddf_fun_t *device, uint32_t mode)
{
	e1000_t *e1000_data = DRIVER_DATA_DEV(device);
	int rc = EOK;
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	bool short_mode = (mode & NIC_DEFECTIVE_SHORT ? true : false);
	bool bad_mode = (mode & NIC_DEFECTIVE_BAD_CRC ? true : false);
	
	if (short_mode && bad_mode)
		rctl |= RCTL_SBP;
	else if ((!short_mode) && (!bad_mode))
		rctl &= ~RCTL_SBP;
	else
		rc = ENOTSUP;
	
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
	return rc;
};

/** Write receive address to RA registr
 *
 * @param e1000_data E1000 data structure
 * @param position   RA register position
 * @param address    Ethernet address
 * @param set_av_bit Set the Addtess Valid bit
 *
 */
static void e1000_write_receive_address(e1000_t *e1000_data,
    unsigned int position, const nic_address_t * address,
    bool set_av_bit)
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
		rah |= E1000_REG_READ(e1000_data, E1000_RAH_ARRAY(position)) &
		    RAH_AV;
	
	E1000_REG_WRITE(e1000_data, E1000_RAH_ARRAY(position), rah);
	E1000_REG_WRITE(e1000_data, E1000_RAL_ARRAY(position), ral);
}

/** Disable receive address in RA registr
 *
 *  Clear Address Valid bit
 *
 * @param e1000_data E1000 data structure
 * @param position   RA register position
 *
 */
static void e1000_disable_receive_address(e1000_t *e1000_data,
    unsigned int position)
{
	uint32_t rah =
	    E1000_REG_READ(e1000_data, E1000_RAH_ARRAY(position));
	rah = rah & ~RAH_AV;
	E1000_REG_WRITE(e1000_data, E1000_RAH_ARRAY(position), rah);
}

/** Clear all unicast addresses from RA registers
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_clear_unicast_receive_addresses(e1000_t *e1000_data)
{
	for (unsigned int ra_num = 1;
	    ra_num <= e1000_data->unicast_ra_count;
	    ra_num++)
		e1000_disable_receive_address(e1000_data, ra_num);
	
	e1000_data->unicast_ra_count = 0;
}

/** Clear all multicast addresses from RA registers
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_clear_multicast_receive_addresses(e1000_t *e1000_data)
{
	unsigned int first_multicast_ra_num =
	    E1000_RECEIVE_ADDRESS - e1000_data->multicast_ra_count;
	
	for (unsigned int ra_num = E1000_RECEIVE_ADDRESS - 1;
	    ra_num >= first_multicast_ra_num;
	    ra_num--)
		e1000_disable_receive_address(e1000_data, ra_num);
	
	e1000_data->multicast_ra_count = 0;
}

/** Return receive address filter positions count usable for unicast
 *
 * @param e1000_data E1000 data structure
 *
 * @return receive address filter positions count usable for unicast
 *
 */
static unsigned int get_free_unicast_address_count(e1000_t *e1000_data)
{
	return E1000_RECEIVE_ADDRESS - 1 - e1000_data->multicast_ra_count;
}

/** Return receive address filter positions count usable for multicast
 *
 * @param e1000_data E1000 data structure
 *
 * @return receive address filter positions count usable for multicast
 *
 */
static unsigned int get_free_multicast_address_count(e1000_t *e1000_data)
{
	return E1000_RECEIVE_ADDRESS - 1 - e1000_data->unicast_ra_count;
}

/** Write unicast receive addresses to receive address filter registers
 *
 * @param e1000_data E1000 data structure
 * @param addr       Pointer to address array
 * @param addr_cnt   Address array count
 *
 */
static void e1000_add_unicast_receive_addresses(e1000_t *e1000_data,
    const nic_address_t *addr, size_t addr_cnt)
{
	assert(addr_cnt <= get_free_unicast_address_count(e1000_data));
	
	nic_address_t *addr_iterator = (nic_address_t *) addr;
	
	/* ra_num = 0 is primary address */
	for (unsigned int ra_num = 1;
	    ra_num <= addr_cnt;
	    ra_num++) {
		e1000_write_receive_address(e1000_data, ra_num, addr_iterator, true);
		addr_iterator++;
	}
}

/** Write multicast receive addresses to receive address filter registers
 *
 * @param e1000_data E1000 data structure
 * @param addr       Pointer to address array
 * @param addr_cnt   Address array count
 *
 */
static void e1000_add_multicast_receive_addresses(e1000_t *e1000_data,
    const nic_address_t *addr, size_t addr_cnt)
{
	assert(addr_cnt <= get_free_multicast_address_count(e1000_data));
	
	nic_address_t *addr_iterator = (nic_address_t *) addr;
	
	unsigned int first_multicast_ra_num = E1000_RECEIVE_ADDRESS - addr_cnt;
	for (unsigned int ra_num = E1000_RECEIVE_ADDRESS - 1;
	    ra_num >= first_multicast_ra_num;
	    ra_num-- {
		e1000_write_receive_address(e1000_data, ra_num, addr_iterator, true);
		addr_iterator++;
	}
}

/** Disable receiving packets for default address
 *
 * @param e1000_data E1000 data structure
 *
 */
static void disable_ra0_address_filter(e1000_t *e1000_data)
{
	uint32_t rah0 = E1000_REG_READ(e1000_data, E1000_RAH_ARRAY(0));
	rah0 = rah0 & ~RAH_AV;
	E1000_REG_WRITE(e1000_data, E1000_RAH_ARRAY(0), rah0);
}

/** Enable receiving packets for default address
 *
 * @param e1000_data E1000 data structure
 *
 */
static void enable_ra0_address_filter(e1000_t *e1000_data)
{
	uint32_t rah0 = E1000_REG_READ(e1000_data, E1000_RAH_ARRAY(0));
	rah0 = rah0 | RAH_AV;
	E1000_REG_WRITE(e1000_data, E1000_RAH_ARRAY(0), rah0);
}

/** Disable unicast promiscuous mode
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_disable_unicast_promisc(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl & ~RCTL_UPE;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Enable unicast promiscuous mode
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_enable_unicast_promisc(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl | RCTL_UPE;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Disable multicast promiscuous mode
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_disable_multicast_promisc(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl & ~RCTL_MPE;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Enable multicast promiscuous mode
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_enable_multicast_promisc(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl | RCTL_MPE;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Enable accepting of broadcast packets
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_enable_broadcast_accept(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl | RCTL_BAM;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Disable accepting of broadcast packets
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_disable_broadcast_accept(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl & ~RCTL_BAM;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Enable VLAN filtering according to VFTA registers
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_enable_vlan_filter(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl | RCTL_VFE;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Disable VLAN filtering
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_disable_vlan_filter(e1000_t *e1000_data)
{
	uint32_t rctl = E1000_REG_READ(e1000_data, E1000_RCTL);
	rctl = rctl & ~RCTL_VFE;
	E1000_REG_WRITE(e1000_data, E1000_RCTL, rctl);
}

/** Set multicast packets acceptance mode
 *
 * @param nic_data NIC device to update
 * @param mode     Mode to set
 * @param addr     Address list (used in mode = NIC_MULTICAST_LIST)
 * @param addr_cnt Length of address list (used in mode = NIC_MULTICAST_LIST)
 *
 * @return EOK
 *
 */
static int e1000_on_multicast_mode_change(nic_t *nic_data,
    nic_multicast_mode_t mode, const nic_address_t *addr, size_t addr_cnt)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	int rc = EOK;
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	switch (mode) {
	case NIC_MULTICAST_BLOCKED:
		e1000_clear_multicast_receive_addresses(e1000_data);
		e1000_disable_multicast_promisc(e1000_data);
		nic_report_hw_filtering(nic_data, -1, 1, -1);
		break;
	case NIC_MULTICAST_LIST:
		e1000_clear_multicast_receive_addresses(e1000_data);
		if (addr_cnt > get_free_multicast_address_count(e1000_data)) {
			/*
			 * Future work: fill MTA table
			 * Not strictly neccessary, it only saves some compares
			 * in the NIC library.
			 */
			e1000_enable_multicast_promisc(e1000_data);
			nic_report_hw_filtering(nic_data, -1, 0, -1);
		} else {
			e1000_disable_multicast_promisc(e1000_data);
			e1000_add_multicast_receive_addresses(e1000_data, addr, addr_cnt);
			nic_report_hw_filtering(nic_data, -1, 1, -1);
		}
		break;
	case NIC_MULTICAST_PROMISC:
		e1000_enable_multicast_promisc(e1000_data);
		e1000_clear_multicast_receive_addresses(e1000_data);
		nic_report_hw_filtering(nic_data, -1, 1, -1);
		break;
	default:
		rc = ENOTSUP;
		break;
	}
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
	return rc;
}

/** Set unicast packets acceptance mode
 *
 * @param nic_data NIC device to update
 * @param mode     Mode to set
 * @param addr     Address list (used in mode = NIC_MULTICAST_LIST)
 * @param addr_cnt Length of address list (used in mode = NIC_MULTICAST_LIST)
 *
 * @return EOK
 *
 */
static int e1000_on_unicast_mode_change(nic_t *nic_data,
    nic_unicast_mode_t mode, const nic_address_t *addr, size_t addr_cnt)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	int rc = EOK;
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	switch (mode) {
	case NIC_UNICAST_BLOCKED:
		disable_ra0_address_filter(e1000_data);
		e1000_clear_unicast_receive_addresses(e1000_data);
		e1000_disable_unicast_promisc(e1000_data);
		nic_report_hw_filtering(nic_data, 1, -1, -1);
		break;
	case NIC_UNICAST_DEFAULT:
		enable_ra0_address_filter(e1000_data);
		e1000_clear_unicast_receive_addresses(e1000_data);
		e1000_disable_unicast_promisc(e1000_data);
		nic_report_hw_filtering(nic_data, 1, -1, -1);
		break;
	case NIC_UNICAST_LIST:
		enable_ra0_address_filter(e1000_data);
		e1000_clear_unicast_receive_addresses(e1000_data);
		if (addr_cnt > get_free_unicast_address_count(e1000_data)) {
			e1000_enable_unicast_promisc(e1000_data);
			nic_report_hw_filtering(nic_data, 0, -1, -1);
		} else {
			e1000_disable_unicast_promisc(e1000_data);
			e1000_add_unicast_receive_addresses(e1000_data, addr, addr_cnt);
			nic_report_hw_filtering(nic_data, 1, -1, -1);
		}
		break;
	case NIC_UNICAST_PROMISC:
		e1000_enable_unicast_promisc(e1000_data);
		enable_ra0_address_filter(e1000_data);
		e1000_clear_unicast_receive_addresses(e1000_data);
		nic_report_hw_filtering(nic_data, 1, -1, -1);
		break;
	default:
		rc = ENOTSUP;
		break;
	}
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
	return rc;
}

/** Set broadcast packets acceptance mode
 *
 * @param nic_data NIC device to update
 * @param mode     Mode to set
 *
 * @return EOK
 *
 */
static int e1000_on_broadcast_mode_change(nic_t *nic_data,
    nic_broadcast_mode_t mode)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	int rc = EOK;
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	switch (mode) {
	case NIC_BROADCAST_BLOCKED:
		e1000_disable_broadcast_accept(e1000_data);
		break;
	case NIC_BROADCAST_ACCEPTED:
		e1000_enable_broadcast_accept(e1000_data);
		break;
	default:
		rc = ENOTSUP;
		break;
	}
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
	return rc;
}

/** Check if receiving is enabled
 *
 * @param e1000_data E1000 data structure
 *
 * @return true if receiving is enabled
 *
 */
static bool e1000_is_rx_enabled(e1000_t *e1000_data)
{
	if (E1000_REG_READ(e1000_data, E1000_RCTL) & (RCTL_EN))
		return true;
	
	return false;
}

/** Enable receiving
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_enable_rx(e1000_t *e1000_data)
{
	/* Set Receive Enable Bit */
	E1000_REG_WRITE(e1000_data, E1000_RCTL,
	    E1000_REG_READ(e1000_data, E1000_RCTL) | (RCTL_EN));
}

/** Disable receiving
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_disable_rx(e1000_t *e1000_data)
{
	/* Clear Receive Enable Bit */
	E1000_REG_WRITE(e1000_data, E1000_RCTL,
	    E1000_REG_READ(e1000_data, E1000_RCTL) & ~(RCTL_EN));
}

/** Set VLAN mask
 *
 * @param nic_data  NIC device to update
 * @param vlan_mask VLAN mask
 *
 */
static void e1000_on_vlan_mask_change(nic_t *nic_data,
   const nic_vlan_mask_t *vlan_mask)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	if (vlan_mask) {
		/*
		 * Disable receiving, so that packet matching
		 * partially written VLAN is not received.
		 */
		bool rx_enabled = e1000_is_rx_enabled(e1000_data);
		if (rx_enabled)
			e1000_disable_rx(e1000_data);
		
		for (unsigned int i = 0; i < NIC_VLAN_BITMAP_SIZE; i += 4) {
			uint32_t bitmap_part =
			    ((uint32_t) vlan_mask->bitmap[i]) |
			    (((uint32_t) vlan_mask->bitmap[i + 1]) << 8) |
			    (((uint32_t) vlan_mask->bitmap[i + 2]) << 16) |
			    (((uint32_t) vlan_mask->bitmap[i + 3]) << 24);
			E1000_REG_WRITE(e1000_data, E1000_VFTA_ARRAY(i / 4), bitmap_part);
		}
		
		e1000_enable_vlan_filter(e1000_data);
		if (rx_enabled)
			e1000_enable_rx(e1000_data);
	} else
		e1000_disable_vlan_filter(e1000_data);
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
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
static int e1000_vlan_set_tag(ddf_fun_t *device, uint16_t tag, bool add,
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
	
	e1000_t *e1000_data = DRIVER_DATA_DEV(device);
	
	e1000_data->vlan_tag = tag;
	e1000_data->vlan_tag_add = add;
	
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	
	uint32_t ctrl = E1000_REG_READ(e1000_data, E1000_CTRL);
	if (strip)
		ctrl |= CTRL_VME;
	else
		ctrl &= ~CTRL_VME;
	
	E1000_REG_WRITE(e1000_data, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	return EOK;
}

/** Fill receive descriptor with new empty packet
 *
 * Store packet in e1000_data->rx_ring_packets
 *
 * @param nic_data NIC data stricture
 * @param offset   Receive descriptor offset
 *
 */
static void e1000_fill_new_rx_descriptor(nic_t *nic_data, unsigned int offset)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	packet_t *packet = nic_alloc_packet(nic_data, E1000_MAX_RECEIVE_PACKET_SIZE);
	
	assert(packet);
	
	*(e1000_data->rx_ring_packets + offset) = packet;
	e1000_rx_descriptor_t * rx_descriptor = (e1000_rx_descriptor_t *)
	    (e1000_data->rx_ring.virtual +
	    offset * sizeof(e1000_rx_descriptor_t));
	
	void *phys_addr = nic_dma_lock_packet(packet);
	
	if (phys_addr) {
		rx_descriptor->phys_addr =
		    PTR_TO_U64(phys_addr + packet->data_start);
	} else
		rx_descriptor->phys_addr = 0;
	
	rx_descriptor->length = 0;
	rx_descriptor->checksum = 0;
	rx_descriptor->status = 0;
	rx_descriptor->errors = 0;
	rx_descriptor->special = 0;
}

/** Clear receive descriptor
 *
 * @param e1000_data E1000 data
 * @param offset     Receive descriptor offset
 *
 */
static void e1000_clear_rx_descriptor(e1000_t *e1000_data, unsigned int offset)
{
	e1000_rx_descriptor_t *rx_descriptor = (e1000_rx_descriptor_t *)
	    (e1000_data->rx_ring.virtual +
	    offset * sizeof(e1000_rx_descriptor_t));
	
	rx_descriptor->length = 0;
	rx_descriptor->checksum = 0;
	rx_descriptor->status = 0;
	rx_descriptor->errors = 0;
	rx_descriptor->special = 0;
}

/** Clear receive descriptor
 *
 * @param nic_data NIC data
 * @param offset   Receive descriptor offset
 *
 */
static void e1000_clear_tx_descriptor(nic_t *nic_data, unsigned int offset)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	e1000_tx_descriptor_t * tx_descriptor = (e1000_tx_descriptor_t *)
	    (e1000_data->tx_ring.virtual + 
	    offset * sizeof(e1000_tx_descriptor_t));
	
	if (tx_descriptor->length) {
		packet_t * old_packet = *(e1000_data->tx_ring_packets + offset);
		if (old_packet)
			nic_release_packet(nic_data, old_packet);
	}
	
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

/** Receive packets
 *
 * @param nic_data NIC data
 *
 */
static void e1000_receive_packets(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	uint32_t *tail_addr = E1000_REG_ADDR(e1000_data, E1000_RDT);
	uint32_t next_tail = e1000_inc_tail(*tail_addr, E1000_RX_PACKETS_COUNT);
	
	e1000_rx_descriptor_t *rx_descriptor = (e1000_rx_descriptor_t *)
	    (e1000_data->rx_ring.virtual +
	    next_tail * sizeof(e1000_rx_descriptor_t));
	
	while (rx_descriptor->status & 0x01) {
		uint32_t packet_size = rx_descriptor->length - E1000_CRC_SIZE;
		
		packet_t *packet = *(e1000_data->rx_ring_packets + next_tail);
		packet_suffix(packet, packet_size);
		
		nic_dma_unlock_packet(packet);
		nic_received_packet(nic_data, packet);
		
		e1000_fill_new_rx_descriptor(nic_data, next_tail);
		
		*tail_addr = e1000_inc_tail(*tail_addr, E1000_RX_PACKETS_COUNT);
		next_tail = e1000_inc_tail(*tail_addr, E1000_RX_PACKETS_COUNT);
		
		rx_descriptor = (e1000_rx_descriptor_t *)
		    (e1000_data->rx_ring.virtual +
		    next_tail * sizeof(e1000_rx_descriptor_t));
	}
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
}

/** Enable E1000 interupts
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_enable_interrupts(e1000_t *e1000_data)
{
	E1000_REG_WRITE(e1000_data, E1000_IMS, ICR_RXT0);
}

/** Disable E1000 interupts
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_disable_interrupts(e1000_t *e1000_data)
{
	E1000_REG_WRITE(e1000_data, E1000_IMS, 0);
}

/** Interrupt handler implementation
 *
 * This function is called from e1000_interrupt_handler()
 * and e1000_poll()
 *
 * @param nic_data NIC data
 * @param icr      ICR register value
 *
 */
static void e1000_interrupt_handler_impl(nic_t *nic_data, uint32_t icr)
{
	if (icr & ICR_RXT0)
		e1000_receive_packets(nic_data);
}

/** Handle device interrupt
 *
 * @param dev   E1000 device
 * @param iid   IPC call id
 * @param icall IPC call structure
 *
 */
static void e1000_interrupt_handler(ddf_dev_t *dev, ipc_callid_t iid,
    ipc_call_t *icall)
{
	uint32_t icr = (uint32_t) IPC_GET_ARG2(*icall);
	nic_t *nic_data = NIC_DATA_DEV(dev);
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	e1000_interrupt_handler_impl(nic_data, icr);
	e1000_enable_interrupts(e1000_data);
}

/** Register interrupt handler for the card in the system
 *
 * Note: The global irq_reg_mutex is locked because of work with global
 * structure.
 *
 * @param nic_data Driver data
 *
 * @return EOK if the handler was registered
 * @return Negative error code otherwise
 *
 */
inline static int e1000_register_int_handler(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	/* Lock the mutex in whole driver while working with global structure */
	fibril_mutex_lock(&irq_reg_mutex);
	
	// FIXME: This is not supported in mainline
	e1000_irq_code.cmds[0].addr =
	    0xffff800000000000 + e1000_data->phys_reg_base + E1000_ICR;
	e1000_irq_code.cmds[2].addr =
	    0xffff800000000000 + e1000_data->phys_reg_base + E1000_IMC;
	
	int rc = register_interrupt_handler(nic_get_ddf_dev(nic_data),
	    e1000_data->irq, e1000_interrupt_handler, &e1000_irq_code);
	
	fibril_mutex_unlock(&irq_reg_mutex);
	return rc;
}

/** Force receiving all packets in the receive buffer
 *
 * @param nic_data NIC data
 *
 */
static void e1000_poll(nic_t *nic_data)
{
	assert(nic_data);
	
	e1000_t *e1000_data = nic_get_specific(nic_data);
	assert(e1000_data);
	
	uint32_t icr = E1000_REG_READ(e1000_data, E1000_ICR);
	e1000_interrupt_handler_impl(nic_data, icr);
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
static int e1000_poll_mode_change(nic_t *nic_data, nic_poll_mode_t mode,
    const struct timeval *period)
{
	assert(nic_data);
	
	e1000_t *e1000_data = nic_get_specific(nic_data);
	assert(e1000_data);
	
	switch (mode) {
	case NIC_POLL_IMMEDIATE:
		E1000_REG_WRITE(e1000_data, E1000_ITR, 0);
		e1000_enable_interrupts(e1000_data);
		break;
	case NIC_POLL_ON_DEMAND:
		e1000_disable_interrupts(e1000_data);
		break;
	case NIC_POLL_PERIODIC:
		assert(period);
		uint16_t itr_interval = e1000_calculate_itr_interval(period);
		E1000_REG_WRITE(e1000_data, E1000_ITR, (uint32_t) itr_interval);
		e1000_enable_interrupts(e1000_data);
		break;
	default:
		return ENOTSUP;
	}
	
	return EOK;
}

/** Initialize receive registers
 *
 * @param e1000_data E1000 data structure
 *
 */
static void e1000_initialize_rx_registers(e1000_t * e1000_data)
{
	E1000_REG_WRITE(e1000_data, E1000_RDLEN, E1000_RX_PACKETS_COUNT * 16);
	E1000_REG_WRITE(e1000_data, E1000_RDH, 0);
	
	/* It is not posible to let HW use all descriptors */
	E1000_REG_WRITE(e1000_data, E1000_RDT, E1000_RX_PACKETS_COUNT - 1);
	
	/* Set Broadcast Enable Bit */
	E1000_REG_WRITE(e1000_data, E1000_RCTL, RCTL_BAM);
}

/** Initialize receive structure
 *
 * @param nic_data NIC data
 *
 * @return EOK if succeed
 * @return Negative error code otherwise
 *
 */
static int e1000_initialize_rx_structure(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	e1000_data->rx_ring.size =
	    ALIGN_UP(E1000_RX_PACKETS_COUNT * sizeof(e1000_rx_descriptor_t),
	    PAGE_SIZE) / PAGE_SIZE;
	e1000_data->rx_ring.mapping_flags = AS_AREA_READ | AS_AREA_WRITE;
	
	int rc = dma_allocate_anonymous(&e1000_data->rx_ring, 0);
	if (rc != EOK)
		return rc;
	
	E1000_REG_WRITE(e1000_data, E1000_RDBAH,
	    (uint32_t) (PTR_TO_U64(e1000_data->rx_ring.physical) >> 32));
	E1000_REG_WRITE(e1000_data, E1000_RDBAL,
	    (uint32_t) PTR_TO_U64(e1000_data->rx_ring.physical));
	
	e1000_data->rx_ring_packets =
	    malloc(E1000_RX_PACKETS_COUNT * sizeof(packet_t *));
	// FIXME: Check return value
	
	/* Write descriptor */
	for (unsigned int offset = 0;
	    offset < E1000_RX_PACKETS_COUNT;
	    offset++)
		e1000_fill_new_rx_descriptor(nic_data, offset);
	
	e1000_initialize_rx_registers(e1000_data);
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
	return EOK;
}

/** Uninitialize receive structure
 *
 * @param nic_data NIC data
 *
 */
static void e1000_uninitialize_rx_structure(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	/* Write descriptor */
	for (unsigned int offset = 0;
	    offset < E1000_RX_PACKETS_COUNT;
	    offset++) {
		packet_t *packet = *(e1000_data->rx_ring_packets + offset);
		nic_dma_unlock_packet(packet);
		nic_release_packet(nic_data, packet);
	}
	
	free(e1000_data->rx_ring_packets);
	dma_free(&e1000_data->rx_ring);
}

/** Clear receive descriptor ring
 *
 * @param e1000_data E1000 data
 *
 */
static void e1000_clear_rx_ring(e1000_t * e1000_data)
{
	/* Write descriptor */
	for (unsigned int offset = 0;
	    offset < E1000_RX_PACKETS_COUNT;
	    offset++)
		e1000_clear_rx_descriptor(e1000_data, offset);
}

/** Initialize filters
 *
 * @param e1000_data E1000 data
 *
 */
static void e1000_initialize_filters(e1000_t *e1000_data)
{
	/* Initialize address filter */
	e1000_data->unicast_ra_count = 0;
	e1000_data->multicast_ra_count = 0;
	e1000_clear_unicast_receive_addresses(e1000_data);
}

/** Initialize VLAN
 *
 * @param e1000_data E1000 data
 *
 */
static void e1000_initialize_vlan(e1000_t *e1000_data)
{
	e1000_data->vlan_tag_add = false;
}

/** Fill MAC address from EEPROM to RA[0] register
 *
 * @param e1000_data E1000 data
 *
 */
static void e1000_fill_mac_from_eeprom(e1000_t *e1000_data)
{
	/* MAC address from eeprom to RA[0] */
	nic_address_t address;
	e1000_eeprom_get_address(e1000_data, &address);
	e1000_write_receive_address(e1000_data, 0, &address, true);
}

/** Initialize other registers
 *
 * @param dev E1000 data.
 *
 * @return EOK if succeed
 * @return Negative error code otherwise
 *
 */
static void e1000_initialize_registers(e1000_t *e1000_data)
{
	E1000_REG_WRITE(e1000_data, E1000_ITR,
	    e1000_calculate_itr_interval_from_usecs(
	    E1000_DEFAULT_INTERRUPT_INTEVAL_USEC));
	E1000_REG_WRITE(e1000_data, E1000_FCAH, 0);
	E1000_REG_WRITE(e1000_data, E1000_FCAL, 0);
	E1000_REG_WRITE(e1000_data, E1000_FCT, 0);
	E1000_REG_WRITE(e1000_data, E1000_FCTTV, 0);
	E1000_REG_WRITE(e1000_data, E1000_VET, VET_VALUE);
	E1000_REG_WRITE(e1000_data, E1000_CTRL, CTRL_ASDE);
}

/** Initialize transmit registers
 *
 * @param e1000_data E1000 data.
 *
 */
static void e1000_initialize_tx_registers(e1000_t *e1000_data)
{
	E1000_REG_WRITE(e1000_data, E1000_TDLEN, E1000_TX_PACKETS_COUNT * 16);
	E1000_REG_WRITE(e1000_data, E1000_TDH, 0);
	E1000_REG_WRITE(e1000_data, E1000_TDT, 0);
	
	E1000_REG_WRITE(e1000_data, E1000_TIPG,
	    10 << TIPG_IPGT_SHIFT |
	    8 << TIPG_IPGR1_SHIFT |
	    6 << TIPG_IPGR2_SHIFT);
	
	E1000_REG_WRITE(e1000_data, E1000_TCTL,
	    0x0F << TCTL_CT_SHIFT /* Collision Threshold */ |
	    0x40 << TCTL_COLD_SHIFT /* Collision Distance */ |
	    TCTL_PSP /* Pad Short Packets */);
}

/** Initialize transmit structure
 *
 * @param e1000_data E1000 data.
 *
 */
static int e1000_initialize_tx_structure(e1000_t *e1000_data)
{
	fibril_mutex_lock(&e1000_data->tx_lock);
	
	e1000_data->tx_ring.size =
	    ALIGN_UP(E1000_TX_PACKETS_COUNT * sizeof(e1000_tx_descriptor_t),
	    PAGE_SIZE) / PAGE_SIZE;
	e1000_data->tx_ring.mapping_flags = AS_AREA_READ | AS_AREA_WRITE;
	
	int rc = dma_allocate_anonymous(&e1000_data->tx_ring, 0);
	if (rc != EOK)
		return rc;
	
	bzero(e1000_data->tx_ring.virtual,
	    E1000_TX_PACKETS_COUNT * sizeof(e1000_tx_descriptor_t));
	
	E1000_REG_WRITE(e1000_data, E1000_TDBAH,
	    (uint32_t) (PTR_TO_U64(e1000_data->tx_ring.physical) >> 32));
	E1000_REG_WRITE(e1000_data, E1000_TDBAL,
	    (uint32_t) PTR_TO_U64(e1000_data->tx_ring.physical));
	
	e1000_data->tx_ring_packets =
	    malloc(E1000_TX_PACKETS_COUNT * sizeof(packet_t *));
	// FIXME: Check return value
	
	e1000_initialize_tx_registers(e1000_data);
	
	fibril_mutex_unlock(&e1000_data->tx_lock);
	return EOK;
}

/** Uninitialize transmit structure
 *
 * @param nic_data NIC data
 *
 */
static void e1000_uninitialize_tx_structure(e1000_t *e1000_data)
{
	free(e1000_data->tx_ring_packets);
	dma_free(&e1000_data->tx_ring);
}

/** Clear transmit descriptor ring
 *
 * @param nic_data NIC data
 *
 */
static void e1000_clear_tx_ring(nic_t *nic_data)
{
	/* Write descriptor */
	for (unsigned int offset = 0;
	    offset < E1000_TX_PACKETS_COUNT;
	    offset++)
		e1000_clear_tx_descriptor(nic_data, offset);
}

/** Enable transmit
 *
 * @param e1000_data E1000 data
 *
 */
static void e1000_enable_tx(e1000_t *e1000_data)
{
	/* Set Transmit Enable Bit */
	E1000_REG_WRITE(e1000_data, E1000_TCTL,
	    E1000_REG_READ(e1000_data, E1000_TCTL) | (TCTL_EN));
}

/** Disable transmit
 *
 * @param e1000_data E1000 data
 *
 */
static void e1000_disable_tx(e1000_t *e1000_data)
{
	/* Clear Transmit Enable Bit */
	E1000_REG_WRITE(e1000_data, E1000_TCTL,
	    E1000_REG_READ(e1000_data, E1000_TCTL) & ~(TCTL_EN));
}

/** Reset E1000 device
 *
 * @param e1000_data The E1000 data
 *
 */
static int e1000_reset(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	E1000_REG_WRITE(e1000_data, E1000_CTRL, CTRL_RST);
	
	/* Wait for the reset */
	usleep(20);
	
	/* check if RST_BIT cleared */
	if (E1000_REG_READ(e1000_data, E1000_CTRL) & (CTRL_RST))
		return EINVAL;
	
	e1000_initialize_registers(e1000_data);
	e1000_initialize_rx_registers(e1000_data);
	e1000_initialize_tx_registers(e1000_data);
	e1000_fill_mac_from_eeprom(e1000_data);
	e1000_initialize_filters(e1000_data);
	e1000_initialize_vlan(e1000_data);
	
	return EOK;
}

/** Activate the device to receive and transmit packets
 *
 * @param nic_data NIC driver data
 *
 * @return EOK if activated successfully
 * @return Error code otherwise
 *
 */
static int e1000_on_activating(nic_t *nic_data)
{
	assert(nic_data);
	
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	fibril_mutex_lock(&e1000_data->tx_lock);
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	
	e1000_enable_interrupts(e1000_data);
	
	nic_enable_interrupt(nic_data, e1000_data->irq);
	
	e1000_clear_rx_ring(e1000_data);
	e1000_enable_rx(e1000_data);
	
	e1000_clear_tx_ring(nic_data);
	e1000_enable_tx(e1000_data);
	
	uint32_t ctrl = E1000_REG_READ(e1000_data, E1000_CTRL);
	ctrl |= CTRL_SLU;
	E1000_REG_WRITE(e1000_data, E1000_CTRL, ctrl);
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	fibril_mutex_unlock(&e1000_data->tx_lock);
	fibril_mutex_unlock(&e1000_data->rx_lock);
	
	return EOK;
}

/** Callback for NIC_STATE_DOWN change
 *
 * @param nic_data NIC driver data
 *
 * @return EOK if succeed
 * @return Error code otherwise
 *
 */
static int e1000_on_down_unlocked(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	uint32_t ctrl = E1000_REG_READ(e1000_data, E1000_CTRL);
	ctrl &= ~CTRL_SLU;
	E1000_REG_WRITE(e1000_data, E1000_CTRL, ctrl);
	
	e1000_disable_tx(e1000_data);
	e1000_disable_rx(e1000_data);
	
	nic_disable_interrupt(nic_data, e1000_data->irq);
	e1000_disable_interrupts(e1000_data);
	
	/*
	 * Wait for the for the end of all data
	 * transfers to descriptors.
	 */
	usleep(100);
	
	return EOK;
}

/** Callback for NIC_STATE_DOWN change
 *
 * @param nic_data NIC driver data
 *
 * @return EOK if succeed
 * @return Error code otherwise
 *
 */
static int e1000_on_down(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	fibril_mutex_lock(&e1000_data->tx_lock);
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	
	int rc = e1000_on_down_unlocked(nic_data);
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	fibril_mutex_unlock(&e1000_data->tx_lock);
	fibril_mutex_unlock(&e1000_data->rx_lock);
	
	return rc;
}

/** Callback for NIC_STATE_STOPPED change
 *
 * @param nic_data NIC driver data
 *
 * @return EOK if succeed
 * @return Error code otherwise
 *
 */
static int e1000_on_stopping(nic_t *nic_data)
{
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	fibril_mutex_lock(&e1000_data->tx_lock);
	fibril_mutex_lock(&e1000_data->ctrl_lock);
	
	int rc = e1000_on_down_unlocked(nic_data);
	if (rc == EOK)
		rc = e1000_reset(nic_data);
	
	fibril_mutex_unlock(&e1000_data->ctrl_lock);
	fibril_mutex_unlock(&e1000_data->tx_lock);
	fibril_mutex_unlock(&e1000_data->rx_lock);
	
	return rc;
}

/** Create driver data structure
 *
 * @return Intialized device data structure or NULL
 *
 */
static e1000_t *e1000_create_dev_data(ddf_dev_t *dev)
{
	assert(dev);
	assert(!dev->driver_data);
	
	nic_t *nic_data = nic_create_and_bind(dev);
	if (!nic_data)
		return NULL;
	
	e1000_t *e1000_data = malloc(sizeof(e1000_t));
	if (!e1000_data) {
		nic_unbind_and_destroy(dev);
		return NULL;
	}
	
	bzero(e1000_data, sizeof(e1000_t));
	
	nic_set_specific(nic_data, e1000_data);
	nic_set_write_packet_handler(nic_data, e1000_write_packet);
	nic_set_state_change_handlers(nic_data, e1000_on_activating,
	    e1000_on_down, e1000_on_stopping);
	nic_set_filtering_change_handlers(nic_data,
	    e1000_on_unicast_mode_change, e1000_on_multicast_mode_change,
	    e1000_on_broadcast_mode_change, NULL, e1000_on_vlan_mask_change);
	nic_set_poll_handlers(nic_data, e1000_poll_mode_change, e1000_poll);
	
	fibril_mutex_initialize(&e1000_data->ctrl_lock);
	fibril_mutex_initialize(&e1000_data->rx_lock);
	fibril_mutex_initialize(&e1000_data->tx_lock);
	fibril_mutex_initialize(&e1000_data->eeprom_lock);
	
	return e1000_data;
}

/** Delete driver data structure
 *
 * @param data E1000 device data structure
 *
 */
inline static void e1000_delete_dev_data(ddf_dev_t *dev)
{
	assert(dev);
	
	if (dev->driver_data != NULL)
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
	
	if (dev->parent_sess != NULL) {
		async_hangup(dev->parent_sess);
		dev->parent_sess = NULL;
	}
}

/** Fill the irq and io_addr part of device data structure
 *
 * The hw_resources must be obtained before calling this function
 *
 * @param dev          Device structure
 * @param hw_resources Hardware resources obtained from the parent device
 *
 * @return EOK if succeed
 * @return Negative error code otherwise
 *
 */
static int e1000_fill_resource_info(ddf_dev_t *dev,
    const hw_res_list_parsed_t *hw_resources)
{
	assert(dev != NULL);
	assert(hw_resources != NULL);
	assert(dev->driver_data != NULL);
	
	e1000_t *e1000_data = DRIVER_DATA_DEV(dev);
	
	if (hw_resources->irqs.count != 1)
		return EINVAL;
	
	e1000_data->irq = hw_resources->irqs.irqs[0];
	e1000_data->phys_reg_base =
	    MEMADDR_TO_PTR(hw_resources->mem_ranges.ranges[0].address);
	
	return EOK;
}

/** Obtain information about hardware resources of the device
 *
 * The device must be connected to the parent
 *
 * @param dev Device structure
 *
 * @return EOK if succeed
 * @return Negative error code otherwise
 *
 */
static int e1000_get_resource_info(ddf_dev_t *dev)
{
	assert(dev != NULL);
	assert(NIC_DATA_DEV(dev) != NULL);
	
	hw_res_list_parsed_t hw_res_parsed;
	hw_res_list_parsed_init(&hw_res_parsed);
	
	/* Get hw resources form parent driver */
	int rc = nic_get_resources(NIC_DATA_DEV(dev), &hw_res_parsed);
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
 * @return Negative error code otherwise
 *
 */
static int e1000_device_initialize(ddf_dev_t *dev)
{
	/* Allocate driver data for the device. */
	e1000_t *e1000_data = e1000_create_dev_data(dev);
	if (e1000_data == NULL)
		return ENOMEM;
	
	/* Obtain and fill hardware resources info */
	rc = e1000_get_resource_info(dev);
	if (rc != EOK) {
		e1000_dev_cleanup(dev);
		return rc;
	}
	
	rc = pci_config_space_read_16(dev->parent_sess, PCI_DEVICE_ID,
	    &e1000_data->device_id);
	if (rc != EOK) {
		e1000_dev_cleanup(dev);
		return rc;
	}
	
	return EOK;
}

/** Enable the I/O ports of the device.
 *
 * @param dev E1000 device.
 *
 * @return EOK if successed
 * @return Negative error code otherwise
 *
 */
static int e1000_pio_enable(ddf_dev_t *dev)
{
	e1000_t *e1000_data = DRIVER_DATA_DEV(dev);
	
	int rc = pio_enable(e1000_data->phys_reg_base, 8 * PAGE_SIZE,
	    &e1000_data->virt_reg_base);
	if (rc != EOK)
		return EADDRNOTAVAIL;
	
	return EOK;
}

/** The add_device callback of E1000 callback
 *
 * Probe and initialize the newly added device.
 *
 * @param dev E1000 device.
 *
 */
int e1000_add_device(ddf_dev_t *dev)
{
	assert(dev);
	
	/* Initialize device structure for E1000 */
	int rc = e1000_device_initialize(dev);
	if (rc != EOK)
		return rc;
	
	/* Device initialization */
	nic_t *nic_data = dev->driver_data;
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	/* Map registers */
	rc = e1000_pio_enable(dev);
	if (rc != EOK)
		goto err_destroy;
	
	e1000_initialize_registers(e1000_data);
	rc = e1000_initialize_tx_structure(e1000_data);
	if (rc != EOK)
		goto err_pio;
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	e1000_fill_mac_from_eeprom(e1000_data);
	e1000_initialize_filters(e1000_data);
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
	
	e1000_initialize_vlan(e1000_data);
	
	rc = nic_register_as_ddf_fun(nic_data, &e1000_dev_ops);
	if (rc != EOK)
		goto err_tx_structure;
	
	rc = e1000_register_int_handler(nic_data);
	if (rc != EOK)
		goto err_tx_structure;
	
	rc = nic_connect_to_services(nic_data);
	if (rc != EOK)
		goto err_irq;
	
	rc = e1000_initialize_rx_structure(nic_data);
	if (rc != EOK)
		goto err_irq;
	
	nic_address_t e1000_address;
	e1000_get_address(e1000_data, &e1000_address);
	rc = nic_report_address(nic_data, &e1000_address);
	if (rc != EOK)
		goto err_rx_structure;
	
	struct timeval period;
	period.tv_sec = 0;
	period.tv_usec = E1000_DEFAULT_INTERRUPT_INTERVAL_USEC;
	rc = nic_report_poll_mode(nic_data, NIC_POLL_PERIODIC, &period);
	if (rc != EOK)
		goto err_rx_structure;
	
	return EOK;
	
err_rx_structure:
	e1000_uninitialize_rx_structure(nic_data);
err_irq:
	unregister_interrupt_handler(dev, DRIVER_DATA_DEV(dev)->irq);
err_tx_structure:
	e1000_uninitialize_tx_structure(e1000_data);
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
static uint16_t e1000_eeprom_read(e1000_t *e1000_data, uint8_t eeprom_address)
{
	fibril_mutex_lock(&e1000_data->eeprom_lock);
	
	uint32_t eerd_done;
	uint32_t eerd_address_offset;
	
	switch (e1000_data->device_id) {
	case 0x107c:
	case 0x1013:
	case 0x1018:
	case 0x1019:
	case 0x101A:
	case 0x1076:
	case 0x1077:
	case 0x1078:
	case 0x10b9:
		/* 82541xx and 82547GI/EI */
		eerd_done = EERD_DONE_82541XX_82547GI_EI;
		eerd_address_offset = EERD_ADDRESS_OFFSET_82541XX_82547GI_EI;
		break;
	default:
		eerd_done = EERD_DONE;
		eerd_address_offset = EERD_ADDRESS_OFFSET;
		break;
	}
	
	/* Write address and START bit to EERD register */
	uint32_t write_data = EERD_START |
	    (((uint32_t) eeprom_address) << eerd_address_offset);
	E1000_REG_WRITE(e1000_data, E1000_EERD, write_data);
	
	uint32_t eerd = E1000_REG_READ(e1000_data, E1000_EERD);
	while ((eerd & eerd_done) == 0) {
		usleep(1);
		eerd = E1000_REG_READ(e1000_data, E1000_EERD);
	}
	
	fibril_mutex_unlock(&e1000_data->eeprom_lock);
	
	return (uint16_t) (eerd >> EERD_DATA_OFFSET);
}

/** Get MAC address of the E1000 adapter
 *
 * @param device  E1000 device
 * @param address Place to store the address
 * @param max_len Maximal addresss length to store
 *
 * @return EOK if succeed
 * @return Negative error code otherwise
 *
 */
static int e1000_get_address(e1000_t *e1000_data, nic_address_t *address)
{
	fibril_mutex_lock(&e1000_data->rx_lock);
	
	uint8_t *mac0_dest = (uint8_t *) address->address;
	uint8_t *mac1_dest = (uint8_t *) address->address + 1;
	uint8_t *mac2_dest = (uint8_t *) address->address + 2;
	uint8_t *mac3_dest = (uint8_t *) address->address + 3;
	uint8_t *mac4_dest = (uint8_t *) address->address + 4;
	uint8_t *mac5_dest = (uint8_t *) address->address + 5;
	
	uint32_t rah = E1000_REG_READ(e1000_data, E1000_RAH_ARRAY(0));
	uint32_t ral = E1000_REG_READ(e1000_data, E1000_RAL_ARRAY(0));
	
	*mac0_dest = (uint8_t) ral;
	*mac1_dest = (uint8_t) (ral >> 8);
	*mac2_dest = (uint8_t) (ral >> 16);
	*mac3_dest = (uint8_t) (ral >> 24);
	*mac4_dest = (uint8_t) rah;
	*mac5_dest = (uint8_t) (rah >> 8);
	
	fibril_mutex_unlock(&e1000_data->rx_lock);
	return EOK;
};

/** Set card MAC address
 *
 * @param device  E1000 device
 * @param address Address
 *
 * @return EOK if succeed
 * @return Negative error code otherwise
 */
static int e1000_set_addr(ddf_fun_t *dev, const nic_address_t *addr)
{
	nic_t *nic_data = NIC_DATA_DEV(dev);
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	
	fibril_mutex_lock(&e1000_data->rx_lock);
	fibril_mutex_lock(&e1000_data->tx_lock);
	
	int rc = nic_report_address(nic_data, addr);
	if (rc == EOK)
		e1000_write_receive_address(e1000_data, 0, addr, false)
	
	fibril_mutex_unlock(&e1000_data->tx_lock);
	fibril_mutex_unlock(&e1000_data->rx_lock);
	
	return rc;
}

static void e1000_eeprom_get_address(e1000_t *e1000_data,
    nic_address_t *address)
{
	uint16_t *mac0_dest = (uint16_t *) address->address;
	uint16_t *mac2_dest = (uint16_t *) (address->address + 2);
	uint16_t *mac4_dest = (uint16_t *) (address->address + 4);
	
	*mac0_dest = e1000_eeprom_read(e1000_data, 0);
	*mac2_dest = e1000_eeprom_read(e1000_data, 1);
	*mac4_dest = e1000_eeprom_read(e1000_data, 2);
}

/** Send packet
 *
 * @param nic_data NIC driver data structure
 * @param packet   Packet to send
 *
 * @return EOK if succeed
 * @return Error code in the case of error
 *
 */
static void e1000_write_packet(nic_t *nic_data, packet_t *packet)
{
	assert(nic_data);
	
	e1000_t *e1000_data = DRIVER_DATA_NIC(nic_data);
	fibril_mutex_lock(&e1000_data->tx_lock);
	
	uint32_t tdt = E1000_REG_READ(e1000_data, E1000_TDT);
	e1000_tx_descriptor_t *tx_descriptor_addr = (e1000_tx_descriptor_t *)
	    (e1000_data->tx_ring.virtual + tdt * sizeof(e1000_tx_descriptor_t));
	
	bool descriptor_available = false;
	
	/* Descriptor never used */
	if (tx_descriptor_addr->length == 0)
		descriptor_available = true;
	
	/* Descriptor done */
	if (tx_descriptor_addr->status & TXDESCRIPTOR_STATUS_DD) {
		descriptor_available = true;
		packet_t *old_packet = *(e1000_data->tx_ring_packets + tdt);
		if (old_packet) {
			nic_dma_unlock_packet(old_packet);
			nic_release_packet(nic_data, old_packet);
		}
	}
	
	if (!descriptor_available) {
		/* Packet lost */
		fibril_mutex_unlock(&e1000_data->tx_lock);
		return;
	}
	
	size_t packet_size = packet_get_data_length(packet);
	
	void *phys_addr = nic_dma_lock_packet(packet);
	if (!phys_addr) {
		fibril_mutex_unlock(&e1000_data->tx_lock);
		return;
	}
	
	*(e1000_data->tx_ring_packets + tdt) = packet;
	
	tx_descriptor_addr->phys_addr =
	    PTR_TO_U64(phys_addr + packet->data_start);
	tx_descriptor_addr->length = packet_size;
	
	/*
	 * Report status to STATUS.DD (descriptor done),
	 * add ethernet CRC, end of packet.
	 */
	tx_descriptor_addr->command = TXDESCRIPTOR_COMMAND_RS |
	    TXDESCRIPTOR_COMMAND_IFCS |
	    TXDESCRIPTOR_COMMAND_EOP;
	
	tx_descriptor_addr->checksum_offset = 0;
	tx_descriptor_addr->status = 0;
	if (e1000_data->vlan_tag_add) {
		tx_descriptor_addr->special = e1000_data->vlan_tag;
		tx_descriptor_addr->command |= TXDESCRIPTOR_COMMAND_VLE;
	} else
		tx_descriptor_addr->special = 0;
	
	tx_descriptor_addr->checksum_start_field = 0;
	
	tdt++;
	if (tdt == E1000_TX_PACKETS_COUNT)
		tdt = 0;
	
	E1000_REG_WRITE(e1000_data, E1000_TDT, tdt);
	
	fibril_mutex_unlock(&e1000_data->tx_lock);
}

int main(void)
{
	int rc = dma_allocator_init();
	if (rc != EOK)
		return rc;
	
	nic_driver_implement(&e1000_driver_ops, &e1000_dev_ops, &e1000_nic_iface);
	e1000_driver_init();
	return ddf_driver_main(&e1000_driver);
}
