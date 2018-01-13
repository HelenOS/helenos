/*
 * Copyright (c) 2011 Jiri Michalec
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
#include <align.h>
#include <byteorder.h>
#include <libarch/barrier.h>
#include <as.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>
#include <device/hw_res.h>
#include <io/log.h>
#include <nic.h>
#include <pci_dev_iface.h>
#include <stdio.h>
#include <str.h>

#include "defs.h"
#include "driver.h"
#include "general.h"

/** Global mutex for work with shared irq structure */
FIBRIL_MUTEX_INITIALIZE(irq_reg_lock);

/** Lock interrupt structure mutex */
#define RTL8139_IRQ_STRUCT_LOCK() \
	fibril_mutex_lock(&irq_reg_lock)

/** Unlock interrupt structure mutex */
#define RTL8139_IRQ_STRUCT_UNLOCK() \
	fibril_mutex_unlock(&irq_reg_lock)

/** PCI clock frequency in kHz */
#define RTL8139_PCI_FREQ_KHZ  33000

#define RTL8139_AUTONEG_CAPS (ETH_AUTONEG_10BASE_T_HALF | \
	ETH_AUTONEG_10BASE_T_FULL | ETH_AUTONEG_100BASE_TX_HALF | \
	ETH_AUTONEG_100BASE_TX_FULL | ETH_AUTONEG_PAUSE_SYMETRIC)

/** Lock transmitter and receiver data
 *
 * This function shall be called whenever
 * both transmitter and receiver locking
 * to force safe lock ordering (deadlock prevention)
 *
 * @param rtl8139 RTL8139 private data
 *
 */
inline static void rtl8139_lock_all(rtl8139_t *rtl8139)
{
	assert(rtl8139);
	fibril_mutex_lock(&rtl8139->tx_lock);
	fibril_mutex_lock(&rtl8139->rx_lock);
}

/** Unlock transmitter and receiver data
 *
 * @param rtl8139 RTL8139 private data
 *
 */
inline static void rtl8139_unlock_all(rtl8139_t *rtl8139)
{
	assert(rtl8139);
	fibril_mutex_unlock(&rtl8139->rx_lock);
	fibril_mutex_unlock(&rtl8139->tx_lock);
}

#ifndef RXBUF_SIZE_FLAGS
	/** Flags for receiver buffer - 16kB default */
	#define RXBUF_SIZE_FLAGS RTL8139_RXFLAGS_SIZE_16
#endif

#if (RXBUF_SIZE_FLAGS > RTL8139_RXFLAGS_SIZE_64) || (RXBUF_SIZE_FLAGS < 0)
	#error Bad receiver buffer flags size flags
#endif

/** Size of the receiver buffer
 *
 *  Incrementing flags by one twices the buffer size
 *  the lowest size is 8*1024 (flags = 0)
 */
#define RxBUF_SIZE RTL8139_RXSIZE(RXBUF_SIZE_FLAGS)

/** Total size of the receiver buffer to allocate */
#define RxBUF_TOT_LENGTH RTL8139_RXBUF_LENGTH(RXBUF_SIZE_FLAGS)


/** Default interrupt mask */
#define RTL_DEFAULT_INTERRUPTS UINT16_C(0xFFFF)

/** Obtain the value of the register part
 *  The bit operations will be done
 *  The _SHIFT and _MASK for the register part must exists as macros
 *  or variables
 */
#define REG_GET_VAL(value, reg_part)\
		(((value) >> reg_part##_SHIFT) & reg_part##_MASK)


/** Set interrupts on controller
 *
 *  @param rtl8139  The card private structure
 */
inline static void rtl8139_hw_int_set(rtl8139_t *rtl8139)
{
	pio_write_16(rtl8139->io_port + IMR, rtl8139->int_mask);
}

/** Check on the controller if the receiving buffer is empty
 *
 *  @param rtl8139  The controller data
 *
 *  @return Nonzero if empty, zero otherwise
 */
inline static int rtl8139_hw_buffer_empty(rtl8139_t *rtl8139)
{
	return pio_read_16(rtl8139->io_port + CR) & CR_BUFE;
}

/** Update the mask of accepted frames in the RCR register according to
 * rcr_accept_mode value in rtl8139_t
 *
 * @param rtl8139  The rtl8139 private data
 */
static void rtl8139_hw_update_rcr(rtl8139_t *rtl8139)
{
	uint32_t rcr = rtl8139->rcr_data.rcr_base | rtl8139->rcr_data.ucast_mask
	    | rtl8139->rcr_data.mcast_mask | rtl8139->rcr_data.bcast_mask
	    | rtl8139->rcr_data.defect_mask | 
	    (RXBUF_SIZE_FLAGS << RCR_RBLEN_SHIFT);
	
	ddf_msg(LVL_DEBUG, "Rewriting rcr: %x -> %x", pio_read_32(rtl8139->io_port + RCR),
	    rcr);

	pio_write_32(rtl8139->io_port + RCR, rcr);
}

/** Fill the mask of accepted multicast frames in the card registers
 *
 *  @param rtl8139  The rtl8139 private data
 *  @param mask     The mask to set
 */
inline static void rtl8139_hw_set_mcast_mask(rtl8139_t *rtl8139,
    uint64_t mask)
{
	pio_write_32(rtl8139->io_port + MAR0, (uint32_t) mask);
	pio_write_32(rtl8139->io_port + MAR0 + sizeof(uint32_t),
	    (uint32_t)(mask >> 32));
	return;
}

/** Set PmEn (Power management enable) bit value
 *
 *  @param rtl8139  rtl8139 card data
 *  @param bit_val  If bit_val is zero pmen is set to 0, otherwise pmen is set to 1
 */
inline static void rtl8139_hw_pmen_set(rtl8139_t *rtl8139, uint8_t bit_val)
{
	uint8_t config1 = pio_read_8(rtl8139->io_port + CONFIG1);
	uint8_t config1_new;
	if (bit_val)
		config1_new = config1 | CONFIG1_PMEn;
	else
		config1_new = config1 & ~(uint8_t)(CONFIG1_PMEn);

	if (config1_new == config1)
		return;

	rtl8139_regs_unlock(rtl8139->io_port);
	pio_write_8(rtl8139->io_port + CONFIG1, config1_new);
	rtl8139_regs_lock(rtl8139->io_port);

	async_sess_t *pci_sess =
		ddf_dev_parent_sess_get(nic_get_ddf_dev(rtl8139->nic_data));

	if (bit_val) {
		uint8_t pmen;
		pci_config_space_read_8(pci_sess, 0x55, &pmen);
		pci_config_space_write_8(pci_sess, 0x55, pmen | 1 | (1 << 7));
	} else {
		uint8_t pmen;
		pci_config_space_read_8(pci_sess, 0x55, &pmen);
		pci_config_space_write_8(pci_sess, 0x55, pmen & ~(1 | (1 << 7)));
	}
}

/** Get MAC address of the RTL8139 adapter
 *
 *  @param rtl8139  The RTL8139 device
 *  @param address  The place to store the address
 *
 *  @return EOK if succeed, error code otherwise
 */
inline static void rtl8139_hw_get_addr(rtl8139_t *rtl8139,
    nic_address_t *addr)
{
	assert(rtl8139);
	assert(addr);

	uint32_t *mac0_dest = (uint32_t *)addr->address;
	uint16_t *mac4_dest = (uint16_t *)(addr->address + 4);

	/* Read MAC address from the i/o (4byte + 2byte reads) */
	*mac0_dest = pio_read_32(rtl8139->io_port + MAC0);
	*mac4_dest = pio_read_16(rtl8139->io_port + MAC0 + 4);
};

/** Set MAC address to the device
 *
 *  @param rtl8139  Controller private structure
 *  @param addr     The address to set
 */
static void rtl8139_hw_set_addr(rtl8139_t *rtl8139, const nic_address_t *addr)
{
	assert(rtl8139);
	assert(addr);

	const uint32_t *val1 = (const uint32_t*)addr->address;
	const uint16_t *val2 = (const uint16_t*)(addr->address + sizeof(uint32_t));

	rtl8139_regs_unlock(rtl8139->io_port);
	pio_write_32(rtl8139->io_port + MAC0, *val1);
	pio_write_32(rtl8139->io_port + MAC0 + 4, *val2);
	rtl8139_regs_lock(rtl8139->io_port);
}

/**  Provide OR in the 8bit register (set selected bits to 1)
 *
 *   @param rtl8139     The rtl8139 structure
 *   @param reg_offset  Register offset in the device IO space
 *   @param bits_add    The value to or
 */
inline static void rtl8139_hw_reg_add_8(rtl8139_t * rtl8139, size_t reg_offset,
    uint8_t bits_add)
{
	uint8_t value = pio_read_8(rtl8139->io_port + reg_offset);
	value |= bits_add;
	pio_write_8(rtl8139->io_port + reg_offset, value);
}

/**  Unset selected bits in 8bit register
 *
 *   @param rtl8139     The rtl8139 structure
 *   @param reg_offset  Register offset in the device IO space
 *   @param bits_add    The mask of bits to remove
 */
inline static void rtl8139_hw_reg_rem_8(rtl8139_t * rtl8139, size_t reg_offset,
    uint8_t bits_add)
{
	uint8_t value = pio_read_8(rtl8139->io_port + reg_offset);
	value &= ~bits_add;
	pio_write_8(rtl8139->io_port + reg_offset, value);
}

static errno_t rtl8139_set_addr(ddf_fun_t *fun, const nic_address_t *);
static errno_t rtl8139_get_device_info(ddf_fun_t *fun, nic_device_info_t *info);
static errno_t rtl8139_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state);
static errno_t rtl8139_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role);
static errno_t rtl8139_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t);

