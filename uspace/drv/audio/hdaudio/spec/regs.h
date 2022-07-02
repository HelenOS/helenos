/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio register interface
 */

#ifndef SPEC_REGS_H
#define SPEC_REGS_H

#include <stdint.h>

/** Stream Descriptor registers */
typedef struct {
	/** Control 1 */
	uint8_t ctl1;
	/** Control 2 */
	uint8_t ctl2;
	/** Control 3 */
	uint8_t ctl3;
	/** Status */
	uint8_t sts;
	/** Link Position in Current Buffer */
	uint32_t lpib;
	/** Cyclic Buffer Length */
	uint32_t cbl;
	/** Last Valid Index */
	uint16_t lvi;
	/** Reserved */
	uint8_t reserved1[2];
	/** FIFO Size */
	uint16_t fifod;
	/** Format */
	uint16_t fmt;
	/** Reserved */
	uint8_t reserved2[4];
	/** Buffer Descriptor List Pointer - Lower */
	uint32_t bdpl;
	/** Buffer Descriptor List Pointer - Upper */
	uint32_t bdpu;
} hda_sdesc_regs_t;

typedef struct {
	/** Global Capabilities */
	uint16_t gcap;
	/** Minor Version */
	uint8_t vmin;
	/** Major Version */
	uint8_t vmaj;
	/** Output Payload Capability */
	uint16_t outpay;
	/** Input Payload Capability */
	uint16_t inpay;
	/** Global Control */
	uint32_t gctl;
	/** Wake Enable */
	uint16_t wakeen;
	/** State Change Status */
	uint16_t statests;
	/** Global Status */
	uint16_t gsts;
	/** Reserved */
	uint8_t reserved1[6];
	/** Output Stream Payload Capability */
	uint16_t outstrmpay;
	/** Input Stream Payload Capability */
	uint16_t instrmpay;
	/** Reserved */
	uint8_t reserved2[4];
	/** Interrupt Control */
	uint32_t intctl;
	/** Interrupt Status */
	uint32_t intsts;
	/** Reserved */
	uint8_t reserved3[8];
	/** Wall Clock Counter */
	uint32_t walclk;
	/** Reserved */
	uint8_t reserved4[4];
	/** Stream Synchronization */
	uint32_t ssync;
	/** Reserved */
	uint8_t reserved5[4];
	/** CORB Lower Base Address */
	uint32_t corblbase;
	/** CORB Upper Base Address */
	uint32_t corbubase;
	/** CORB Write Pointer */
	uint16_t corbwp;
	/** CORB Read Pointer */
	uint16_t corbrp;
	/** CORB Control */
	uint8_t corbctl;
	/** CORB Status */
	uint8_t corbsts;
	/** CORB Size */
	uint8_t corbsize;
	/** Reserved */
	uint8_t reserved6[1];
	/** RIRB Lower Base Address */
	uint32_t rirblbase;
	/** RIRB Upper Base Address */
	uint32_t rirbubase;
	/** RIRB Write Pointer */
	uint16_t rirbwp;
	/** Response Interrupt Count */
	uint16_t rintcnt;
	/** RIRB Control */
	uint8_t rirbctl;
	/** RIRB Status */
	uint8_t rirbsts;
	/** RIRB Size */
	uint8_t rirbsize;
	/** Reserved */
	uint8_t reserved7[1];
	/** Immediate Command Output Interface */
	uint32_t icoi;
	/** Immediate Command Input Interface */
	uint32_t icii;
	/** Immediate Command Status */
	uint16_t icis;
	/** Reserved */
	uint8_t reserved8[6];
	/** DMA Position Buffer Lower Base */
	uint32_t dplbase;
	/** DMA Position Buffer Upper Base */
	uint32_t dpubase;
	/** Reserved */
	uint8_t reserved9[8];
	/** Stream descriptor registers */
	hda_sdesc_regs_t sdesc[64];
	/** Fill up to 0x2030 */
	uint8_t reserved10[6064];
	/** Wall Clock Counter Alias */
	uint32_t walclka;
	/** Stream Descriptor Link Position in Current Buffer */
	uint32_t sdlpiba[64];
} hda_regs_t;

/** Stream Descriptor Control bits */
typedef enum {
	/** Descriptor Error Interrupt Enable */
	sdctl1_deie = 4,
	/** FIFO Error Interrupt Enable */
	sdctl1_feie = 3,
	/** Interrupt on Completion Enable */
	sdctl1_ioce = 2,
	/** Stream Run */
	sdctl1_run = 1,
	/** Stream Reset */
	sdctl1_srst = 0
} hda_sdesc_ctl1_bits;

