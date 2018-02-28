/*
 * Copyright (c) 2012 Petr Jerman
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

/** @file
 * Header for AHCI driver.
 */

#ifndef __AHCI_H__
#define __AHCI_H__

#include <async.h>
#include <ddf/interrupt.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "ahci_hw.h"

/** AHCI Device. */
typedef struct {
	/** Pointer to ddf device. */
	ddf_dev_t *dev;

	/** Pointer to AHCI memory registers. */
	volatile ahci_memregs_t *memregs;

	/** Pointers to sata devices. */
	void *sata_devs[AHCI_MAX_PORTS];

	/** Parent session */
	async_sess_t *parent_sess;
} ahci_dev_t;

/** SATA Device. */
typedef struct {
	/** Pointer to AHCI device. */
	ahci_dev_t *ahci;

	/** Pointer to ddf function. */
	ddf_fun_t *fun;

	/** SATA port number (0-31). */
	uint8_t port_num;

	/** Device in invalid state (disconnected and so on). */
	bool is_invalid_device;

	/** Pointer to SATA port. */
	volatile ahci_port_t *port;

	/** Pointer to command header. */
	volatile ahci_cmdhdr_t *cmd_header;

	/** Pointer to command table. */
	volatile uint32_t *cmd_table;

	/** Mutex for single operation on device. */
	fibril_mutex_t lock;

	/** Mutex for event signaling condition variable. */
	fibril_mutex_t event_lock;

	/** Event signaling condition variable. */
	fibril_condvar_t event_condvar;

	/** Event interrupt state. */
	ahci_port_is_t event_pxis;

	/** Number of device data blocks. */
	uint64_t blocks;

	/** Size of device data blocks. */
	size_t block_size;

	/** Name of SATA device. */
	char model[STR_BOUNDS(40) + 1];

	/** Device in invalid state (disconnected and so on). */
	bool is_packet_device;

	/** Highest UDMA mode supported. */
	uint8_t highest_udma_mode;
} sata_dev_t;

#endif
