/*
 * Copyright (C) 2006 Martin Decky
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

/** @addtogroup xen32
 * @{
 */
/** @file
 */

#ifndef __xen32_BOOT_H__
#define __xen32_BOOT_H__

#define GUEST_CMDLINE	1024
#define VIRT_CPUS		32
#define START_INFO_SIZE	1104

#define BOOT_OFFSET		0x0000
#define TEMP_STACK_SIZE 0x1000

#define XEN_VIRT_START	0xFC000000
#define XEN_CS			0xe019

#ifndef __ASM__

#define mp_map ((pfn_t *) XEN_VIRT_START)

#include <arch/types.h>

typedef uint32_t evtchn_t;

typedef struct {
	uint32_t version;
	uint32_t pad0;
	uint64_t tsc_timestamp;   /**< TSC at last update of time vals */
	uint64_t system_time;     /**< Time, in nanosecs, since boot */
	uint32_t tsc_to_system_mul;
	int8_t tsc_shift;
	int8_t pad1[3];
} vcpu_time_info_t;

typedef struct {
	uint32_t cr2;
	uint32_t pad[5];
} arch_vcpu_info_t;

typedef struct arch_shared_info {
	pfn_t max_pfn;                  /**< max pfn that appears in table */
	uint32_t pfn_to_mfn_frame_list_list;
    uint32_t nmi_reason;
} arch_shared_info_t;

typedef struct {
	uint8_t evtchn_upcall_pending;
	uint8_t evtchn_upcall_mask;
	evtchn_t evtchn_pending_sel;
	arch_vcpu_info_t arch;
	vcpu_time_info_t time;
} vcpu_info_t;

typedef struct {
	vcpu_info_t vcpu_info[VIRT_CPUS];
	evtchn_t evtchn_pending[32];
	evtchn_t evtchn_mask[32];
	
	uint32_t wc_version;                  /**< Version counter */
	uint32_t wc_sec;                      /**< Secs  00:00:00 UTC, Jan 1, 1970 */
	uint32_t wc_nsec;                     /**< Nsecs 00:00:00 UTC, Jan 1, 1970 */
	
	arch_shared_info_t arch;
} shared_info_t;

typedef struct {
	int8_t magic[32];           /**< "xen-<version>-<platform>" */
	uint32_t frames;            /**< Available frames */
	shared_info_t *shared_info; /**< Shared info structure (machine address) */
	uint32_t flags;             /**< SIF_xxx flags */
	pfn_t store_mfn;            /**< Shared page (machine page) */
	evtchn_t store_evtchn;      /**< Event channel for store communication */
	void *console_mfn;          /**< Console page (machine address) */
	evtchn_t console_evtchn;    /**< Event channel for console messages */
	pte_t *ptl0;                /**< Boot PTL0 (kernel address) */
	uint32_t pt_frames;         /**< Number of bootstrap page table frames */
	pfn_t *pm_map;              /**< Physical->machine frame map (kernel address) */
	void *mod_start;            /**< Modules start (kernel address) */
	uint32_t mod_len;           /**< Modules size (bytes) */
	int8_t cmd_line[GUEST_CMDLINE];
} start_info_t;

typedef struct {
	pfn_t start;
	pfn_t size;
	pfn_t reserved;
} memzone_t;

extern start_info_t start_info;
extern shared_info_t shared_info;
extern memzone_t meminfo;

#endif

#endif

/** @}
 */
