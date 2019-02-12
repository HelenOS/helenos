/*
 * Copyright (c) 2019 Jakub Jermar
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