static errno_t rtl8139_pause_get(ddf_fun_t*, nic_result_t*, nic_result_t*, 
    uint16_t *);
static errno_t rtl8139_pause_set(ddf_fun_t*, int, int, uint16_t);

static errno_t rtl8139_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement);
static errno_t rtl8139_autoneg_disable(ddf_fun_t *fun);
static errno_t rtl8139_autoneg_probe(ddf_fun_t *fun, uint32_t *our_advertisement,
    uint32_t *their_advertisement, nic_result_t *result, 
    nic_result_t *their_result);
static errno_t rtl8139_autoneg_restart(ddf_fun_t *fun);

static errno_t rtl8139_defective_get_mode(ddf_fun_t *fun, uint32_t *mode);
static errno_t rtl8139_defective_set_mode(ddf_fun_t *fun, uint32_t mode);

static errno_t rtl8139_wol_virtue_add(nic_t *nic_data,
	const nic_wol_virtue_t *virtue);
static void rtl8139_wol_virtue_rem(nic_t *nic_data,
	const nic_wol_virtue_t *virtue);

static errno_t rtl8139_poll_mode_change(nic_t *nic_data, nic_poll_mode_t mode,
    const struct timeval *period);
static void rtl8139_poll(nic_t *nic_data);

/** Network interface options for RTL8139 card driver */
static nic_iface_t rtl8139_nic_iface = {
	.set_address = &rtl8139_set_addr,
	.get_device_info = &rtl8139_get_device_info,
	.get_cable_state = &rtl8139_get_cable_state,
	.get_operation_mode = &rtl8139_get_operation_mode,
	.set_operation_mode = &rtl8139_set_operation_mode,

	.get_pause = &rtl8139_pause_get,
	.set_pause = &rtl8139_pause_set,

	.autoneg_enable = &rtl8139_autoneg_enable,
	.autoneg_disable = &rtl8139_autoneg_disable,
	.autoneg_probe = &rtl8139_autoneg_probe,
	.autoneg_restart = &rtl8139_autoneg_restart,

	.defective_get_mode = &rtl8139_defective_get_mode,
	.defective_set_mode = &rtl8139_defective_set_mode,
};

/** Basic device operations for RTL8139 driver */
static ddf_dev_ops_t rtl8139_dev_ops;

static errno_t rtl8139_dev_add(ddf_dev_t *dev);

/** Basic driver operations for RTL8139 driver */
static driver_ops_t rtl8139_driver_ops = {
	.dev_add = &rtl8139_dev_add,
};

/** Driver structure for RTL8139 driver */
static driver_t rtl8139_driver = {
	.name = NAME,
	.driver_ops = &rtl8139_driver_ops
};

/* The default implementation callbacks */
static errno_t rtl8139_on_activated(nic_t *nic_data);
static errno_t rtl8139_on_stopped(nic_t *nic_data);
static void rtl8139_send_frame(nic_t *nic_data, void *data, size_t size);

/** Check if the transmit buffer is busy */
#define rtl8139_tbuf_busy(tsd) ((pio_read_32(tsd) & TSD_OWN) == 0)

/** Send frame with the hardware
 *
 * note: the main_lock is locked when framework calls this function
 *
 * @param nic_data  The nic driver data structure
 * @param data      Frame data
 * @param size      Frame size in bytes
 *
 * @return EOK if succeed, error code in the case of error
 */
static void rtl8139_send_frame(nic_t *nic_data, void *data, size_t size)
{
	assert(nic_data);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);
	ddf_msg(LVL_DEBUG, "Sending frame");

	if (size > RTL8139_FRAME_MAX_LENGTH) {
		ddf_msg(LVL_ERROR, "Send frame: frame too long, %zu bytes",
		    size);
		nic_report_send_error(rtl8139->nic_data, NIC_SEC_OTHER, 1);
		goto err_size;
	}

	assert((size & TSD_SIZE_MASK) == size);

	/* Lock transmitter structure for obtaining next buffer */
	fibril_mutex_lock(&rtl8139->tx_lock);

	/* Check if there is free buffer */
	if (rtl8139->tx_next - TX_BUFF_COUNT == rtl8139->tx_used) {
		nic_set_tx_busy(nic_data, 1);
		fibril_mutex_unlock(&rtl8139->tx_lock);
		nic_report_send_error(nic_data, NIC_SEC_BUFFER_FULL, 1);
		goto err_busy_no_inc;
	}

	/* Get buffer id to use and set next buffer to use */
	size_t tx_curr = rtl8139->tx_next++ % TX_BUFF_COUNT;

	fibril_mutex_unlock(&rtl8139->tx_lock);

	/* Get address of the buffer descriptor and frame data */
	void *tsd = rtl8139->io_port + TSD0 + tx_curr * 4;
	void *buf_addr = rtl8139->tx_buff[tx_curr];

	/* Wait until the buffer is free */
	assert(!rtl8139_tbuf_busy(tsd));

	/* Write frame data to the buffer, set the size to TSD and clear OWN bit */
	memcpy(buf_addr, data, size);

	/* Set size of the data to send */
	uint32_t tsd_value = pio_read_32(tsd);
	tsd_value = rtl8139_tsd_set_size(tsd_value, size);
	pio_write_32(tsd, tsd_value);

	/* barrier for HW to really see the current buffer data */
	write_barrier();

	tsd_value &= ~(uint32_t)TSD_OWN;
	pio_write_32(tsd, tsd_value);
	return;
	
err_busy_no_inc:
err_size:
	return;
};


/** Reset the controller
 *
 *  @param io_base  The address of the i/o port mapping start
 */
inline static void rtl8139_hw_soft_reset(void *io_base)
{
	pio_write_8(io_base + CR, CR_RST);
	memory_barrier();
	while(pio_read_8(io_base + CR) & CR_RST) {
		async_usleep(1);
		read_barrier();
	}
}

/** Provide soft reset of the controller
 *
 * The caller must lock tx_lock and rx_lock before calling this function
 *
 */
static void rtl8139_soft_reset(rtl8139_t *rtl8139)
{
	assert(rtl8139);

	rtl8139_hw_soft_reset(rtl8139->io_port);
	nic_t *nic_data = rtl8139->nic_data;

	/* Write MAC address to the card */
	nic_address_t addr;
	nic_query_address(nic_data, &addr);
	rtl8139_hw_set_addr(rtl8139, &addr);

	/* Recover accept modes back */
	rtl8139_hw_set_mcast_mask(rtl8139, nic_query_mcast_hash(nic_data));
	rtl8139_hw_update_rcr(rtl8139);

	rtl8139->tx_used = 0;
	rtl8139->tx_next = 0;
	nic_set_tx_busy(rtl8139->nic_data, 0);
}

/** Create frame structure from the buffer data
 *
 * @param nic_data      NIC driver data
 * @param rx_buffer     The receiver buffer
 * @param rx_size       The buffer size
 * @param frame_start   The offset where packet data start
 * @param frame_size    The size of the frame data
 *
 * @return The frame list node (not connected)
 *
 */
static nic_frame_t *rtl8139_read_frame(nic_t *nic_data,
    void *rx_buffer, size_t rx_size, size_t frame_start, size_t frame_size)
{
	nic_frame_t *frame = nic_alloc_frame(nic_data, frame_size);
	if (! frame) {
		ddf_msg(LVL_ERROR, "Can not allocate frame for received frame.");
		return NULL;
	}

	void *ret = rtl8139_memcpy_wrapped(frame->data, rx_buffer, frame_start,
	    RxBUF_SIZE, frame_size);
	if (ret == NULL) {
		nic_release_frame(nic_data, frame);
		return NULL;
	}
	return frame;
}

/* Reset receiver
 *
 * Use in the case of receiver error (lost in the rx_buff)
 *
 * @param rtl8139  controller private data
 */
