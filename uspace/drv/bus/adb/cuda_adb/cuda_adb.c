/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup genarch
 * @{
 */
/** @file VIA-CUDA Apple Desktop Bus driver
 *
 * Note: We should really do a full bus scan at the beginning and resolve
 * address conflicts. Also we should consider the handler ID in r3. Instead
 * we just assume a keyboard at address 2 or 8 and a mouse at address 9.
 */

#include <assert.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <ipc/adb.h>
#include <libarch/ddi.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cuda_adb.h"
#include "cuda_hw.h"

#define NAME  "cuda_adb"

static void cuda_dev_connection(ipc_callid_t, ipc_call_t *, void *);
static errno_t cuda_init(cuda_t *);
static void cuda_irq_handler(ipc_call_t *, void *);

static void cuda_irq_listen(cuda_t *);
static void cuda_irq_receive(cuda_t *);
static void cuda_irq_rcv_end(cuda_t *, void *, size_t *);
static void cuda_irq_send_start(cuda_t *);
static void cuda_irq_send(cuda_t *);

static void cuda_packet_handle(cuda_t *, uint8_t *, size_t);
static void cuda_send_start(cuda_t *);
static void cuda_autopoll_set(cuda_t *, bool);

static void adb_packet_handle(cuda_t *, uint8_t *, size_t, bool);

static irq_pio_range_t cuda_ranges[] = {
	{
		.base = 0,
		.size = sizeof(cuda_regs_t)
	}
};

static irq_cmd_t cuda_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,
		.dstarg = 1
	},
	{
		.cmd = CMD_AND,
		.value = SR_INT,
		.srcarg = 1,
		.dstarg = 2
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 1,
		.srcarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};


static irq_code_t cuda_irq_code = {
	sizeof(cuda_ranges) / sizeof(irq_pio_range_t),
	cuda_ranges,
	sizeof(cuda_cmds) / sizeof(irq_cmd_t),
	cuda_cmds
};

static errno_t cuda_dev_create(cuda_t *cuda, const char *name, const char *id,
    adb_dev_t **rdev)
{
	adb_dev_t *dev = NULL;
	ddf_fun_t *fun;
	errno_t rc;

	fun = ddf_fun_create(cuda->dev, fun_inner, name);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function '%s'.", name);
		rc = ENOMEM;
		goto error;
	}

	rc = ddf_fun_add_match_id(fun, id, 10);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding match ID.");
		rc = ENOMEM;
		goto error;
	}

	dev = ddf_fun_data_alloc(fun, sizeof(adb_dev_t));
	if (dev == NULL) {
		ddf_msg(LVL_ERROR, "Failed allocating memory for '%s'.", name);
		rc = ENOMEM;
		goto error;
	}

	dev->fun = fun;
	list_append(&dev->lcuda, &cuda->devs);

	ddf_fun_set_conn_handler(fun, cuda_dev_connection);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function '%s'.", name);
		goto error;
	}

	*rdev = dev;
	return EOK;
error:
	if (fun != NULL)
		ddf_fun_destroy(fun);
	return rc;
}

errno_t cuda_add(cuda_t *cuda, cuda_res_t *res)
{
	adb_dev_t *kbd = NULL;
	adb_dev_t *mouse = NULL;
	errno_t rc;

	cuda->phys_base = res->base;

	rc = cuda_dev_create(cuda, "kbd", "adb/keyboard", &kbd);
	if (rc != EOK)
		goto error;

	rc = cuda_dev_create(cuda, "mouse", "adb/mouse", &mouse);
	if (rc != EOK)
		goto error;

	cuda->addr_dev[2] = kbd;
	cuda->addr_dev[8] = kbd;

	cuda->addr_dev[9] = mouse;

	rc = cuda_init(cuda);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed initializing CUDA hardware.");
		return rc;
	}

	return EOK;
error:
	return rc;
}

errno_t cuda_remove(cuda_t *cuda)
{
	return ENOTSUP;
}

errno_t cuda_gone(cuda_t *cuda)
{
	return ENOTSUP;
}

/** Device connection handler */
static void cuda_dev_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	adb_dev_t *dev = (adb_dev_t *) ddf_fun_data_get((ddf_fun_t *) arg);
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	while (true) {
		callid = async_get_call(&call);
		method = IPC_GET_IMETHOD(call);

		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}

		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, &call);
		if (sess != NULL) {
			dev->client_sess = sess;
			async_answer_0(callid, EOK);
		} else {
			async_answer_0(callid, EINVAL);
		}
	}
}

