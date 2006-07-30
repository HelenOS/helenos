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
#define START_INFO_SIZE	1104
#define BOOT_OFFSET		0x0000

#ifndef __ASM__

#include <arch/types.h>

typedef struct {
	int8_t magic[32];           /**< "xen-<version>-<platform>" */
	uint32_t frames;            /**< Available frames */
	void *shared_info;          /**< Shared info structure (machine address) */
	uint32_t flags;             /**< SIF_xxx flags */
	pfn_t store_mfn;            /**< Shared page (machine page) */
	uint32_t store_evtchn;      /**< Event channel for store communication */
	void *console_mfn;          /**< Console page (machine address) */
	uint32_t console_evtchn;    /**< Event channel for console messages */
	pte_t *ptl0;                /**< Boot PTL0 (kernel address) */
	uint32_t pt_frames;         /**< Number of bootstrap page table frames */
	pfn_t *pm_map;              /**< Physical->machine frame map (kernel address) */
	void *mod_start;            /**< Modules start (kernel address) */
	uint32_t mod_len;           /**< Modules size (bytes) */
	int8_t cmd_line[GUEST_CMDLINE];
} start_info_t;

extern start_info_t start_info;

#endif

#endif

/** @}
 */