static void rtl8139_rx_reset(rtl8139_t *rtl8139) 
{
	/* Disable receiver, update offset and enable receiver again */
	uint8_t cr = pio_read_8(rtl8139->io_port + CR);
	rtl8139_regs_unlock(rtl8139);

	pio_write_8(rtl8139->io_port + CR, cr & ~(uint8_t)CR_RE);

	write_barrier();
	pio_write_32(rtl8139->io_port + CAPR, 0);
	pio_write_32(rtl8139->io_port + RBSTART, 
	    PTR2U32(rtl8139->rx_buff_phys));

	write_barrier();

	rtl8139_hw_update_rcr(rtl8139);
	pio_write_8(rtl8139->io_port + CR, cr);
	rtl8139_regs_lock(rtl8139);

	nic_report_receive_error(rtl8139->nic_data, NIC_REC_OTHER, 1);
}

/** Receive all frames in queue
 *
 *  @param nic_data  The controller data
 *  @return The linked list of nic_frame_list_t nodes, each containing one frame
 */
static nic_frame_list_t *rtl8139_frame_receive(nic_t *nic_data)
{
	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	if (rtl8139_hw_buffer_empty(rtl8139))
		return NULL;

	nic_frame_list_t *frames = nic_alloc_frame_list();
	if (!frames)
		ddf_msg(LVL_ERROR, "Can not allocate frame list for received frames.");

	void *rx_buffer = rtl8139->rx_buff_virt;

	/* where to start reading */
	uint16_t rx_offset = pio_read_16(rtl8139->io_port + CAPR) + 16;
	/* unread bytes count */
	uint16_t bytes_received = pio_read_16(rtl8139->io_port + CBA);
	uint16_t max_read;
	uint16_t cur_read = 0;

	/* get values to the <0, buffer size) */
	bytes_received %= RxBUF_SIZE;
	rx_offset %= RxBUF_SIZE;
	
	/* count how many bytes to read maximaly */
	if (bytes_received < rx_offset)
		max_read = bytes_received + (RxBUF_SIZE - rx_offset);
	else
		max_read = bytes_received - rx_offset;

	memory_barrier();
	while (!rtl8139_hw_buffer_empty(rtl8139)) {
		void *rx_ptr = rx_buffer + rx_offset % RxBUF_SIZE;
		uint32_t frame_header = uint32_t_le2host( *((uint32_t*)rx_ptr) );
		uint16_t size = frame_header >> 16;
		uint16_t frame_size = size - RTL8139_CRC_SIZE;
		/* received frame flags in frame header */
		uint16_t rcs = (uint16_t) frame_header;

		if (size == RTL8139_EARLY_SIZE) {
			/* The frame copying is still in progress, break receiving */
			ddf_msg(LVL_DEBUG, "Early threshold reached, not completely coppied");
			break;
		}

		/* Check if the header is valid, otherwise we are lost in the buffer */
		if (size == 0 || size > RTL8139_FRAME_MAX_LENGTH) {
			ddf_msg(LVL_ERROR, "Receiver error -> receiver reset (size: %4" PRIu16 ", "
			    "header 0x%4" PRIx32 ". Offset: %" PRIu16 ")", size, frame_header,
			    rx_offset);
			goto rx_err;
		}
		if (size < RTL8139_RUNT_MAX_SIZE && !(rcs & RSR_RUNT)) {
			ddf_msg(LVL_ERROR, "Receiver error -> receiver reset (%"PRIx16")", size);
			goto rx_err;
		}

		cur_read += size + RTL_FRAME_HEADER_SIZE;
		if (cur_read > max_read)
			break;

		if (frames) {
			nic_frame_t *frame = rtl8139_read_frame(nic_data, rx_buffer,
			    RxBUF_SIZE, rx_offset + RTL_FRAME_HEADER_SIZE, frame_size);

			if (frame)
				nic_frame_list_append(frames, frame);
		}

		/* Update offset */
		rx_offset = ALIGN_UP(rx_offset + size + RTL_FRAME_HEADER_SIZE, 4);

		/* Write lesser value to prevent overflow into unread frame
		 * (the recomendation from the RealTech rtl8139 programming guide)
		 */
		uint16_t capr_val = rx_offset - 16;
		pio_write_16(rtl8139->io_port + CAPR, capr_val);

		/* Ensure no CR read optimalization during next empty buffer test */
		memory_barrier();
	}
	return frames;
rx_err:
	rtl8139_rx_reset(rtl8139);
	return frames;
};


irq_pio_range_t rtl8139_irq_pio_ranges[] = {
	{
		.base = 0,
		.size = RTL8139_IO_SIZE
	}
};

/** Commands to deal with interrupt
 *
 *  Read ISR, check if tere is any interrupt pending.
 *  If so, reset it and accept the interrupt.
 *  The .addr of the first and third command must
 *  be filled to the ISR port address
 */
