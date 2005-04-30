/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __MPS_H__
#define __MPS_H__

#include <arch/types.h>
#include <typedefs.h>
#include <synch/waitq.h>
#include <config.h>

#define CT_EXT_ENTRY_TYPE		0
#define CT_EXT_ENTRY_LEN		1

struct mps_fs {
	__u32 signature;
	struct mps_ct *configuration_table;
	__u8 length;
	__u8 revision;
	__u8 checksum;
	__u8 config_type;
	__u8 mpfib2;
	__u8 mpfib3;
	__u8 mpfib4;
	__u8 mpfib5;
} __attribute__ ((packed));

struct mps_ct {
	__u32 signature;
	__u16 base_table_length;
	__u8 revision;
	__u8 checksum;
	__u8 oem_id[8];
	__u8 product_id[12];
	__u8 *oem_table;
	__u16 oem_table_size;
	__u16 entry_count;
	__u32 *l_apic;
	__u16 ext_table_length;
	__u8 ext_table_checksum;
	__u8 xxx;
	__u8 base_table[0];
} __attribute__ ((packed));

struct __processor_entry {
	__u8 type;
	__u8 l_apic_id;
	__u8 l_apic_version;
	__u8 cpu_flags;
	__u8 cpu_signature[4];
	__u32 feature_flags;
	__u32 xxx[2];
} __attribute__ ((packed));

struct __bus_entry {
	__u8 type;
	__u8 bus_id;
	__u8 bus_type[6];
} __attribute__ ((packed));

struct __io_apic_entry {
	__u8 type;
	__u8 io_apic_id;
	__u8 io_apic_version;
	__u8 io_apic_flags;
	__u32 *io_apic;
} __attribute__ ((packed));

struct __io_intr_entry {
	__u8 type;
	__u8 intr_type;
	__u8 poel;
	__u8 xxx;
	__u8 src_bus_id;
	__u8 src_bus_irq;
	__u8 dst_io_apic_id;
	__u8 dst_io_apic_pin;
} __attribute__ ((packed));

struct __l_intr_entry {
	__u8 type;
	__u8 intr_type;
	__u8 poel;
	__u8 xxx;
	__u8 src_bus_id;
	__u8 src_bus_irq;
	__u8 dst_l_apic_id;
	__u8 dst_l_apic_pin;
} __attribute__ ((packed));


extern waitq_t ap_completion_wq;
extern waitq_t kmp_completion_wq;

extern int mps_irq_to_pin(int irq);

extern void mps_init(void);
extern void kmp(void *arg);

#endif
