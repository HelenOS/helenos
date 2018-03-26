/*
 * Copyright (c) 2013 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI host controller register structure
 */
#ifndef DRV_EHCI_EHCI_REGS_H
#define DRV_EHCI_EHCI_REGS_H

#include <byteorder.h>
#include <macros.h>
#include <ddi.h>

#define EHCI_WR(reg, val) pio_write_32(&(reg), host2uint32_t_le(val))
#define EHCI_RD(reg) uint32_t_le2host(pio_read_32(&(reg)))
#define EHCI_RD8(reg) pio_read_8(&(reg))
#define EHCI_SET(reg, val) pio_set_32(&(reg), host2uint32_t_le(val), 10)
#define EHCI_CLR(reg, val) pio_clear_32(&(reg), host2uint32_t_le(val), 10)

/** EHCI memory mapped capability registers structure */
typedef struct ehci_cap_regs {
	const ioport8_t caplength;
	PADD8(1);
	const ioport16_t hciversion;
	const ioport32_t hcsparams;
#define EHCI_CAPS_HCS_DEBUG_PORT_MASK   0xf
#define EHCI_CAPS_HCS_DEBUG_PORT_SHIFT  (1 << 20)
#define EHCI_CAPS_HCS_INDICATORS_FLAG   (1 << 16)
#define EHCI_CAPS_HCS_N_CC_MASK         0xf
#define EHCI_CAPS_HCS_N_CC_SHIFT        12
#define EHCI_CAPS_HCS_N_PCC_MASK        0xf
#define EHCI_CAPS_HCS_N_PCC_SHIFT       8
#define EHCI_CAPS_HCS_ROUTING_FLAG      (1 << 7)
#define EHCI_CAPS_HCS_PPC_FLAG          (1 << 4)
#define EHCI_CAPS_HCS_N_PORTS_MASK      0xf
#define EHCI_CAPS_HCS_N_PORTS_SHIFT     0

	const ioport32_t hccparams;
#define EHCI_CAPS_HCC_EECP_MASK           0xff
#define EHCI_CAPS_HCC_EECP_SHIFT          8
#define EHCI_CAPS_HCC_ISO_THRESHOLD_MASK  0xf
#define EHCI_CAPS_HCC_ISO_THRESHOLD_SHIFT 4
#define EHCI_CAPS_HCC_ASYNC_PART_FLAG     (1 << 2)
#define EHCI_CAPS_HCC_PROG_FRAME_FLAG     (1 << 1)
#define EHCI_CAPS_HCC_64_FLAG             (1 << 0)
	ioport8_t hcsp_portoute[8];

} ehci_caps_regs_t;