irq_cmd_t rtl8139_irq_commands[] = {
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
irq_code_t rtl8139_irq_code = {
	.rangecount = sizeof(rtl8139_irq_pio_ranges) / sizeof(irq_pio_range_t),
	.ranges = rtl8139_irq_pio_ranges,
	.cmdcount = sizeof(rtl8139_irq_commands) / sizeof(irq_cmd_t),
	.cmds = rtl8139_irq_commands
};

/** Deal with transmitter interrupt
 *
 *  @param nic_data  Nic driver data
 */
static void rtl8139_tx_interrupt(nic_t *nic_data)
{
	rtl8139_t *rtl8139 = nic_get_specific(nic_data);

	fibril_mutex_lock(&rtl8139->tx_lock);

	size_t tx_next = rtl8139->tx_next;
	size_t tx_used = rtl8139->tx_used;
	while (tx_used != tx_next) {
		size_t desc_to_check = tx_used % TX_BUFF_COUNT;
		void * tsd_to_check = rtl8139->io_port + TSD0 
		    + desc_to_check * sizeof(uint32_t);
		uint32_t tsd_value = pio_read_32(tsd_to_check);

		/* If sending is still in the progress */
		if ((tsd_value & TSD_OWN) == 0)
			break;

		tx_used++;

		/* If the frame was sent */
		if (tsd_value & TSD_TOK) {
			size_t size = REG_GET_VAL(tsd_value, TSD_SIZE);
			nic_report_send_ok(nic_data, 1, size);
		} else if (tsd_value & TSD_CRS) {
			nic_report_send_error(nic_data, NIC_SEC_CARRIER_LOST, 1);
		} else if (tsd_value & TSD_OWC) {
			nic_report_send_error(nic_data, NIC_SEC_WINDOW_ERROR, 1);
		} else if (tsd_value & TSD_TABT) {
			nic_report_send_error(nic_data, NIC_SEC_ABORTED, 1);
		} else if (tsd_value & TSD_CDH) {
			nic_report_send_error(nic_data, NIC_SEC_HEARTBEAT, 1);
		}

		unsigned collisions = REG_GET_VAL(tsd_value, TSD_NCC);
		if (collisions > 0) {
			nic_report_collisions(nic_data, collisions);
		}

		if (tsd_value & TSD_TUN) {
			nic_report_send_error(nic_data, NIC_SEC_FIFO_OVERRUN, 1);
		}
	}
	if (rtl8139->tx_used != tx_used) {
		rtl8139->tx_used = tx_used;
		nic_set_tx_busy(nic_data, 0);
	}
	fibril_mutex_unlock(&rtl8139->tx_lock);
}

/** Receive all frames from the buffer
 *
 *  @param rtl8139  driver private data
 */
static void rtl8139_receive_frames(nic_t *nic_data)
{
	assert(nic_data);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	fibril_mutex_lock(&rtl8139->rx_lock);
	nic_frame_list_t *frames = rtl8139_frame_receive(nic_data);
	fibril_mutex_unlock(&rtl8139->rx_lock);

	if (frames)
		nic_received_frame_list(nic_data, frames);
}


/** Deal with poll interrupt
 *
 *  @param nic_data  Nic driver data
 */
static int rtl8139_poll_interrupt(nic_t *nic_data)
{
	assert(nic_data);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	uint32_t timer_val;
	int receive = rtl8139_timer_act_step(&rtl8139->poll_timer, &timer_val);

	assert(timer_val);
	pio_write_32(rtl8139->io_port + TIMERINT, timer_val);
	pio_write_32(rtl8139->io_port + TCTR, 0x0);
	ddf_msg(LVL_DEBUG, "rtl8139 timer: %"PRIu32"\treceive: %d", timer_val, receive);
	return receive;
}


/** Poll device according to isr status
 *
 *  The isr value must be obtained and cleared by the caller. The reason
 *  of this function separate is to allow polling from both interrupt
 *  (which clears controller ISR before the handler runs) and the polling
 *  callbacks.
 *
 *  @param nic_data  Driver data
 *  @param isr       Interrupt status register value
 */
static void rtl8139_interrupt_impl(nic_t *nic_data, uint16_t isr)
{
	assert(nic_data);
	
	nic_poll_mode_t poll_mode = nic_query_poll_mode(nic_data, 0);

	/* Process only when should in the polling mode */
	if (poll_mode == NIC_POLL_PERIODIC) {
		int receive = 0;
		if (isr & INT_TIME_OUT) {
			receive = rtl8139_poll_interrupt(nic_data);
		}
		if (! receive)
			return;
	}

	/* Check transmittion interrupts first to allow transmit next frames
	 * sooner
	 */
	if (isr & (INT_TOK | INT_TER)) {
		rtl8139_tx_interrupt(nic_data);
	}
	if (isr & INT_ROK) {
		rtl8139_receive_frames(nic_data);
	}
	if (isr & (INT_RER | INT_RXOVW | INT_FIFOOVW)) {
		if (isr & INT_RER) {
			//TODO: is this only the general error, or any particular?
		}
		if (isr & (INT_FIFOOVW)) {
			nic_report_receive_error(nic_data, NIC_REC_FIFO_OVERRUN, 1);
		} else if (isr & (INT_RXOVW)) {
			rtl8139_t *rtl8139 = nic_get_specific(nic_data);
			assert(rtl8139);

			uint32_t miss = pio_read_32(rtl8139->io_port + MPC) & MPC_VMASK;
			pio_write_32(rtl8139->io_port + MPC, 0);
			nic_report_receive_error(nic_data, NIC_REC_BUFFER_OVERFLOW, miss);
		}
	}
}

/** Handle device interrupt
 *
 * @param icall  The IPC call structure
 * @param dev    The rtl8139 device
 *
 */
static void rtl8139_interrupt_handler(ipc_call_t *icall, ddf_dev_t *dev)
{
	assert(dev);
	assert(icall);

	uint16_t isr = (uint16_t) IPC_GET_ARG2(*icall);
	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8139_t *rtl8139 = nic_get_specific(nic_data);

	rtl8139_interrupt_impl(nic_data, isr);

	/* Turn the interrupts on again */
	rtl8139_hw_int_set(rtl8139);
};

/** Register interrupt handler for the card in the system
 *
 *  Note: the global irq_reg_mutex is locked because of work with global
 *  structure.
 *
 *  @param nic_data  The driver data
 *
 *  @param[out] handle  IRQ capability handle if the handler was registered.
 *
 *  @return An error code otherwise.
 */
inline static errno_t rtl8139_register_int_handler(nic_t *nic_data, cap_handle_t *handle)
{
	rtl8139_t *rtl8139 = nic_get_specific(nic_data);

	/* Lock the mutex in whole driver while working with global structure */
	RTL8139_IRQ_STRUCT_LOCK();

	rtl8139_irq_code.ranges[0].base = (uintptr_t) rtl8139->io_addr;
	rtl8139_irq_code.cmds[0].addr = rtl8139->io_addr + ISR;
	rtl8139_irq_code.cmds[2].addr = rtl8139->io_addr + ISR;
	rtl8139_irq_code.cmds[3].addr = rtl8139->io_addr + IMR;
	errno_t rc = register_interrupt_handler(nic_get_ddf_dev(nic_data),
	    rtl8139->irq, rtl8139_interrupt_handler, &rtl8139_irq_code, handle);

	RTL8139_IRQ_STRUCT_UNLOCK();

	return rc;
}

/** Start the controller
 *
 * The caller must lock tx_lock and rx_lock before calling this function
 *
 * @param rtl8139  The card private data
 */
inline static void rtl8139_card_up(rtl8139_t *rtl8139)
{
	void *io_base = rtl8139->io_port;
	size_t i;

	/* Wake up the device */
	pio_write_8(io_base + CONFIG1, 0x00);
	/* Reset the device */
	rtl8139_soft_reset(rtl8139);

	/* Write transmittion buffer addresses */
	for(i = 0; i < TX_BUFF_COUNT; ++i) {
		uint32_t addr = PTR2U32(rtl8139->tx_buff_phys + i*TX_BUFF_SIZE);
		pio_write_32(io_base + TSAD0 + 4*i, addr);
	}
	rtl8139->tx_next = 0;
	rtl8139->tx_used = 0;
	nic_set_tx_busy(rtl8139->nic_data, 0);

	pio_write_32(io_base + RBSTART, PTR2U32(rtl8139->rx_buff_phys));

	/* Enable transmitter and receiver */
	uint8_t cr_value = pio_read_8(io_base + CR);
	pio_write_8(io_base + CR, cr_value | CR_TE | CR_RE);
	rtl8139_hw_update_rcr(rtl8139);
}

/** Activate the device to receive and transmit frames
 *
 *  @param nic_data  The nic driver data
 *
 *  @return EOK if activated successfully, error code otherwise
 */
static errno_t rtl8139_on_activated(nic_t *nic_data)
{
	assert(nic_data);
	ddf_msg(LVL_NOTE, "Activating device");

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	rtl8139_lock_all(rtl8139);
	rtl8139_card_up(rtl8139);
	rtl8139_unlock_all(rtl8139);

	rtl8139->int_mask = RTL_DEFAULT_INTERRUPTS;
	rtl8139_hw_int_set(rtl8139);

	errno_t rc = hw_res_enable_interrupt(rtl8139->parent_sess, rtl8139->irq);
	if (rc != EOK) {
		rtl8139_on_stopped(nic_data);
		return rc;
	}

	ddf_msg(LVL_DEBUG, "Device activated, interrupt %d registered", rtl8139->irq);
	return EOK;
}

/** Callback for NIC_STATE_STOPPED change
 *
 *  @param nic_data  The nic driver data
 *
 *  @return EOK if succeed, error code otherwise
 */
static errno_t rtl8139_on_stopped(nic_t *nic_data)
{
	assert(nic_data);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	rtl8139->rcr_data.ucast_mask = RTL8139_RCR_UCAST_DEFAULT;
	rtl8139->rcr_data.mcast_mask = RTL8139_RCR_MCAST_DEFAULT;
	rtl8139->rcr_data.bcast_mask = RTL8139_RCR_BCAST_DEFAULT;
	rtl8139->rcr_data.defect_mask = RTL8139_RCR_DEFECT_DEFAULT;

	/* Reset the card to the initial state (interrupts, Tx and Rx disabled) */
	rtl8139_lock_all(rtl8139);
	rtl8139_soft_reset(rtl8139);
	rtl8139_unlock_all(rtl8139);
	return EOK;
}


static errno_t rtl8139_unicast_set(nic_t *nic_data, nic_unicast_mode_t mode,
    const nic_address_t *, size_t);
static errno_t rtl8139_multicast_set(nic_t *nic_data, nic_multicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count);
static errno_t rtl8139_broadcast_set(nic_t *nic_data, nic_broadcast_mode_t mode);


/** Create driver data structure
 *
 *  @return Intialized device data structure or NULL
 */
static rtl8139_t *rtl8139_create_dev_data(ddf_dev_t *dev)
{
	assert(dev);
	assert(!nic_get_from_ddf_dev(dev));

	nic_t *nic_data = nic_create_and_bind(dev);
	if (!nic_data)
		return NULL;

	rtl8139_t *rtl8139 = calloc(1, sizeof(rtl8139_t));
	if (!rtl8139) {
		nic_unbind_and_destroy(dev);
		return NULL;
	}

	rtl8139->dev = dev;

	rtl8139->nic_data = nic_data;
	nic_set_specific(nic_data, rtl8139);
	nic_set_send_frame_handler(nic_data, rtl8139_send_frame);
	nic_set_state_change_handlers(nic_data,
		rtl8139_on_activated, NULL, rtl8139_on_stopped);
	nic_set_filtering_change_handlers(nic_data,
		rtl8139_unicast_set, rtl8139_multicast_set, rtl8139_broadcast_set,
		NULL, NULL);
	nic_set_wol_virtue_change_handlers(nic_data,
		rtl8139_wol_virtue_add, rtl8139_wol_virtue_rem);
	nic_set_poll_handlers(nic_data, rtl8139_poll_mode_change, rtl8139_poll);


	fibril_mutex_initialize(&rtl8139->rx_lock);
	fibril_mutex_initialize(&rtl8139->tx_lock);

	nic_set_wol_max_caps(nic_data, NIC_WV_BROADCAST, 1);
	nic_set_wol_max_caps(nic_data, NIC_WV_LINK_CHANGE, 1);
	nic_set_wol_max_caps(nic_data, NIC_WV_MAGIC_PACKET, 1);

	return rtl8139;
}

/** Clean up the rtl8139 device structure.
 *
 * @param dev  The device structure.
 */
static void rtl8139_dev_cleanup(ddf_dev_t *dev)
{
	assert(dev);

	if (ddf_dev_data_get(dev))
		nic_unbind_and_destroy(dev);
}

/** Fill the irq and io_addr part of device data structure
 *
 *  The hw_resources must be obtained before calling this function
 *
 *  @param dev           The device structure
 *  @param hw_resources  Devices hardware resources
 *
 *  @return EOK if succeed, error code otherwise
 */
static errno_t rtl8139_fill_resource_info(ddf_dev_t *dev, const hw_res_list_parsed_t
    *hw_resources)
{
	assert(dev);
	assert(hw_resources);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_dev(dev));
	assert(rtl8139);

	if (hw_resources->irqs.count != 1) {
		ddf_msg(LVL_ERROR, "%s device: unexpected irq count", ddf_dev_get_name(dev));
		return EINVAL;
	};
	if (hw_resources->io_ranges.count != 1) {
		ddf_msg(LVL_ERROR, "%s device: unexpected io ranges count", ddf_dev_get_name(dev));
		return EINVAL;
	}

	rtl8139->irq = hw_resources->irqs.irqs[0];
	ddf_msg(LVL_DEBUG, "%s device: irq 0x%x assigned", ddf_dev_get_name(dev), rtl8139->irq);

	rtl8139->io_addr = IOADDR_TO_PTR(RNGABS(hw_resources->io_ranges.ranges[0]));
	if (hw_resources->io_ranges.ranges[0].size < RTL8139_IO_SIZE) {
		ddf_msg(LVL_ERROR, "i/o range assigned to the device "
		    "%s is too small.", ddf_dev_get_name(dev));
		return EINVAL;
	}
	ddf_msg(LVL_DEBUG, "%s device: i/o addr %p assigned.", ddf_dev_get_name(dev), rtl8139->io_addr);

	return EOK;
}

