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

#ifndef RTL8139_DRIVER_H_
#define RTL8139_DRIVER_H_

#include <stddef.h>
#include <stdint.h>
#include "defs.h"
#include "general.h"

/** The driver name */
#define NAME  "rtl8139"

/** Transmittion buffers count */
#define TX_BUFF_COUNT  4

/** Size of buffer for one frame (2kB) */
#define TX_BUFF_SIZE  (2 * 1024)

/** Number of pages to allocate for TxBuffers */
#define TX_PAGES  2

/** Size of the CRC after the received frame in the receiver buffer */
#define RTL8139_CRC_SIZE  4

/** The default mode of accepting unicast frames */
#define RTL8139_RCR_UCAST_DEFAULT  RCR_ACCEPT_PHYS_MATCH

/** The default mode of accepting multicast frames */
#define RTL8139_RCR_MCAST_DEFAULT  0

/** The default mode of accepting broadcast frames */
#define RTL8139_RCR_BCAST_DEFAULT  RCR_ACCEPT_BROADCAST

/** The default mode of accepting defect frames */
#define RTL8139_RCR_DEFECT_DEFAULT  0

/** Mask for accepting all multicast */
#define RTL8139_MCAST_MASK_PROMISC  UINT64_MAX

/** Data */
struct rtl8139_rcr_data {
	/** Configuration part of RCR */
	uint32_t rcr_base;
	/** Mask of unicast  */
	uint8_t ucast_mask;
	/** Mask of multicast */
	uint8_t mcast_mask;
	/** Mask of broadcast */
	uint8_t bcast_mask;
	/** Mask of defective */
	uint8_t defect_mask;
};

/** Power management related data */
typedef struct rtl8139_pm {
	/** Count of used activities which needs PMEn bit set */
	int active;
} rtl8139_pm_t;

/** RTL8139 device data */
typedef struct rtl8139_data {
	/** DDF device */
	ddf_dev_t *dev;
	/** Parent session */
	async_sess_t *parent_sess;
	/** I/O address of the device */
	void *io_addr;
	/** Mapped I/O port */
	void *io_port;
	/** The irq assigned */
	int irq;

	/** Mask of the turned interupts (IMR value) */
	uint16_t int_mask;

	/** The memory allocated for the transmittion buffers
	 *  Each buffer takes 2kB
	 */
	uintptr_t tx_buff_phys;
	void *tx_buff_virt;

	/** Virtual addresses of the Tx buffers */
	void *tx_buff[TX_BUFF_COUNT];

	/** The nubmer of the next buffer to use, index = tx_next % TX_BUFF_COUNT */
	size_t tx_next;
	/** The number of the first used buffer in the row
	 *
	 *  tx_used is in the interval tx_next - TX_BUFF_COUNT and tx_next:
	 *  	tx_next - TX_BUFF_COUNT: there is no useable Tx descriptor
	 *  	tx_next: all Tx descriptors are can be used
	 */
	size_t tx_used;

	/** Buffer for receiving frames */
	uintptr_t rx_buff_phys;
	void *rx_buff_virt;

	/** Receiver control register data */
	struct rtl8139_rcr_data rcr_data;

	/** Power management information */
	rtl8139_pm_t pm;

	/** Lock for receiver */
	fibril_mutex_t rx_lock;
	/** Lock for transmitter */
	fibril_mutex_t tx_lock;

	/** Polling mode information */
	rtl8139_timer_act_t poll_timer;

	/** Backward pointer to nic_data */
	nic_t *nic_data;

	/** Version of RT8139 controller */
	rtl8139_version_id_t hw_version;
} rtl8139_t;

/*
 * Pointers casting - for both amd64 and ia32
 */

/** Cast pointer to uint32_t
 *
 *  @param ptr The pointer to cast
 *  @return The uint32_t pointer representation. The low 32 bit is taken
 *  in the case of the 64 bit pointers
 */
#define PTR2U32(ptr) ((uint32_t)((size_t)(ptr)))

/** Check if the pointer can be cast to uint32_t without the data lost
 *
 *  @param ptr The pointer to check
 *  @return The true value if the pointer can be cast, false if not
 */
#define PTR_IS_32(ptr) ((size_t)PTR2U32(ptr) == (size_t)(ptr))

/** Cast the ioaddr part to the void*
 *
 *  @param ioaddr The ioaddr value
 */
#define IOADDR_TO_PTR(ioaddr) ((void*)((size_t)(ioaddr)))

/* ***** Bit operation macros ***** */

/** Set the bits specified by the given bit mask to the different values
 *
 * The bit from the src is used if the corresponding bit in the mask is 0,
 * the bit from the value is used if the corresponding bit in the mask is 1
 *
 * @param src The original value
 * @param value The new bit value
 * @param mask The mask to specify modified bits
 * @param type The values type
 *
 * @return New value
 */
#define bit_set_part_g(src, value, mask, type) \
	((type)(((src) & ~((type)(mask))) | ((value) & (type)(mask))))

/** Set the bits specified by the given bit mask to the different values
 *
 * The version of the uint32_t
 *
 * @see bit_set_part_g
 */

#define bit_set_part_32(src, value, mask) bit_set_part_g(src, value, mask, uint32_t)
/** Set the bits specified by the given bit mask to the different values
 *
 * The version of the uint16_t
 *
 * @see bit_set_part_g
 */

#define bit_set_part_16(src, value, mask) bit_set_part_g(src, value, mask, uint16_t)

/** Set the bits specified by the given bit mask to the different values
 *
 * The version of the uint8_t
 *
 * @see bit_set_part_g
 */
#define bit_set_part_8(src, value, mask) bit_set_part_g(src, value, mask, uint8_t)

/** Clear specified bits in the value
 *
 * @param src Original value
 * @param clear_mask Bits to clear mask
 * @param type The values type
 */
#define bit_clear_g(src, clear_mask, type) ((type)((src) & ~((type)(clear_mask))))

/** Clear specified bits in the value, 32bit version
 *
 *  @see bit_clear_g
 */
#define bit_clear_32(src, clear_mask) bit_clear_g(src, clear_mask, uint32_t)
/** Clear specified bits in the value, 16bit version
 *
 *  @see bit_clear_g
 */
#define bit_clear_16(src, clear_mask) bit_clear_g(src, clear_mask, uint16_t)
/** Clear specified bits in the value, 8bit version
 *
 *  @see bit_clear_g
 */
#define bit_clear_8(src, clear_mask) bit_clear_g(src, clear_mask, uint8_t)

/** Obtain value of the TSD register with size part modified
 *
 *  @param tsd_value Old value of the TSD
 *  @param size The size to set
 */
#define rtl8139_tsd_set_size(tsd_value, size) \
	bit_set_part_32(tsd_value, (size) << TSD_SIZE_SHIFT, TSD_SIZE_MASK << TSD_SIZE_SHIFT)

#endif