/** EHCI memory mapped operational registers structure */
typedef struct ehci_regs {
	ioport32_t usbcmd;
#define USB_CMD_INT_THRESHOLD_MASK     0xff
#define USB_CMD_INT_THRESHOLD_SHIFT    16
#define USB_CMD_PARK_MODE_FLAG         (1 << 11)
#define USB_CMD_PARK_COUNT_MASK        0x3
#define USB_CMD_PARK_COUNT_SHIFT       8
#define USB_CMD_LIGHT_RESET            (1 << 7)
#define USB_CMD_IRQ_ASYNC_DOORBELL     (1 << 6)
#define USB_CMD_ASYNC_SCHEDULE_FLAG    (1 << 5)
#define USB_CMD_PERIODIC_SCHEDULE_FLAG (1 << 4)
#define USB_CMD_FRAME_LIST_SIZE_MASK   0x3
#define USB_CMD_FRAME_LIST_SIZE_SHIFT  2
#define USB_CMD_FRAME_LIST_SIZE_1024   0x0
#define USB_CMD_FRAME_LIST_SIZE_512    0x1
#define USB_CMD_FRAME_LIST_SIZE_256    0x2
#define USB_CMD_HC_RESET_FLAG          (1 << 1)
#define USB_CMD_RUN_FLAG               (1 << 0)

	ioport32_t usbsts;
#define USB_STS_ASYNC_SCHED_FLAG         (1 << 15)
#define USB_STS_PERIODIC_SCHED_FLAG      (1 << 14)
#define USB_STS_RECLAMATION_FLAG         (1 << 13)
#define USB_STS_HC_HALTED_FLAG           (1 << 12)
#define USB_STS_IRQ_ASYNC_ADVANCE_FLAG   (1 <<  5)
#define USB_STS_HOST_ERROR_FLAG          (1 <<  4)
#define USB_STS_FRAME_ROLLOVER_FLAG      (1 <<  3)
#define USB_STS_PORT_CHANGE_FLAG         (1 <<  2)
#define USB_STS_ERR_IRQ_FLAG             (1 <<  1)
#define USB_STS_IRQ_FLAG                 (1 <<  0)

	ioport32_t usbintr;
#define USB_INTR_ASYNC_ADVANCE_FLAG   (1 << 5)
#define USB_INTR_HOST_ERR_FLAG        (1 << 4)
#define USB_INTR_FRAME_ROLLOVER_FLAG  (1 << 3)
#define USB_INTR_PORT_CHANGE_FLAG     (1 << 2)
#define USB_INTR_ERR_IRQ_FLAG         (1 << 1)
#define USB_INTR_IRQ_FLAG             (1 << 0)

	ioport32_t frindex;
#define USB_FRINDEX_MASK   0xfff

	ioport32_t ctrldssegment;
	ioport32_t periodiclistbase;
#define USB_PERIODIC_LIST_BASE_MASK   0xfffff000

	ioport32_t asynclistaddr;
#define USB_ASYNCLIST_MASK   0xfffffff0

	PADD32(9);

	ioport32_t configflag;
#define USB_CONFIG_FLAG_FLAG   (1 << 0)

	ioport32_t portsc[];
#define USB_PORTSC_WKOC_E_FLAG       (1 << 22)
#define USB_PORTSC_WKDSCNNT_E_FLAG   (1 << 21)
#define USB_PORTSC_WKCNNT_E_FLAG     (1 << 20)
#define USB_PORTSC_PORT_TEST_MASK    (0xf << 16)
#define USB_PORTSC_NO_TEST           (0x0 << 16)
#define USB_PORTSC_TEST_J_STATE      (0x1 << 16)
#define USB_PORTSC_TEST_K_STATE      (0x2 << 16)
#define USB_PORTSC_TEST_SE0_NAK      (0x3 << 16)
#define USB_PORTSC_TEST_PACKET       (0x4 << 16)
#define USB_PORTSC_TEST_FORCE_ENABLE (0x5 << 16)
#define USB_PORTSC_INDICATOR_MASK    (0x3 << 14)
#define USB_PORTSC_INDICATOR_OFF     (0x0 << 14)
#define USB_PORTSC_INDICATOR_AMBER   (0x1 << 14)
#define USB_PORTSC_INDICATOR_GREEN   (0x2 << 14)
#define USB_PORTSC_PORT_OWNER_FLAG   (1 << 13)
#define USB_PORTSC_PORT_POWER_FLAG   (1 << 12)
#define USB_PORTSC_LINE_STATUS_MASK  (0x3 << 10)
#define USB_PORTSC_LINE_STATUS_SE0   (0x0 << 10)
#define USB_PORTSC_LINE_STATUS_K     (0x1 << 10)
#define USB_PORTSC_LINE_STATUS_J     (0x2 << 10)
#define USB_PORTSC_PORT_RESET_FLAG   (1 << 8)
#define USB_PORTSC_SUSPEND_FLAG      (1 << 7)
#define USB_PORTSC_RESUME_FLAG       (1 << 6)
#define USB_PORTSC_OC_CHANGE_FLAG    (1 << 5)
#define USB_PORTSC_OC_ACTIVE_FLAG    (1 << 4)
#define USB_PORTSC_EN_CHANGE_FLAG    (1 << 3)
#define USB_PORTSC_ENABLED_FLAG      (1 << 2)
#define USB_PORTSC_CONNECT_CH_FLAG   (1 << 1)
#define USB_PORTSC_CONNECT_FLAG      (1 << 0)

#define USB_PORTSC_WC_MASK \
    (USB_PORTSC_CONNECT_CH_FLAG | USB_PORTSC_EN_CHANGE_FLAG | USB_PORTSC_OC_CHANGE_FLAG)
} ehci_regs_t;

#endif
/**
 * @}
 */