/** Obtain information about hardware resources of the device
 *
 *  The device must be connected to the parent
 *
 *  @param dev  The device structure
 *
 *  @return EOK if succeed, error code otherwise
 */
static errno_t rtl8139_get_resource_info(ddf_dev_t *dev)
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
	errno_t ret = rtl8139_fill_resource_info(dev, &hw_res_parsed);
	hw_res_list_parsed_clean(&hw_res_parsed);

	return ret;
}


/** Allocate buffers using DMA framework
 *
 * The buffers structures in the device specific data is filled
 *
 * @param data  The device specific structure to fill
 *
 * @return EOK in the case of success, error code otherwise
 */
static errno_t rtl8139_buffers_create(rtl8139_t *rtl8139)
{
	size_t i = 0;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Creating buffers");
	
	rtl8139->tx_buff_virt = AS_AREA_ANY;
	rc = dmamem_map_anonymous(TX_PAGES * PAGE_SIZE, DMAMEM_4GiB,
	    AS_AREA_WRITE, 0, &rtl8139->tx_buff_phys, &rtl8139->tx_buff_virt);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Can not allocate transmitter buffers.");
		goto err_tx_alloc;
	}

	for (i = 0; i < TX_BUFF_COUNT; ++i)
		rtl8139->tx_buff[i] = rtl8139->tx_buff_virt + i * TX_BUFF_SIZE;

	ddf_msg(LVL_DEBUG, "The transmittion buffers allocated");

	/* Use the first buffer for next transmittion */
	rtl8139->tx_next = 0;
	rtl8139->tx_used = 0;

	/* Allocate buffer for receiver */
	ddf_msg(LVL_DEBUG, "Allocating receiver buffer of the size %d bytes",
	    RxBUF_TOT_LENGTH);
	
	rtl8139->rx_buff_virt = AS_AREA_ANY;
	rc = dmamem_map_anonymous(RxBUF_TOT_LENGTH, DMAMEM_4GiB,
	    AS_AREA_READ, 0, &rtl8139->rx_buff_phys, &rtl8139->rx_buff_virt);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Can not allocate receive buffer.");
		goto err_rx_alloc;
	}
	ddf_msg(LVL_DEBUG, "The buffers created");

	return EOK;

err_rx_alloc:
	dmamem_unmap_anonymous(&rtl8139->tx_buff_virt);
err_tx_alloc:
	return rc;
}

/** Initialize the rtl8139 device structure
 *
 *  @param dev  The device information
 *
 *  @return EOK if succeed, error code otherwise
 */
static errno_t rtl8139_device_initialize(ddf_dev_t *dev)
{
	ddf_msg(LVL_DEBUG, "rtl8139_dev_initialize %s", ddf_dev_get_name(dev));

	errno_t ret = EOK;

	ddf_msg(LVL_DEBUG, "rtl8139: creating device data");

	/* Allocate driver data for the device. */
	rtl8139_t *rtl8139 = rtl8139_create_dev_data(dev);
	if (rtl8139 == NULL) {
		ddf_msg(LVL_ERROR, "Not enough memory for initializing %s.", ddf_dev_get_name(dev));
		return ENOMEM;
	}

	ddf_msg(LVL_DEBUG, "rtl8139: dev_data created");
	rtl8139->parent_sess = ddf_dev_parent_sess_get(dev);
	if (rtl8139->parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Error connecting parent device.");
		return EIO;
	}

	/* Obtain and fill hardware resources info and connect to parent */
	ret = rtl8139_get_resource_info(dev);
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Can not obatin hw resources information");
		goto failed;
	}

	ddf_msg(LVL_DEBUG, "rtl8139: resource_info obtained");

	/* Allocate DMA buffers */
	ret = rtl8139_buffers_create(rtl8139);
	if (ret != EOK)
		goto failed;

	/* Set default frame acceptance */
	rtl8139->rcr_data.ucast_mask = RTL8139_RCR_UCAST_DEFAULT;
	rtl8139->rcr_data.mcast_mask = RTL8139_RCR_MCAST_DEFAULT;
	rtl8139->rcr_data.bcast_mask = RTL8139_RCR_BCAST_DEFAULT;
	rtl8139->rcr_data.defect_mask = RTL8139_RCR_DEFECT_DEFAULT;
	/* Set receiver early treshold to 8/16 of frame length */
	rtl8139->rcr_data.rcr_base = (0x8 << RCR_ERTH_SHIFT);

	ddf_msg(LVL_DEBUG, "The device is initialized");
	return ret;
	
failed:
	ddf_msg(LVL_ERROR, "The device initialization failed");
	rtl8139_dev_cleanup(dev);
	return ret;
}

/** Enable the i/o ports of the device.
 *
 * @param dev  The RTL8139 device.
 *
 * @return EOK if successed, error code otherwise
 */
static errno_t rtl8139_pio_enable(ddf_dev_t *dev)
{
	ddf_msg(LVL_DEBUG, NAME ": rtl8139_pio_enable %s", ddf_dev_get_name(dev));

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_dev(dev));

	/* Gain control over port's registers. */
	if (pio_enable(rtl8139->io_addr, RTL8139_IO_SIZE, &rtl8139->io_port)) {
		ddf_msg(LVL_ERROR, "Cannot gain the port %p for device %s.", rtl8139->io_addr,
		    ddf_dev_get_name(dev));
		return EADDRNOTAVAIL;
	}

	return EOK;
}

/** Initialize the driver private data according to the
 *  device registers
 *
 *  @param rtl8139 rtl8139 private data
 */
static void rtl8139_data_init(rtl8139_t *rtl8139)
{
	assert(rtl8139);

	/* Check the version id */
	uint32_t tcr = pio_read_32(rtl8139->io_port + TCR);
	uint32_t hwverid = RTL8139_HWVERID(tcr);
	size_t i = 0;
	rtl8139->hw_version = RTL8139_VER_COUNT;
	for (i = 0; i < RTL8139_VER_COUNT; ++i) {
		if (rtl8139_versions[i].hwverid == 0)
			break;

		if (rtl8139_versions[i].hwverid == hwverid) {
			rtl8139->hw_version = rtl8139_versions[i].ver_id;
			ddf_msg(LVL_NOTE, "HW version found: index %zu, ver_id %d (%s)", i,
			    rtl8139_versions[i].ver_id, model_names[rtl8139->hw_version]);
		}
	}
}

/** The add_device callback of RTL8139 callback
 *
 * Probe and initialize the newly added device.
 *
 * @param dev  The RTL8139 device.
 *
 * @return EOK if added successfully, error code otherwise
 */
errno_t rtl8139_dev_add(ddf_dev_t *dev)
{
	ddf_fun_t *fun;

	ddf_msg(LVL_NOTE, "RTL8139_dev_add %s (handle = %zu)",
	    ddf_dev_get_name(dev), ddf_dev_get_handle(dev));

	/* Init device structure for rtl8139 */
	errno_t rc = rtl8139_device_initialize(dev);
	if (rc != EOK)
		return rc;

	/* Map I/O ports */
	rc = rtl8139_pio_enable(dev);
	if (rc != EOK)
		goto err_destroy;

	nic_t *nic_data = nic_get_from_ddf_dev(dev);
	rtl8139_t *rtl8139 = nic_get_specific(nic_data);

	nic_address_t addr;
	rtl8139_hw_get_addr(rtl8139, &addr);
	rc = nic_report_address(nic_data, &addr);
	if (rc != EOK)
		goto err_pio;

	/* Initialize the driver private structure */
	rtl8139_data_init(rtl8139);

	/* Register interrupt handler */
	int irq_cap;
	rc = rtl8139_register_int_handler(nic_data, &irq_cap);
	if (rc != EOK) {
		goto err_pio;
	}

	fun = ddf_fun_create(nic_get_ddf_dev(nic_data), fun_exposed, "port0");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating device function");
		goto err_srv;
	}

	nic_set_ddf_fun(nic_data, fun);
	ddf_fun_set_ops(fun, &rtl8139_dev_ops);

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
	unregister_interrupt_handler(dev, irq_cap);
