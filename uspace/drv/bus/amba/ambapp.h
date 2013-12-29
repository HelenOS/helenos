/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2011 Jiri Svoboda
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup amba
 * @{
 */
/** @file
 */

#ifndef AMBAPP_H_
#define AMBAPP_H_

#include <ddf/driver.h>

#define AMBAPP_MAX_DEVICES     64
#define AMBAPP_AHBMASTER_AREA  0xfffff000
#define AMBAPP_AHBSLAVE_AREA   0xfffff800
#define AMBAPP_CONF_AREA       0xff000

#define AMBA_MAX_HW_RES  (4 + 1)

typedef enum {
	GAISLER = 1,
	ESA = 4
} amba_vendor_id_t;

typedef enum {
	GAISLER_LEON3    = 0x003,
	GAISLER_LEON3DSU = 0x004,
	GAISLER_ETHAHB   = 0x005,
	GAISLER_APBMST   = 0x006,
	GAISLER_AHBUART  = 0x007,
	GAISLER_SRCTRL   = 0x008,
	GAISLER_SDCTRL   = 0x009,
	GAISLER_APBUART  = 0x00c,
	GAISLER_IRQMP    = 0x00d,
	GAISLER_AHBRAM   = 0x00e,
	GAISLER_GPTIMER  = 0x011,
	GAISLER_PCITRG   = 0x012,
	GAISLER_PCISBRG  = 0x013,
	GAISLER_PCIFBRG  = 0x014,
	GAISLER_PCITRACE = 0x015,
	GAISLER_PCIDMA   = 0x016,
	GAISLER_AHBTRACE = 0x017,
	GAISLER_ETHDSU   = 0x018,
	GAISLER_PIOPORT  = 0x01a,
	GAISLER_AHBJTAG  = 0x01c,
	GAISLER_SPW      = 0x01f,
	GAISLER_ATACTRL  = 0x024,
	GAISLER_VGA      = 0x061,
	GAISLER_KBD      = 0x060,
	GAISLER_ETHMAC   = 0x01d,
	GAISLER_DDRSPA   = 0x025,
	GAISLER_EHCI     = 0x026,
	GAISLER_UHCI     = 0x027,
	GAISLER_SPW2     = 0x029,
	GAISLER_DDR2SPA  = 0x02e,
	GAISLER_AHBSTAT  = 0x052,
	GAISLER_FTMCTRL  = 0x054,
	ESA_MCTRL        = 0x00f
} amba_device_id_t;

typedef struct {
	unsigned int addr : 12;
	unsigned int reserved : 2;
	unsigned int prefetchable : 1;
	unsigned int cacheable : 1;
	unsigned int mask : 12;
	unsigned int type : 4;
} __attribute__((packed)) ambapp_bar_t;

typedef struct {
	unsigned int vendor_id : 8;
	unsigned int device_id : 24;
	unsigned int reserved : 2;
	unsigned int version : 5;
	unsigned int irq : 5;
	uint32_t user_defined[3];
	ambapp_bar_t bar[4];
} __attribute__((packed)) ambapp_entry_t;

#endif

/**
 * @}
 */