/** Stream Descriptor Status bits */
typedef enum {
	/** FIFO Ready */
	sdctl1_fifordy = 3,
	/** Descriptor Error */
	sdsts_dese = 2,
	/** FIFO Error */
	sdsts_fifoe = 1,
	/** Buffer Completion Interrupt Status */
	sdsts_bcis = 2
} hda_sdesc_sts_bits;

typedef enum {
	/** Number of Output Streams Supported (H) */
	gcap_oss_h = 15,
	/** Number of Output Streams Supported (L) */
	gcap_oss_l = 12,
	/** Number of Input Streams Supported (H) */
	gcap_iss_h = 11,
	/** Number of Input Streams Supported (L) */
	gcap_iss_l = 8,
	/** Number of Bidirectional Streams Supported (H) */
	gcap_bss_h = 7,
	/** Number of Bidirectional Streams Supported (L) */
	gcap_bss_l = 3,
	/** Number of Serial Data Out Signals (H) */
	gcap_nsdo_h = 2,
	/** Number of Serial Data Out Signals (H) */
	gcap_nsdo_l = 1,
	/** 64 Bit Address Supported */
	gcap_64ok = 0
} hda_gcap_bits_t;

typedef enum {
	/** Accept Unsolicited Response Enable */
	gctl_unsol = 8,
	/** Flush Control */
	gctl_fcntrl = 1,
	/** Controller Reset */
	gctl_crst = 0
} hda_gctl_bits_t;

typedef enum {
	/** Global Interrupt Enable */
	intctl_gie = 31,
	/** Controller Interrupt Enable */
	intctl_cie = 30,
	/** Stream Interrupt Enable */
	intctl_sie = 29
} hda_intctl_bits_t;

typedef enum {
	/** CORB Read Pointer Reset */
	corbrp_rst = 15,
	/** CORB Read Pointer (H) */
	corbrp_rp_h = 7,
	/** CORB Read Pointer (L) */
	corbrp_rp_l = 0
} hda_corbrp_bits_t;

typedef enum {
	/** CORB Write Pointer (H) */
	corbwp_wp_h = 7,
	/** CORB Write Pointer (L) */
	corbwp_wp_l = 0
} hda_corbwp_bits_t;

typedef enum {
	/** Enable CORB DMA Engine */
	corbctl_run = 1,
	/** CORB Memory Error Interrupt Enable */
	corbctl_meie = 0
} hda_corbctl_bits_t;

typedef enum {
	/** CORB Size Capability (H) */
	corbsize_cap_h = 7,
	/** CORB Size Capability (L) */
	corbsize_cap_l = 4,
	/** CORB Size (H) */
	corbsize_size_h = 1,
	/** CORB Size (L) */
	corbsize_size_l = 0
} hda_corbsize_bits_t;

typedef enum {
	/** RIRB Write Pointer Reset */
	rirbwp_rst = 15,
	/** RIRB Write Pointer (H) */
	rirbwp_wp_h = 7,
	/** RIRB Write Pointer (L) */
	rirbwp_wp_l = 0
} hda_rirbwp_bits_t;

typedef enum {
	/** Response Overrun Interrupt Control */
	rirbctl_oic = 2,
	/** RIRB DMA Enable */
	rirbctl_run = 1,
	/** CORB Memory Error Interrupt Enable */
	rirbctl_int = 0
} hda_rirbctl_bits_t;

typedef enum {
	/** Response Overrun Interrupt Status */
	rirbsts_ois = 2,
	/** Response Interrupt */
	rirbsts_intfl = 0
} hda_rirbsts_bits_t;

typedef enum {
	/** RIRB Size Capability (H) */
	rirbsize_cap_h = 7,
	/** RIRB Size Capability (L) */
	rirbsize_cap_l = 4,
	/** RIRB Size (H) */
	rirbsize_size_h = 1,
	/** RIRB Size (L) */
	rirbsize_size_l = 0
} hda_rirbsize_bits_t;

typedef struct {
	/** Response - data received from codec */
	uint32_t resp;
	/** Response Extended - added by controller */
	uint32_t respex;
} hda_rirb_entry_t;

typedef enum {
	/** Unsolicited response */
	respex_unsol = 4,
	/** Codec Address (H) */
	respex_addr_h = 3,
	/** Codec Address (L) */
	respex_addr_l = 0
} hda_respex_bits_t;

#endif

/** @}
 */