err_pio:
	// rtl8139_pio_disable(dev);
	/* TODO: find out if the pio_disable is needed */
err_destroy:
	rtl8139_dev_cleanup(dev);
	return rc;
};

/** Set card MAC address
 *
 *  @param device   The RTL8139 device
 *  @param address  The place to store the address
 *  @param max_len  Maximal addresss length to store
 *
 *  @return EOK if succeed, error code otherwise
 */
static errno_t rtl8139_set_addr(ddf_fun_t *fun, const nic_address_t *addr)
{
	assert(fun);
	assert(addr);

	nic_t *nic_data =nic_get_from_ddf_fun((fun));
	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	rtl8139_lock_all(rtl8139);

	errno_t rc = nic_report_address(nic_data, addr);
	if ( rc != EOK) {
		rtl8139_unlock_all(rtl8139);
		return rc;
	}

	rtl8139_hw_set_addr(rtl8139, addr);

	rtl8139_unlock_all(rtl8139);
	return EOK;
}

/** Get the device information
 *
 *  @param dev   The NIC device
 *  @param info  The information to fill
 *
 *  @return EOK
 */
static errno_t rtl8139_get_device_info(ddf_fun_t *fun, nic_device_info_t *info)
{
	assert(fun);
	assert(info);

	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	assert(nic_data);
	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	/* TODO: fill the information more completely */
	info->vendor_id = 0x10ec;
	str_cpy(info->vendor_name, NIC_VENDOR_MAX_LENGTH, "Realtek");

	if (rtl8139->hw_version < RTL8139_VER_COUNT) {
		str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH, 
		    model_names[rtl8139->hw_version]);
	} else {
		str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH, "RTL8139");
	}

	info->ethernet_support[ETH_10M] = ETH_10BASE_T;
	info->ethernet_support[ETH_100M] = ETH_100BASE_TX;

	info->autoneg_support = RTL8139_AUTONEG_CAPS;
	return EOK;
}

/** Check the cable state
 *
 *  @param[in]  dev    The device
 *  @param[out] state  The state to fill
 *
 *  @return EOK
 */
static errno_t rtl8139_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state)
{
	assert(fun);
	assert(state);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	if (pio_read_16(rtl8139->io_port + CSCR) & CS_CON_STATUS) {
		*state = NIC_CS_PLUGGED;
	} else {
		*state = NIC_CS_UNPLUGGED;
	}

	return EOK;
}

/** Get operation mode of the device
 */
static errno_t rtl8139_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	assert(fun);
	assert(speed);
	assert(duplex);
	assert(role);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	uint16_t bmcr_val = pio_read_16(rtl8139->io_port + BMCR);
	uint8_t msr_val = pio_read_8(rtl8139->io_port + MSR);

	if (bmcr_val & BMCR_DUPLEX) {
		*duplex = NIC_CM_FULL_DUPLEX;
	} else {
		*duplex = NIC_CM_HALF_DUPLEX;
	}

	if (msr_val & MSR_SPEED10) {
		*speed = 10;
	} else {
		*speed = 100;
	}

	*role = NIC_ROLE_UNKNOWN;
	return EOK;
}

/** Value validity */
enum access_mode {
	/** Value is invalid now */
	VALUE_INVALID = 0,
	/** Read-only */
	VALUE_RO,
	/** Read-write */
	VALUE_RW
};

/** Check if pause frame operations are valid in current situation 
 *
 *  @param rtl8139  RTL8139 private structure
 *
 *  @return VALUE_INVALID if the value has no sense in current moment
 *  @return VALUE_RO if the state is read-only in current moment
 *  @return VALUE_RW if the state can be modified
 */
static int rtl8139_pause_is_valid(rtl8139_t *rtl8139)
{
	assert(rtl8139);

	uint16_t bmcr = pio_read_16(rtl8139->io_port + BMCR);
	if ((bmcr & (BMCR_AN_ENABLE | BMCR_DUPLEX)) == 0)
		return VALUE_INVALID;

	if (bmcr & BMCR_AN_ENABLE) {
		uint16_t anar_lp = pio_read_16(rtl8139->io_port + ANLPAR);
		if (anar_lp & ANAR_PAUSE)
			return VALUE_RO;
	}

	return VALUE_RW;
}

/** Get current pause frame configuration
 *
 *  Values are filled with NIC_RESULT_NOT_AVAILABLE if the value has no sense in
 *  the moment (half-duplex).
 *
 *  @param[in]  fun         The DDF structure of the RTL8139
 *  @param[out] we_send     Sign if local constroller sends pause frame
 *  @param[out] we_receive  Sign if local constroller receives pause frame
 *  @param[out] time        Time filled in pause frames. 0xFFFF in rtl8139
 *
 *  @return EOK if succeed
 */
static errno_t rtl8139_pause_get(ddf_fun_t *fun, nic_result_t *we_send, 
    nic_result_t *we_receive, uint16_t *time)
{
	assert(fun);
	assert(we_send);
	assert(we_receive);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	if (rtl8139_pause_is_valid(rtl8139) == VALUE_INVALID) {
		*we_send = NIC_RESULT_NOT_AVAILABLE;
		*we_receive = NIC_RESULT_NOT_AVAILABLE;
		*time = 0;
		return EOK;
	}

	uint8_t msr = pio_read_8(rtl8139->io_port + MSR);

	*we_send = (msr & MSR_TXFCE) ? NIC_RESULT_ENABLED : NIC_RESULT_DISABLED;
	*we_receive = (msr & MSR_RXFCE) ? NIC_RESULT_ENABLED : NIC_RESULT_DISABLED;
	*time = RTL8139_PAUSE_VAL;

	return EOK;
};

/** Set current pause frame configuration
 *
 *  @param fun            The DDF structure of the RTL8139
 *  @param allow_send     Sign if local constroller sends pause frame
 *  @param allow_receive  Sign if local constroller receives pause frames
 *  @param time           Time to use, ignored (not supported by device)
 *
 *  @return EOK if succeed, INVAL if the pause frame has no sence
 */
static errno_t rtl8139_pause_set(ddf_fun_t *fun, int allow_send, int allow_receive, 
    uint16_t time)
{
	assert(fun);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	if (rtl8139_pause_is_valid(rtl8139) != VALUE_RW)
		return EINVAL;
	
	uint8_t msr = pio_read_8(rtl8139->io_port + MSR);
	msr &= ~(uint8_t)(MSR_TXFCE | MSR_RXFCE);

	if (allow_receive)
		msr |= MSR_RXFCE;
	if (allow_send)
		msr |= MSR_TXFCE;
	
	pio_write_8(rtl8139->io_port + MSR, msr);

	if (allow_send && time > 0) {
		ddf_msg(LVL_WARN, "Time setting is not supported in set_pause method.");
	}
	return EOK;
};

/** Set operation mode of the device
 *
 */
static errno_t rtl8139_set_operation_mode(ddf_fun_t *fun, int speed,
    nic_channel_mode_t duplex, nic_role_t role)
{
	assert(fun);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	if (speed != 10 && speed != 100)
		return EINVAL;
	if (duplex != NIC_CM_HALF_DUPLEX && duplex != NIC_CM_FULL_DUPLEX)
		return EINVAL;

	uint16_t bmcr_val = pio_read_16(rtl8139->io_port + BMCR);

	/* Set autonegotion disabled */
	bmcr_val &= ~(uint16_t)BMCR_AN_ENABLE;

	if (duplex == NIC_CM_FULL_DUPLEX) {
		bmcr_val |= BMCR_DUPLEX;
	} else {
		bmcr_val &= ~((uint16_t)BMCR_DUPLEX);
	}

	if (speed == 100) {
		bmcr_val |= BMCR_Spd_100;
	} else {
		bmcr_val &= ~((uint16_t)BMCR_Spd_100);
	}

	rtl8139_regs_unlock(rtl8139->io_port);
	pio_write_16(rtl8139->io_port + BMCR, bmcr_val);
	rtl8139_regs_lock(rtl8139->io_port);
	return EOK;
}

/** Enable autonegoation with specific advertisement
 *
 *  @param dev            The device to update
 *  @param advertisement  The advertisement to set
 *
 *  @returns EINVAL if the advertisement mode is not supproted
 *  @returns EOK if advertisement mode set successfully
 */
