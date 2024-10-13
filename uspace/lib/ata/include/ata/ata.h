/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup libata
 * @{
 */
/** @file ATA driver library definitions.
 */

#ifndef LIBATA_ATA_H
#define LIBATA_ATA_H

#include <async.h>
#include <bd_srv.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <str.h>
#include <stdint.h>
#include <stddef.h>
#include "ata_hw.h"

struct ata_device;

/** ATA DMA direction */
typedef enum {
	/** DMA read */
	ata_dma_read,
	/** DMA write */
	ata_dma_write
} ata_dma_dir_t;

/** ATA channel creation parameters */
typedef struct {
	/** Argument to callback functions */
	void *arg;
	/** IRQ is available */
	bool have_irq;
	/** Use DMA transfers */
	bool use_dma;
	/** Maximum number of bytes that we can DMA in one I/O operation */
	size_t max_dma_xfer;
	/** Read 16 bits from the data port */
	void (*write_data_16)(void *, uint16_t *, size_t);
	/** Write 16 bits to the data port */
	void (*read_data_16)(void *, uint16_t *, size_t);
	/** Read 8 bits from an 8-bit command register */
	void (*write_cmd_8)(void *, uint16_t, uint8_t);
	/** Writes 8 bits to an 8-bit command register */
	uint8_t (*read_cmd_8)(void *, uint16_t);
	/** Read 8 bits from a control register */
	void (*write_ctl_8)(void *, uint16_t, uint8_t);
	/** Write 8 bits to a control register */
	uint8_t (*read_ctl_8)(void *, uint16_t);
	/** Enable interrupts */
	errno_t (*irq_enable)(void *);
	/** Disable interrupts */
	errno_t (*irq_disable)(void *);
	/** Set up DMA channel */
	void (*dma_chan_setup)(void *, void *, size_t, ata_dma_dir_t);
	/** Set up DMA channel */
	void (*dma_chan_teardown)(void *);
	/** Add new device */
	errno_t (*add_device)(void *, unsigned, void *);
	/** Remove device */
	errno_t (*remove_device)(void *, unsigned);
	/** Log notice message */
	void (*msg_note)(void *, char *);
	/** Log error message */
	void (*msg_error)(void *, char *);
	/** Log warning message */
	void (*msg_warn)(void *, char *);
	/** Log debug message */
	void (*msg_debug)(void *, char *);
} ata_params_t;

/** Timeout definitions. Unit is 10 ms. */
enum ata_timeout {
	TIMEOUT_PROBE	=  100, /*  1 s */
	TIMEOUT_BSY	=  100, /*  1 s */
	TIMEOUT_DRDY	= 1000  /* 10 s */
};

enum ata_dev_type {
	ata_reg_dev,	/* Register device (no packet feature set support) */
	ata_pkt_dev	/* Packet device (supports packet feature set). */
};

/** Register device block addressing mode. */
enum rd_addr_mode {
	am_chs,		/**< CHS block addressing */
	am_lba28,	/**< LBA-28 block addressing */
	am_lba48	/**< LBA-48 block addressing */
};

/** Block coordinates */
typedef struct {
	enum rd_addr_mode amode;

	union {
		/** CHS coordinates */
		struct {
			uint8_t sector;
			uint8_t cyl_lo;
			uint8_t cyl_hi;
		};
		/** LBA coordinates */
		struct {
			uint8_t c0;
			uint8_t c1;
			uint8_t c2;
			uint8_t c3;
			uint8_t c4;
			uint8_t c5;
		};
	};

	/** Lower 4 bits for device/head register */
	uint8_t h;
} block_coord_t;

/** ATA device state structure. */
typedef struct ata_device {
	bool present;
	struct ata_channel *chan;

	/** Device type */
	enum ata_dev_type dev_type;

	/** Addressing mode to use (if register device) */
	enum rd_addr_mode amode;

	/*
	 * Geometry. Only valid if operating in CHS mode.
	 */
	struct {
		unsigned heads;
		unsigned cylinders;
		unsigned sectors;
	} geom;

	uint64_t blocks;
	size_t block_size;

	char model[STR_BOUNDS(40) + 1];

	int device_id;
	bd_srvs_t bds;
} ata_device_t;

/** ATA channel */
typedef struct ata_channel {
	/** Parameters */
	ata_params_t params;

	/** Command registers */
	ata_cmd_t *cmd;
	/** Control registers */
	ata_ctl_t *ctl;

	/** Per-device state. */
	ata_device_t device[MAX_DEVICES];

	/** Synchronize channel access */
	fibril_mutex_t lock;
	/** Synchronize access to irq_fired/irq_status */
	fibril_mutex_t irq_lock;
	/** Signalled by IRQ handler */
	fibril_condvar_t irq_cv;
	/** Set to true when interrupt occurs */
	bool irq_fired;
	/** Value of status register read by interrupt handler */
	uint8_t irq_status;
} ata_channel_t;

extern errno_t ata_channel_create(ata_params_t *, ata_channel_t **);
extern errno_t ata_channel_initialize(ata_channel_t *);
extern errno_t ata_channel_destroy(ata_channel_t *);
extern void ata_channel_irq(ata_channel_t *, uint8_t);
extern void ata_connection(ipc_call_t *, ata_device_t *);

#endif

/** @}
 */