static errno_t cuda_init(cuda_t *cuda)
{
	errno_t rc;

	void *vaddr;
	rc = pio_enable((void *) cuda->phys_base, sizeof(cuda_regs_t),
	    &vaddr);
	if (rc != EOK)
		return rc;

	cuda->regs = vaddr;
	cuda->xstate = cx_listen;
	cuda->bidx = 0;
	cuda->snd_bytes = 0;

	fibril_mutex_initialize(&cuda->dev_lock);

	/* Disable all interrupts from CUDA. */
	pio_write_8(&cuda->regs->ier, IER_CLR | ALL_INT);

	cuda_irq_code.ranges[0].base = (uintptr_t) cuda->phys_base;
	cuda_irq_code.cmds[0].addr = (void *) &((cuda_regs_t *)
	    cuda->phys_base)->ifr;
	async_irq_subscribe(10, cuda_irq_handler, cuda, &cuda_irq_code, NULL);

	/* Enable SR interrupt. */
	pio_write_8(&cuda->regs->ier, TIP | TREQ);
	pio_write_8(&cuda->regs->ier, IER_SET | SR_INT);

	/* Enable ADB autopolling. */
	cuda_autopoll_set(cuda, true);

	return EOK;
}

static void cuda_irq_handler(ipc_call_t *call, void *arg)
{
	uint8_t rbuf[CUDA_RCV_BUF_SIZE];
	cuda_t *cuda = (cuda_t *)arg;
	size_t len;
	bool handle;

	handle = false;
	len = 0;

	fibril_mutex_lock(&cuda->dev_lock);

	switch (cuda->xstate) {
	case cx_listen:
		cuda_irq_listen(cuda);
		break;
	case cx_receive:
		cuda_irq_receive(cuda);
		break;
	case cx_rcv_end:
		cuda_irq_rcv_end(cuda, rbuf, &len);
		handle = true;
		break;
	case cx_send_start:
		cuda_irq_send_start(cuda);
		break;
	case cx_send:
		cuda_irq_send(cuda);
		break;
	}

	/* Lower IFR.SR_INT so that CUDA can generate next int by raising it. */
	pio_write_8(&cuda->regs->ifr, SR_INT);

	fibril_mutex_unlock(&cuda->dev_lock);

	/* Handle an incoming packet. */
	if (handle)
		cuda_packet_handle(cuda, rbuf, len);
}

/** Interrupt in listen state.
 *
 * Start packet reception.
 *
 * @param cuda CUDA instance
 */
static void cuda_irq_listen(cuda_t *cuda)
{
	uint8_t b = pio_read_8(&cuda->regs->b);

	if ((b & TREQ) != 0) {
		ddf_msg(LVL_WARN, "cuda_irq_listen: no TREQ?!");
		return;
	}

	pio_write_8(&cuda->regs->b, b & ~TIP);
	cuda->xstate = cx_receive;
}

/** Interrupt in receive state.
 *
 * Receive next byte of packet.
 *
 * @param cuda CUDA instance
 */
static void cuda_irq_receive(cuda_t *cuda)
{
	uint8_t data = pio_read_8(&cuda->regs->sr);
	if (cuda->bidx < CUDA_RCV_BUF_SIZE)
		cuda->rcv_buf[cuda->bidx++] = data;

	uint8_t b = pio_read_8(&cuda->regs->b);

	if ((b & TREQ) == 0) {
		pio_write_8(&cuda->regs->b, b ^ TACK);
	} else {
		pio_write_8(&cuda->regs->b, b | TACK | TIP);
		cuda->xstate = cx_rcv_end;
	}
}

/** Interrupt in rcv_end state.
 *
 * Terminate packet reception. Either go back to listen state or start
 * receiving another packet if CUDA has one for us.
 *
 * @param cuda CUDA instance
 * @param buf Buffer for storing received packet
 * @param len Place to store length of received packet
 */
static void cuda_irq_rcv_end(cuda_t *cuda, void *buf, size_t *len)
{
	uint8_t b = pio_read_8(&cuda->regs->b);

	if ((b & TREQ) == 0) {
		cuda->xstate = cx_receive;
		pio_write_8(&cuda->regs->b, b & ~TIP);
	} else {
		cuda->xstate = cx_listen;
		cuda_send_start(cuda);
	}

	memcpy(buf, cuda->rcv_buf, cuda->bidx);
	*len = cuda->bidx;
	cuda->bidx = 0;
}

/** Interrupt in send_start state.
 *
 * Process result of sending first byte (and send second on success).
 *
 * @param cuda CUDA instance
 */
