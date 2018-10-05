/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_MPS_H_
#define KERN_ia32_MPS_H_

#include <stdint.h>
#include <synch/waitq.h>
#include <config.h>
#include <arch/smp/smp.h>

#define CT_EXT_ENTRY_TYPE  0
#define CT_EXT_ENTRY_LEN   1

struct mps_fs {
	uint32_t signature;
	uint32_t configuration_table;
	uint8_t length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t config_type;
	uint8_t mpfib2;
	uint8_t mpfib3;
	uint8_t mpfib4;
	uint8_t mpfib5;
} __attribute__((packed));

struct mps_ct {
	uint32_t signature;
	uint16_t base_table_length;
	uint8_t revision;
	uint8_t checksum;
	uint8_t oem_id[8];
	uint8_t product_id[12];
	uint32_t oem_table;
	uint16_t oem_table_size;
	uint16_t entry_count;
	uint32_t l_apic;
	uint16_t ext_table_length;
	uint8_t ext_table_checksum;
	uint8_t reserved;
	uint8_t base_table[0];
} __attribute__((packed));

struct __processor_entry {
	uint8_t type;
	uint8_t l_apic_id;
	uint8_t l_apic_version;
	uint8_t cpu_flags;
	uint8_t cpu_signature[4];
	uint32_t feature_flags;
	uint32_t reserved[2];
} __attribute__((packed));

struct __bus_entry {
	uint8_t type;
	uint8_t bus_id;
	uint8_t bus_type[6];
} __attribute__((packed));

struct __io_apic_entry {
	uint8_t type;
	uint8_t io_apic_id;
	uint8_t io_apic_version;
	uint8_t io_apic_flags;
	uint32_t io_apic;
} __attribute__((packed));

struct __io_intr_entry {
	uint8_t type;
	uint8_t intr_type;
	uint8_t poel;
	uint8_t reserved;
	uint8_t src_bus_id;
	uint8_t src_bus_irq;
	uint8_t dst_io_apic_id;
	uint8_t dst_io_apic_pin;
} __attribute__((packed));

struct __l_intr_entry {
	uint8_t type;
	uint8_t intr_type;
	uint8_t poel;
	uint8_t reserved;
	uint8_t src_bus_id;
	uint8_t src_bus_irq;
	uint8_t dst_l_apic_id;
	uint8_t dst_l_apic_pin;
} __attribute__((packed));

extern struct smp_config_operations mps_config_operations;

extern void mps_init(void);

#endif

/** @}
 */
