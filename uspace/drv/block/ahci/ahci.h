/*
 * SPDX-FileCopyrightText: 2012 Petr Jerman
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
