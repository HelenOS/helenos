/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup uspace_drv_cuda_adb
 * @{
 */
/** @file
 */

#ifndef CUDA_ADB_H_
#define CUDA_ADB_H_

#include <adt/list.h>
#include <async.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <loc.h>
#include <stdint.h>
#include "cuda_hw.h"

enum {
	CUDA_RCV_BUF_SIZE = 5
};

enum cuda_xfer_state {
	cx_listen,
	cx_receive,
	cx_rcv_end,
	cx_send_start,
	cx_send
};

typedef struct {
	uintptr_t base;
	int irq;
} cuda_res_t;

/** ADB bus device */
typedef struct {
	ddf_fun_t *fun;
	async_sess_t *client_sess;
	link_t lcuda;
	struct cuda *cuda;
} adb_dev_t;

/** CUDA ADB bus */
typedef struct cude {
	struct cuda_regs *regs;
	uintptr_t phys_base;
	ddf_dev_t *dev;

	uint8_t rcv_buf[CUDA_RCV_BUF_SIZE];
	uint8_t snd_buf[CUDA_RCV_BUF_SIZE];
	size_t bidx;
	size_t snd_bytes;
	enum cuda_xfer_state xstate;
	fibril_mutex_t dev_lock;

	list_t devs;
	adb_dev_t *addr_dev[ADB_MAX_ADDR];
} cuda_t;

extern errno_t cuda_add(cuda_t *, cuda_res_t *);
extern errno_t cuda_remove(cuda_t *);
extern errno_t cuda_gone(cuda_t *);

#endif

/** @}
 */
