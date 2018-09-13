/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup amdm37x
 * @{
 */
/** @file
 * @brief USBTLL IO register structure.
 */
#ifndef AMDM37x_USBTLL_H
#define AMDM37x_USBTLL_H

#include <macros.h>
#include <ddi.h>

#define AMDM37x_USBTLL_BASE_ADDRESS  0x48062000
#define AMDM37x_USBTLL_SIZE  4096

typedef struct {
	const ioport32_t revision;
#define TLL_REVISION_MINOR_MASK  0x0f
#define TLL_REVISION_MAJOR_MASK  0xf0

	PADD32(3);
	ioport32_t sysconfig;
#define TLL_SYSCONFIG_AUTOIDLE_FLAG  (1 << 0)
#define TLL_SYSCONFIG_SOFTRESET_FLAG  (1 << 1)
#define TLL_SYSCONFIG_ENWAKEUP_FLAG  (1 << 2)
#define TLL_SYSCONFIG_SIDLE_MODE_MASK  (0x3 << 3)
#define TLL_SYSCONFIG_SIDLE_MODE_FORCE  (0x0 << 3)
#define TLL_SYSCONFIG_SIDLE_MODE_NO  (0x1 << 3)
#define TLL_SYSCONFIG_SIDLE_MODE_SMART  (0x2 << 3)
#define TLL_SYSCONFIG_CLOCKACTIVITY_FLAG  (1 << 8)

	const ioport32_t sysstatus;
#define TLL_SYSSTATUS_RESET_DONE_FLAG  (1 << 0)

	const ioport32_t irqstatus;
#define TLL_IRQSTATUS_FCLK_START_FLAG  (1 << 0)
#define TLL_IRQSTATUS_FCLK_END_FLAG  (1 << 1)
#define TLL_IRQSTATUS_ACCESS_ERROR_FLAG  (1 << 2)

	ioport32_t irqenable;
#define TLL_IRQSTATUS_FCLK_START_EN_FLAG  (1 << 0)
#define TLL_IRQSTATUS_FCLK_END_EN_FLAG  (1 << 1)
#define TLL_IRQSTATUS_ACCESS_ERROR_EN_FLAG  (1 << 2)

	PADD32(4);
	ioport32_t shared_conf;
#define TLL_SHARED_CONF_FCLK_IS_ON_FLAG  (1 << 0)
#define TLL_SHARED_CONF_FCLK_REQ_FLAG  (1 << 1)
#define TLL_SHARED_CONF_USB_DIVRATIO_MASK  (0x7 << 2)
#define TLL_SHARED_CONF_USB_DIVRATIO(x)  (((x) & 0x7) << 2)
#define TLL_SHARED_CONF_USB_180D_SDR_EN_FLAG  (1 << 5)
#define TLL_SHARED_CONF_USB_90D_DDR_EN_FLAG  (1 << 6)

	PADD32(3);
	ioport32_t channel_conf[3];
#define TLL_CHANNEL_CONF_CHANEN_FLAG  (1 << 0)
#define TLL_CHANNEL_CONF_CHANMODE_MASK  (0x3 << 1)
#define TLL_CHANNEL_CONF_CHANMODE_UTMI_ULPI_MODE (0x0 << 1)
#define TLL_CHANNEL_CONF_CHANMODE_UTMI_SERIAL_MODE (0x1 << 1)
#define TLL_CHANNEL_CONF_CHANMODE_UTMI_TRANS_MODE (0x2 << 1)
#define TLL_CHANNEL_CONF_CHANMODE_NO_MODE (0x3 << 1)
#define TLL_CHANNEL_CONF_UTMIISADEV_FLAG  (1 << 3)
#define TLL_CHANNEL_CONF_TLLATTACH_FLAG  (1 << 4)
#define TLL_CHANNEL_CONF_TLLCONNECT_FLAG  (1 << 5)
#define TLL_CHANNEL_CONF_TLLFULLSPEED_FLAG  (1 << 6)
#define TLL_CHANNEL_CONF_ULPIOUTCLKMODE_FLAG  (1 << 7)
#define TLL_CHANNEL_CONF_ULPIDDRMODE_FLAG  (1 << 8)
#define TLL_CHANNEL_CONF_UTMIAUTOIDLE_FLAG  (1 << 9)
#define TLL_CHANNEL_CONF_ULPIAUTOIDLE_FLAG  (1 << 10)
#define TLL_CHANNEL_CONF_ULPINOBITSTUFF_FLAG  (1 << 11)
#define TLL_CHANNEL_CONF_CHRGVBUS_FLAG  (1 << 15)
#define TLL_CHANNEL_CONF_DRVVBUS_FLAG  (1 << 16)
#define TLL_CHANNEL_CONF_TESTEN_FLAG  (1 << 17)
#define TLL_CHANNEL_CONF_TESTTXEN_FLAG  (1 << 18)
#define TLL_CHANNEL_CONF_TESTTXDAT_FLAG  (1 << 19)
#define TLL_CHANNEL_CONF_TESTTXSE0_FLAG  (1 << 20)
#define TLL_CHANNEL_CONF_FSLSMODE_MASK   (0xf << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_6PIN_UNI_PHY_TX_DATSE0   (0x0 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_6PIN_UNI_PHY_TX_DPDM   (0x1 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_3PIN_BIDI_PHY   (0x2 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_4PIN_BIDI_PHY   (0x3 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_6PIN_UNI_TLL_TX_DATSE0  (0x4 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_6PIN_UNI_TLL_TX_DPDM  (0x5 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_3PIN_BIDI_TLL  (0x6 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_4PIN_BIDI_TLL  (0x7 << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_2PIN_BIDI_TLL_DATSE0  (0xa << 24)
#define TLL_CHANNEL_CONF_FSLSMODE_2PIN_BIDI_TLL_DPDM  (0xb << 24)

#define TLL_CHANNEL_CONF_FSLSLINESTATE_MASK  (0x3 << 28)
#define TLL_CHANNEL_CONF_FSLSLINESTATE_SE0  (0x0 << 28)
#define TLL_CHANNEL_CONF_FSLSLINESTATE_FS_J  (0x1 << 28)
#define TLL_CHANNEL_CONF_FSLSLINESTATE_FS_K  (0x2 << 28)
#define TLL_CHANNEL_CONF_FSLSLINESTATE_SE1  (0x3 << 28)

	/* The rest are 8bit ULPI registers */
} tll_regs_t;

#endif
/**
 * @}
 */
