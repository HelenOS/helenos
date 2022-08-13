/*
 * SPDX-FileCopyrightText: 2019 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VIRTIO_BLK_H_
#define _VIRTIO_BLK_H_

#include <virtio-pci.h>
#include <bd_srv.h>
#include <abi/cap.h>

#include <fibril_synch.h>

#define VIRTIO_BLK_BLOCK_SIZE	512

/* Operation types. */
#define VIRTIO_BLK_T_IN		0
#define VIRTIO_BLK_T_OUT	1

/* Status codes returned by the device. */
#define VIRTIO_BLK_S_OK		0
#define VIRTIO_BLK_S_IOERR	1
#define VIRTIO_BLK_S_UNSUPP	2

#define RQ_BUFFERS	32

/** Device is read-only. */
#define VIRTIO_BLK_F_RO		(1U << 5)

typedef struct {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
} virtio_blk_req_header_t;

typedef struct {
	uint8_t status;
} virtio_blk_req_footer_t;

typedef struct {
	uint64_t capacity;
} virtio_blk_cfg_t;

typedef struct {
	virtio_dev_t virtio_dev;

	void *rq_header[RQ_BUFFERS];
	uintptr_t rq_header_p[RQ_BUFFERS];

	void *rq_buf[RQ_BUFFERS];
	uintptr_t rq_buf_p[RQ_BUFFERS];

	void *rq_footer[RQ_BUFFERS];
	uintptr_t rq_footer_p[RQ_BUFFERS];

	uint16_t rq_free_head;

	int irq;
	cap_irq_handle_t irq_handle;

	bd_srvs_t bds;

	fibril_mutex_t free_lock;
	fibril_condvar_t free_cv;

	fibril_mutex_t completion_lock[RQ_BUFFERS];
	fibril_condvar_t completion_cv[RQ_BUFFERS];
} virtio_blk_t;

#endif