static void cuda_irq_send_start(cuda_t *cuda)
{
	uint8_t b;

	b = pio_read_8(&cuda->regs->b);

	if ((b & TREQ) == 0) {
		/* Collision */
		pio_write_8(&cuda->regs->acr, pio_read_8(&cuda->regs->acr) &
		    ~SR_OUT);
		pio_read_8(&cuda->regs->sr);
		pio_write_8(&cuda->regs->b, pio_read_8(&cuda->regs->b) |
		    TIP | TACK);
		cuda->xstate = cx_listen;
		return;
	}

	pio_write_8(&cuda->regs->sr, cuda->snd_buf[1]);
	pio_write_8(&cuda->regs->b, pio_read_8(&cuda->regs->b) ^ TACK);
	cuda->bidx = 2;

	cuda->xstate = cx_send;
}

/** Interrupt in send state.
 *
 * Send next byte or terminate transmission.
 *
 * @param cuda CUDA instance
 */
static void cuda_irq_send(cuda_t *cuda)
{
	if (cuda->bidx < cuda->snd_bytes) {
		/* Send next byte. */
		pio_write_8(&cuda->regs->sr,
		    cuda->snd_buf[cuda->bidx++]);
		pio_write_8(&cuda->regs->b, pio_read_8(&cuda->regs->b) ^ TACK);
		return;
	}

	/* End transfer. */
	cuda->snd_bytes = 0;
	cuda->bidx = 0;

	pio_write_8(&cuda->regs->acr, pio_read_8(&cuda->regs->acr) & ~SR_OUT);
	pio_read_8(&cuda->regs->sr);
	pio_write_8(&cuda->regs->b, pio_read_8(&cuda->regs->b) | TACK | TIP);

	cuda->xstate = cx_listen;
	/* TODO: Match reply with request. */
}

static void cuda_packet_handle(cuda_t *cuda, uint8_t *data, size_t len)
{
	if (data[0] != PT_ADB)
		return;
	if (len < 2)
		return;

	adb_packet_handle(cuda, data + 2, len - 2, (data[1] & 0x40) != 0);
}

static void adb_packet_handle(cuda_t *cuda, uint8_t *data, size_t size,
    bool autopoll)
{
	uint8_t dev_addr;
	uint8_t reg_no;
	uint16_t reg_val;
	adb_dev_t *dev;
	unsigned i;

	dev_addr = data[0] >> 4;
	reg_no = data[0] & 0x03;

	if (size != 3) {
		ddf_msg(LVL_WARN, "Unrecognized packet, size=%zu", size);
		for (i = 0; i < size; ++i) {
			ddf_msg(LVL_WARN, "  0x%02x", data[i]);
		}
		return;
	}

	if (reg_no != 0) {
		ddf_msg(LVL_WARN, "Unrecognized packet, size=%zu", size);
		for (i = 0; i < size; ++i) {
			ddf_msg(LVL_WARN, "  0x%02x", data[i]);
		}
		return;
	}

	reg_val = ((uint16_t) data[1] << 8) | (uint16_t) data[2];

	ddf_msg(LVL_DEBUG, "Received ADB packet for device address %d",
	    dev_addr);
	dev = cuda->addr_dev[dev_addr];
	if (dev == NULL)
		return;

	async_exch_t *exch = async_exchange_begin(dev->client_sess);
	async_msg_1(exch, ADB_REG_NOTIF, reg_val);
	async_exchange_end(exch);
}

static void cuda_autopoll_set(cuda_t *cuda, bool enable)
{
	cuda->snd_buf[0] = PT_CUDA;
	cuda->snd_buf[1] = CPT_AUTOPOLL;
	cuda->snd_buf[2] = enable ? 0x01 : 0x00;
	cuda->snd_bytes = 3;
	cuda->bidx = 0;

	cuda_send_start(cuda);
}

static void cuda_send_start(cuda_t *cuda)
{
	assert(cuda->xstate == cx_listen);

	if (cuda->snd_bytes == 0)
		return;

	/* Check for incoming data. */
	if ((pio_read_8(&cuda->regs->b) & TREQ) == 0)
		return;

	pio_write_8(&cuda->regs->acr, pio_read_8(&cuda->regs->acr) | SR_OUT);
	pio_write_8(&cuda->regs->sr, cuda->snd_buf[0]);
	pio_write_8(&cuda->regs->b, pio_read_8(&cuda->regs->b) & ~TIP);

	cuda->xstate = cx_send_start;
}

/** @}
 */