static errno_t rtl8139_autoneg_enable(ddf_fun_t *fun, uint32_t advertisement)
{
	assert(fun);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	if (advertisement == 0) {
		advertisement = RTL8139_AUTONEG_CAPS;
	}

	if ((advertisement | RTL8139_AUTONEG_CAPS) != RTL8139_AUTONEG_CAPS)
		return EINVAL; /* some unsuported mode is requested */
	
	assert(advertisement != 0);

	/* Set the autonegotiation advertisement */
	uint16_t anar = ANAR_SELECTOR; /* default selector */
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

	uint16_t bmcr_val = pio_read_16(rtl8139->io_port + BMCR);
	bmcr_val |= BMCR_AN_ENABLE;

	pio_write_16(rtl8139->io_port + ANAR, anar);

	rtl8139_regs_unlock(rtl8139->io_port);
	pio_write_16(rtl8139->io_port + BMCR, bmcr_val);
	rtl8139_regs_lock(rtl8139->io_port);
	return EOK;
}

/** Disable advertisement functionality
 *
 *  @param dev  The device to update
 *
 *  @returns EOK
 */
static errno_t rtl8139_autoneg_disable(ddf_fun_t *fun)
{
	assert(fun);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	uint16_t bmcr_val = pio_read_16(rtl8139->io_port + BMCR);

	bmcr_val &= ~((uint16_t)BMCR_AN_ENABLE);

	rtl8139_regs_unlock(rtl8139->io_port);
	pio_write_16(rtl8139->io_port + BMCR, bmcr_val);
	rtl8139_regs_lock(rtl8139->io_port);

	return EOK;
}

/** Obtain the advertisement NIC framework value from the ANAR/ANLPAR register
 * value
 *
 *  @param[in]  anar           The ANAR register value
 *  @param[out] advertisement  The advertisement result
 */
static void rtl8139_get_anar_state(uint16_t anar, uint32_t *advertisement)
{
	*advertisement = 0;
	if (anar & ANAR_10_HD)
		*advertisement |= ETH_AUTONEG_10BASE_T_HALF;
	if (anar & ANAR_10_FD)
		*advertisement |= ETH_AUTONEG_10BASE_T_FULL;
	if (anar & ANAR_100TX_HD)
		*advertisement |= ETH_AUTONEG_100BASE_TX_HALF;
	if (anar & ANAR_100TX_FD)
		*advertisement |= ETH_AUTONEG_100BASE_TX_FULL;
	if (anar & ANAR_100T4)
		*advertisement |= ETH_AUTONEG_100BASE_T4_HALF;
	if (anar & ANAR_PAUSE)
		*advertisement |= ETH_AUTONEG_PAUSE_SYMETRIC;
}

/** Check the autonegotion state
 *
 *  @param[in]  dev            The device to check
 *  @param[out] advertisement  The device advertisement
 *  @param[out] their_adv      The partners advertisement
 *  @param[out] result         The autonegotion state
 *  @param[out] their_result   The link partner autonegotion status
 *
 *  @returns EOK
 */
static errno_t rtl8139_autoneg_probe(ddf_fun_t *fun, uint32_t *advertisement,
    uint32_t *their_adv, nic_result_t *result, nic_result_t *their_result)
{
	assert(fun);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	uint16_t bmcr = pio_read_16(rtl8139->io_port + BMCR);
	uint16_t anar = pio_read_16(rtl8139->io_port + ANAR);
	uint16_t anar_lp = pio_read_16(rtl8139->io_port + ANLPAR);
	uint16_t aner = pio_read_16(rtl8139->io_port + ANER);

	if (bmcr & BMCR_AN_ENABLE) {
		*result = NIC_RESULT_ENABLED;
	} else {
		*result = NIC_RESULT_DISABLED;
	}

	if (aner & ANER_LP_NW_ABLE) {
		*their_result = NIC_RESULT_ENABLED;
	} else {
		*their_result = NIC_RESULT_DISABLED;
	}

	rtl8139_get_anar_state(anar, advertisement);
	rtl8139_get_anar_state(anar_lp, their_adv);

	return EOK;
}

/** Restart autonegotiation process
 *
 *  @param dev  The device to update
 *
 *  @returns EOK
 */
static errno_t rtl8139_autoneg_restart(ddf_fun_t *fun)
{
	assert(fun);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	uint16_t bmcr = pio_read_16(rtl8139->io_port + BMCR);
	bmcr |= BMCR_AN_RESTART;
	bmcr |= BMCR_AN_ENABLE;

	rtl8139_regs_unlock(rtl8139);
	pio_write_16(rtl8139->io_port + BMCR, bmcr);
	rtl8139_regs_lock(rtl8139);

	return EOK;
}

/** Notify NIC framework about HW filtering state when promisc mode was disabled
 *
 *  @param nic_data     The NIC data
 *  @param mcast_mode   Current multicast mode
 *  @param was_promisc  Sign if the promiscuous mode was active before disabling
 */
