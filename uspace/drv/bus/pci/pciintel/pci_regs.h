/*
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup pciintel
 * @{
 */
/** @file
 */

#ifndef PCI_REGS_H_
#define PCI_REGS_H_

/* Header types */
#define PCI_HEADER_TYPE_DEV	0
#define PCI_HEADER_TYPE_BRIDGE	1
#define PCI_HEADER_TYPE_CARDBUS	2

/* Header type 0 and 1 */
#define PCI_VENDOR_ID		0x00
#define PCI_DEVICE_ID		0x02
#define PCI_COMMAND		0x04
#define PCI_STATUS		0x06
#define PCI_REVISION_ID		0x08
#define PCI_PROG_IF		0x09
#define PCI_SUB_CLASS		0x0A
#define PCI_BASE_CLASS		0x0B
#define PCI_CACHE_LINE_SIZE	0x0C
#define PCI_LATENCY_TIMER	0x0D
#define PCI_HEADER_TYPE		0x0E
#define PCI_BIST		0x0F

#define PCI_BASE_ADDR_0		0x10
#define PCI_BASE_ADDR_1		0x14

/* Header type 0 */
#define PCI_BASE_ADDR_2			0x18
#define PCI_BASE_ADDR_3			0x1B
#define PCI_BASE_ADDR_4			0x20
#define PCI_BASE_ADDR_5			0x24

#define PCI_CARDBUS_CIS_PTR		0x28
#define PCI_SUBSYSTEM_VENDOR_ID		0x2C
#define PCI_SUBSYSTEM_ID		0x2E
#define PCI_EXP_ROM_BASE		0x30
#define PCI_CAP_PTR			0x34
#define PCI_INT_LINE			0x3C
#define PCI_INT_PIN			0x3D
#define PCI_MIN_GNT			0x3E
#define PCI_MAX_LAT			0x3F

/* Header type 1 */
#define PCI_BRIDGE_PRIM_BUS_NUM		0x18
#define PCI_BRIDGE_SEC_BUS_NUM		0x19
#define PCI_BRIDGE_SUBORD_BUS_NUM	0x1A
#define PCI_BRIDGE_SEC_LATENCY_TIMER	0x1B
#define PCI_BRIDGE_IO_BASE		0x1C
#define PCI_BRIDGE_IO_LIMIT		0x1D
#define PCI_BRIDGE_SEC_STATUS		0x1E
#define PCI_BRIDGE_MEMORY_BASE		0x20
#define PCI_BRIDGE_MEMORY_LIMIT		0x22
#define PCI_BRIDGE_PREF_MEMORY_BASE	0x24
#define PCI_BRIDGE_PREF_MEMORY_LIMIT	0x26
#define PCI_BRIDGE_PREF_MEMORY_BASE_UP	0x28
#define PCI_BRIDGE_PREF_MEMORY_LIMIT_UP	0x2C
#define PCI_BRIDGE_IO_BASE_UP		0x30
#define PCI_BRIDGE_IO_LIMIT_UP		0x32
#define PCI_BRIDGE_EXP_ROM_BASE		0x38
#define PCI_BRIDGE_INT_LINE		0x3C
#define PCI_BRIDGE_INT_PIN		0x3D
#define PCI_BRIDGE_CTL			0x3E

/* PCI command flags */
#define PCI_COMMAND_IO            0x001
#define PCI_COMMAND_MEMORY        0x002
#define PCI_COMMAND_MASTER        0x004
#define PCI_COMMAND_SPECIAL       0x008
#define PCI_COMMAND_INVALIDATE    0x010
#define PCI_COMMAND_VGA_PALETTE   0x020
#define PCI_COMMAND_PARITY        0x040
#define PCI_COMMAND_WAIT          0x080
#define PCI_COMMAND_SERR          0x100
#define PCI_COMMAND_FAST_BACK     0x200
#define PCI_COMMAND_INTX_DISABLE  0x400

#endif

/**
 * @}
 */
