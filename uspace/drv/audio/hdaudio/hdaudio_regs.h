/*
 * Copyright (c) 2014 Jiri Svoboda
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

#ifndef HDAUDIO_REGS_H
#define HDAUDIO_REGS_H

#include <sys/types.h>

/** Stream Descriptor registers */
typedef struct {
	/** Control */
	uint16_t ctl;
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
	uint32_t reserved4[4];
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
	uint8_t reserved5[1];
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
	uint8_t reserved6[1];
	/** Immediate Command Output Interface */
	uint32_t icoi;
	/** Immediate Command Input Interface */
	uint32_t icii;
	/** Immediate Command Status */
	uint32_t icis;
	/** Reserved */
	uint8_t reserved7[6];
	/** DMA Position Buffer Lower Base */
	uint32_t dpiblbase;
	/** DMA Position Buffer Upper Base */
	uint32_t dpibubase;
	/** Reserved */
	uint8_t reserved8[8];
	/** Stream descriptor registers */
	hda_sdesc_regs_t sdesc[64];
	/** Fill up to 0x2030 */
	uint8_t reserved9[6064];
	/** Wall Clock Counter Alias */
	uint32_t walclka;
	/** Stream Descriptor Link Position in Current Buffer */
	uint32_t sdlpiba[64];
} hda_regs_t;

#endif

/** @}
 */