inline static void rtl8139_rcx_promics_rem(nic_t *nic_data,
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

/** Set unicast frames acceptance mode
 *
 *  @param nic_data  The nic device to update
 *  @param mode      The mode to set
 *  @param addr      Ignored, no HW support, just use unicast promisc
 *  @param addr_cnt  Ignored, no HW support
 *
 *  @returns EOK
 */
static errno_t rtl8139_unicast_set(nic_t *nic_data, nic_unicast_mode_t mode,
    const nic_address_t *addr, size_t addr_cnt)
{
	assert(nic_data);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	uint8_t was_promisc = rtl8139->rcr_data.ucast_mask & RCR_ACCEPT_ALL_PHYS;

	nic_multicast_mode_t mcast_mode;
	nic_query_multicast(nic_data, &mcast_mode, 0, NULL, NULL);

	switch (mode) {
	case NIC_UNICAST_BLOCKED:
		rtl8139->rcr_data.ucast_mask = 0;
		rtl8139_rcx_promics_rem(nic_data, mcast_mode, was_promisc);
		break;
	case NIC_UNICAST_DEFAULT:
		rtl8139->rcr_data.ucast_mask = RCR_ACCEPT_PHYS_MATCH;
		rtl8139_rcx_promics_rem(nic_data, mcast_mode, was_promisc);
		break;
	case NIC_UNICAST_LIST:
		rtl8139->rcr_data.ucast_mask = RCR_ACCEPT_PHYS_MATCH
		    | RCR_ACCEPT_ALL_PHYS;

		if (mcast_mode == NIC_MULTICAST_PROMISC)
			nic_report_hw_filtering(nic_data, 0, 1, -1);
		else
			nic_report_hw_filtering(nic_data, 0, 0, -1);
		break;
	case NIC_UNICAST_PROMISC:
		rtl8139->rcr_data.ucast_mask = RCR_ACCEPT_PHYS_MATCH
		    | RCR_ACCEPT_ALL_PHYS;

		if (mcast_mode == NIC_MULTICAST_PROMISC)
			nic_report_hw_filtering(nic_data, 1, 1, -1);
		else
			nic_report_hw_filtering(nic_data, 1, 0, -1);
		break;
	default:
		return ENOTSUP;
	}
	fibril_mutex_lock(&rtl8139->rx_lock);
	rtl8139_hw_update_rcr(rtl8139);
	fibril_mutex_unlock(&rtl8139->rx_lock);
	return EOK;
}

/** Set multicast frames acceptance mode
 *
 *  @param nic_data  The nic device to update
 *  @param mode      The mode to set
 *  @param addr      Addresses to accept
 *  @param addr_cnt  Addresses count
 *
 *  @returns EOK
 */
static errno_t rtl8139_multicast_set(nic_t *nic_data, nic_multicast_mode_t mode,
    const nic_address_t *addr, size_t addr_count)
{
	assert(nic_data);
	assert(addr_count == 0 || addr);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	switch (mode) {
	case NIC_MULTICAST_BLOCKED:
		rtl8139->rcr_data.mcast_mask = 0;
		if ((rtl8139->rcr_data.ucast_mask & RCR_ACCEPT_ALL_PHYS) != 0)
			nic_report_hw_filtering(nic_data, -1, 0, -1);
		else
			nic_report_hw_filtering(nic_data, -1, 1, -1);
		break;
	case NIC_MULTICAST_LIST:
		rtl8139_hw_set_mcast_mask(rtl8139, nic_mcast_hash(addr, addr_count));
		rtl8139->rcr_data.mcast_mask = RCR_ACCEPT_MULTICAST;
		nic_report_hw_filtering(nic_data, -1, 0, -1);
		break;
	case NIC_MULTICAST_PROMISC:
		rtl8139_hw_set_mcast_mask(rtl8139, RTL8139_MCAST_MASK_PROMISC);
		rtl8139->rcr_data.mcast_mask = RCR_ACCEPT_MULTICAST;
		nic_report_hw_filtering(nic_data, -1, 1, -1);
		break;
	default:
		return ENOTSUP;
	}
	fibril_mutex_lock(&rtl8139->rx_lock);
	rtl8139_hw_update_rcr(rtl8139);
	fibril_mutex_unlock(&rtl8139->rx_lock);
	return EOK;
}

/** Set broadcast frames acceptance mode
 *
 *  @param nic_data  The nic device to update
 *  @param mode      The mode to set
 *
 *  @returns EOK
 */
static errno_t rtl8139_broadcast_set(nic_t *nic_data, nic_broadcast_mode_t mode)
{
	assert(nic_data);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	switch (mode) {
	case NIC_BROADCAST_BLOCKED:
		rtl8139->rcr_data.bcast_mask = 0;
		break;
	case NIC_BROADCAST_ACCEPTED:
		rtl8139->rcr_data.bcast_mask = RCR_ACCEPT_BROADCAST;
		break;
	default:
		return ENOTSUP;
	}
	fibril_mutex_lock(&rtl8139->rx_lock);
	rtl8139_hw_update_rcr(rtl8139);
	fibril_mutex_unlock(&rtl8139->rx_lock);
	return EOK;
}

/** Get state of acceptance of weird frames
 *
 *  @param[in]  device  The device to check
 *  @param[out] mode    The current mode
 */
static errno_t rtl8139_defective_get_mode(ddf_fun_t *fun, uint32_t *mode)
{
	assert(fun);
	assert(mode);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	*mode = 0;
	if (rtl8139->rcr_data.defect_mask & RCR_ACCEPT_ERROR)
		*mode |= NIC_DEFECTIVE_BAD_CRC;
	if (rtl8139->rcr_data.defect_mask & RCR_ACCEPT_RUNT)
		*mode |= NIC_DEFECTIVE_SHORT;

	return EOK;
};

/** Set acceptance of weird frames
 *
 *  @param device  The device to update
 *  @param mode    The mode to set
 *
 *  @returns ENOTSUP if the mode is not supported
 *  @returns EOK of mode was set
 */
static errno_t rtl8139_defective_set_mode(ddf_fun_t *fun, uint32_t mode)
{
	assert(fun);

	rtl8139_t *rtl8139 = nic_get_specific(nic_get_from_ddf_fun(fun));
	assert(rtl8139);

	if ((mode & (NIC_DEFECTIVE_SHORT | NIC_DEFECTIVE_BAD_CRC)) != mode)
		return ENOTSUP;

	rtl8139->rcr_data.defect_mask = 0;
	if (mode & NIC_DEFECTIVE_SHORT)
		rtl8139->rcr_data.defect_mask |= RCR_ACCEPT_RUNT;
	if (mode & NIC_DEFECTIVE_BAD_CRC)
		rtl8139->rcr_data.defect_mask |= RCR_ACCEPT_ERROR;

	fibril_mutex_lock(&rtl8139->rx_lock);
	rtl8139_hw_update_rcr(rtl8139);
	fibril_mutex_unlock(&rtl8139->rx_lock);
	return EOK;
};


/** Turn Wakeup On Lan method on
 *
 *  @param nic_data  The NIC to update
 *  @param virtue    The method to turn on
 *
 *  @returns EINVAL if the method is not supported
 *  @returns EOK if succeed
 *  @returns ELIMIT if no more methods of this kind can be enabled
 */
static errno_t rtl8139_wol_virtue_add(nic_t *nic_data,
	const nic_wol_virtue_t *virtue)
{
	assert(nic_data);
	assert(virtue);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	switch(virtue->type) {
	case NIC_WV_BROADCAST:
		rtl8139_hw_reg_add_8(rtl8139, CONFIG5, CONFIG5_BROADCAST_WAKEUP);
		break;
	case NIC_WV_LINK_CHANGE:
		rtl8139_regs_unlock(rtl8139->io_port);
		rtl8139_hw_reg_add_8(rtl8139, CONFIG3, CONFIG3_LINK_UP);
		rtl8139_regs_lock(rtl8139->io_port);
		break;
	case NIC_WV_MAGIC_PACKET:
		if (virtue->data)
			return EINVAL;
		rtl8139_regs_unlock(rtl8139->io_port);
		rtl8139_hw_reg_add_8(rtl8139, CONFIG3, CONFIG3_MAGIC);
		rtl8139_regs_lock(rtl8139->io_port);
		break;
	default:
		return EINVAL;
	};
	if(rtl8139->pm.active++ == 0)
		rtl8139_hw_pmen_set(rtl8139, 1);
	return EOK;
}

/** Turn Wakeup On Lan method off
 *
 *  @param nic_data  The NIC to update
 *  @param virtue    The method to turn off
 */
static void rtl8139_wol_virtue_rem(nic_t *nic_data,
	const nic_wol_virtue_t *virtue)
{
	assert(nic_data);
	assert(virtue);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	switch(virtue->type) {
	case NIC_WV_BROADCAST:
		rtl8139_hw_reg_rem_8(rtl8139, CONFIG5, CONFIG5_BROADCAST_WAKEUP);
		break;
	case NIC_WV_LINK_CHANGE:
		rtl8139_regs_unlock(rtl8139->io_port);
		rtl8139_hw_reg_rem_8(rtl8139, CONFIG3, CONFIG3_LINK_UP);
		rtl8139_regs_lock(rtl8139->io_port);
		break;
	case NIC_WV_MAGIC_PACKET:
		rtl8139_regs_unlock(rtl8139->io_port);
		rtl8139_hw_reg_rem_8(rtl8139, CONFIG3, CONFIG3_MAGIC);
		rtl8139_regs_lock(rtl8139->io_port);
		break;
	default:
		return;
	};
	rtl8139->pm.active--;
	if (rtl8139->pm.active == 0)
		rtl8139_hw_pmen_set(rtl8139, 0);
}


/** Set polling mode
 *
 *  @param device  The device to set
 *  @param mode    The mode to set
 *  @param period  The period for NIC_POLL_PERIODIC
 *
 *  @returns EOK if succeed
 *  @returns ENOTSUP if the mode is not supported
 */
static errno_t rtl8139_poll_mode_change(nic_t *nic_data, nic_poll_mode_t mode,
    const struct timeval *period)
{
	assert(nic_data);
	errno_t rc = EOK;

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	fibril_mutex_lock(&rtl8139->rx_lock);

	switch(mode) {
	case NIC_POLL_IMMEDIATE:
		rtl8139->int_mask = RTL_DEFAULT_INTERRUPTS;
		break;
	case NIC_POLL_ON_DEMAND:
		rtl8139->int_mask = 0;
		break;
	case NIC_POLL_PERIODIC:
		assert(period);

		rtl8139_timer_act_t new_timer;
		rc = rtl8139_timer_act_init(&new_timer, RTL8139_PCI_FREQ_KHZ, period);
		if (rc != EOK)
			break;

		/* Disable timer interrupts while working with timer-related data */
		rtl8139->int_mask = 0;
		rtl8139_hw_int_set(rtl8139);

		rtl8139->poll_timer = new_timer;
		rtl8139->int_mask = INT_TIME_OUT;

		/* Force timer interrupt start be writing nonzero value to timer 
		 * interrutp register (should be small to prevent big delay)
		 * Read TCTR to reset timer counter
		 * Change values to simulate the last interrupt from the period.
		 */
		pio_write_32(rtl8139->io_port + TIMERINT, 10);
		pio_write_32(rtl8139->io_port + TCTR, 0);

		ddf_msg(LVL_DEBUG, "Periodic mode. Interrupt mask %" PRIx16 ", "
		    "poll.full_skips %zu, last timer %" PRIu32,
		    rtl8139->int_mask, rtl8139->poll_timer.full_skips,
		    rtl8139->poll_timer.last_val);
		break;
	default:
		rc = ENOTSUP;
		break;
	}

	rtl8139_hw_int_set(rtl8139);

	fibril_mutex_unlock(&rtl8139->rx_lock);

	return rc;
}

/** Force receiving all frames in the receive buffer
 *
 *  @param device  The device to receive
 */
static void rtl8139_poll(nic_t *nic_data)
{
	assert(nic_data);

	rtl8139_t *rtl8139 = nic_get_specific(nic_data);
	assert(rtl8139);

	uint16_t isr = pio_read_16(rtl8139->io_port + ISR);
	pio_write_16(rtl8139->io_port + ISR, 0);

	rtl8139_interrupt_impl(nic_data, isr);
}


/** Main function of RTL8139 driver
 *
 *  Just initialize the driver structures and
 *  put it into the device drivers interface
 */
int main(void)
{
	printf("%s: HelenOS RTL8139 network adapter driver\n", NAME);

	errno_t rc = nic_driver_init(NAME);
	if (rc != EOK)
		return rc;

	nic_driver_implement(&rtl8139_driver_ops, &rtl8139_dev_ops,
	    &rtl8139_nic_iface);

	ddf_log_init(NAME);
	return ddf_driver_main(&rtl8139_driver);
}
