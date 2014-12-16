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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <loc.h>
#include <sysinfo.h>
#include <errno.h>
#include <ipc/adb.h>
#include <async.h>
#include <assert.h>
#include "cuda_adb.h"

#define NAME  "cuda_adb"

static void cuda_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static int cuda_init(void);
static void cuda_irq_handler(ipc_callid_t iid, ipc_call_t *call, void *arg);

static void cuda_irq_listen(void);
static void cuda_irq_receive(void);
static void cuda_irq_rcv_end(void *buf, size_t *len);
static void cuda_irq_send_start(void);
static void cuda_irq_send(void);

static void cuda_packet_handle(uint8_t *buf, size_t len);
static void cuda_send_start(void);
static void cuda_autopoll_set(bool enable);

static void adb_packet_handle(uint8_t *data, size_t size, bool autopoll);


/** B register fields */
enum {
	TREQ	= 0x08,
	TACK	= 0x10,
	TIP	= 0x20
};

/** IER register fields */
enum {
	IER_CLR	= 0x00,
	IER_SET	= 0x80,

	SR_INT	= 0x04,
	ALL_INT	= 0x7f
};

/** ACR register fields */
enum {
	SR_OUT	= 0x10
};

/** Packet types */
enum {
	PT_ADB	= 0x00,
	PT_CUDA	= 0x01
};

/** CUDA packet types */
enum {
	CPT_AUTOPOLL	= 0x01
};

enum {
	ADB_MAX_ADDR	= 16
};

static irq_pio_range_t cuda_ranges[] = {
	{
		.base = 0,
		.size = sizeof(cuda_t)
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

static cuda_instance_t cinst;

static cuda_instance_t *instance = &cinst;
static cuda_t *dev;

static adb_dev_t adb_dev[ADB_MAX_ADDR];

int main(int argc, char *argv[])
{
	service_id_t service_id;
	int rc;
	int i;

	printf(NAME ": VIA-CUDA Apple Desktop Bus driver\n");
	
	/*
	 * Alleviate the virtual memory / page table pressure caused by
	 * interrupt storms when the default large stacks are used.
	 */
	async_set_notification_handler_stack_size(PAGE_SIZE);

	for (i = 0; i < ADB_MAX_ADDR; ++i) {
		adb_dev[i].client_sess = NULL;
		adb_dev[i].service_id = 0;
	}

	async_set_client_connection(cuda_connection);
	rc = loc_server_register(NAME);
	if (rc < 0) {
		printf(NAME ": Unable to register server.\n");
		return rc;
	}

	rc = loc_service_register("adb/kbd", &service_id);
	if (rc != EOK) {
		printf(NAME ": Unable to register service %s.\n", "adb/kdb");
		return rc;
	}

	adb_dev[2].service_id = service_id;
	adb_dev[8].service_id = service_id;

	rc = loc_service_register("adb/mouse", &service_id);
	if (rc != EOK) {
		printf(NAME ": Unable to register servise %s.\n", "adb/mouse");
		return rc;
	}

	adb_dev[9].service_id = service_id;

	if (cuda_init() < 0) {
		printf("cuda_init() failed\n");
		return 1;
	}

	task_retval(0);
	async_manager();

	return 0;
}

/** Character device connection handler */
static void cuda_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	service_id_t dsid;
	int dev_addr, i;

	/* Get the device handle. */
	dsid = IPC_GET_ARG1(*icall);

	/* Determine which disk device is the client connecting to. */
	dev_addr = -1;
	for (i = 0; i < ADB_MAX_ADDR; i++) {
		if (adb_dev[i].service_id == dsid)
			dev_addr = i;
	}

	if (dev_addr < 0) {
		async_answer_0(iid, EINVAL);
		return;
	}

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
			if (adb_dev[dev_addr].client_sess == NULL) {
				adb_dev[dev_addr].client_sess = sess;
				
				/*
				 * A hack so that we send the data to the session
				 * regardless of which address the device is on.
				 */
				for (i = 0; i < ADB_MAX_ADDR; ++i) {
					if (adb_dev[i].service_id == dsid)
						adb_dev[i].client_sess = sess;
				}
				
				async_answer_0(callid, EOK);
			} else
				async_answer_0(callid, ELIMIT);
		} else
			async_answer_0(callid, EINVAL);
	}
}


static int cuda_init(void)
{
	if (sysinfo_get_value("cuda.address.physical", &(instance->cuda_physical)) != EOK)
		return -1;
	
	void *vaddr;
	if (pio_enable((void *) instance->cuda_physical, sizeof(cuda_t), &vaddr) != 0)
		return -1;
	
	dev = vaddr;

	instance->cuda = dev;
	instance->xstate = cx_listen;
	instance->bidx = 0;
	instance->snd_bytes = 0;

	fibril_mutex_initialize(&instance->dev_lock);

	/* Disable all interrupts from CUDA. */
	pio_write_8(&dev->ier, IER_CLR | ALL_INT);

	cuda_irq_code.ranges[0].base = (uintptr_t) instance->cuda_physical;
	cuda_irq_code.cmds[0].addr = (void *) &((cuda_t *) instance->cuda_physical)->ifr;
	async_irq_subscribe(10, device_assign_devno(), cuda_irq_handler, NULL,
	    &cuda_irq_code);

	/* Enable SR interrupt. */
	pio_write_8(&dev->ier, TIP | TREQ);
	pio_write_8(&dev->ier, IER_SET | SR_INT);

	/* Enable ADB autopolling. */
	cuda_autopoll_set(true);

	return 0;
}

static void cuda_irq_handler(ipc_callid_t iid, ipc_call_t *call, void *arg)
{
	uint8_t rbuf[CUDA_RCV_BUF_SIZE];
	size_t len;
	bool handle;

	handle = false;
	len = 0;

	fibril_mutex_lock(&instance->dev_lock);

	switch (instance->xstate) {
	case cx_listen:
		cuda_irq_listen();
		break;
	case cx_receive:
		cuda_irq_receive();
		break;
	case cx_rcv_end:
		cuda_irq_rcv_end(rbuf, &len);
		handle = true;
		break;
	case cx_send_start:
		cuda_irq_send_start();
		break;
	case cx_send:
		cuda_irq_send();
		break;
	}
	
	/* Lower IFR.SR_INT so that CUDA can generate next int by raising it. */
	pio_write_8(&instance->cuda->ifr, SR_INT);

	fibril_mutex_unlock(&instance->dev_lock);

	/* Handle an incoming packet. */
	if (handle)
		cuda_packet_handle(rbuf, len);
}

/** Interrupt in listen state.
 *
 * Start packet reception.
 */
static void cuda_irq_listen(void)
{
	uint8_t b = pio_read_8(&dev->b);
	
	if ((b & TREQ) != 0) {
		printf("cuda_irq_listen: no TREQ?!\n");
		return;
	}
	
	pio_write_8(&dev->b, b & ~TIP);
	instance->xstate = cx_receive;
}

/** Interrupt in receive state.
 *
 * Receive next byte of packet.
 */
static void cuda_irq_receive(void)
{
	uint8_t data = pio_read_8(&dev->sr);
	if (instance->bidx < CUDA_RCV_BUF_SIZE)
		instance->rcv_buf[instance->bidx++] = data;
	
	uint8_t b = pio_read_8(&dev->b);
	
	if ((b & TREQ) == 0) {
		pio_write_8(&dev->b, b ^ TACK);
	} else {
		pio_write_8(&dev->b, b | TACK | TIP);
		instance->xstate = cx_rcv_end;
	}
}

/** Interrupt in rcv_end state.
 *
 * Terminate packet reception. Either go back to listen state or start
 * receiving another packet if CUDA has one for us.
 */
static void cuda_irq_rcv_end(void *buf, size_t *len)
{
	uint8_t b = pio_read_8(&dev->b);
	
	if ((b & TREQ) == 0) {
		instance->xstate = cx_receive;
		pio_write_8(&dev->b, b & ~TIP);
	} else {
		instance->xstate = cx_listen;
		cuda_send_start();
	}
	
	memcpy(buf, instance->rcv_buf, instance->bidx);
	*len = instance->bidx;
	instance->bidx = 0;
}

/** Interrupt in send_start state.
 *
 * Process result of sending first byte (and send second on success).
 */
static void cuda_irq_send_start(void)
{
	uint8_t b;

	b = pio_read_8(&dev->b);

	if ((b & TREQ) == 0) {
		/* Collision */
		pio_write_8(&dev->acr, pio_read_8(&dev->acr) & ~SR_OUT);
		pio_read_8(&dev->sr);
		pio_write_8(&dev->b, pio_read_8(&dev->b) | TIP | TACK);
		instance->xstate = cx_listen;
		return;
	}

	pio_write_8(&dev->sr, instance->snd_buf[1]);
	pio_write_8(&dev->b, pio_read_8(&dev->b) ^ TACK);
	instance->bidx = 2;

	instance->xstate = cx_send;
}

/** Interrupt in send state.
 *
 * Send next byte or terminate transmission.
 */
static void cuda_irq_send(void)
{
	if (instance->bidx < instance->snd_bytes) {
		/* Send next byte. */
		pio_write_8(&dev->sr, instance->snd_buf[instance->bidx++]);
		pio_write_8(&dev->b, pio_read_8(&dev->b) ^ TACK);
		return;
	}

	/* End transfer. */
	instance->snd_bytes = 0;
	instance->bidx = 0;

	pio_write_8(&dev->acr, pio_read_8(&dev->acr) & ~SR_OUT);
	pio_read_8(&dev->sr);
	pio_write_8(&dev->b, pio_read_8(&dev->b) | TACK | TIP);

	instance->xstate = cx_listen;
	/* TODO: Match reply with request. */
}

static void cuda_packet_handle(uint8_t *data, size_t len)
{
	if (data[0] != PT_ADB)
		return;
	if (len < 2)
		return;

	adb_packet_handle(data + 2, len - 2, (data[1] & 0x40) != 0);
}

static void adb_packet_handle(uint8_t *data, size_t size, bool autopoll)
{
	uint8_t dev_addr;
	uint8_t reg_no;
	uint16_t reg_val;
	unsigned i;

	dev_addr = data[0] >> 4;
	reg_no = data[0] & 0x03;

	if (size != 3) {
		printf("unrecognized packet, size=%d\n", size);
		for (i = 0; i < size; ++i) {
			printf(" 0x%02x", data[i]);
		}
		putchar('\n');
		return;
	}

	if (reg_no != 0) {
		printf("unrecognized packet, size=%d\n", size);
		for (i = 0; i < size; ++i) {
			printf(" 0x%02x", data[i]);
		}
		putchar('\n');
		return;
	}

	reg_val = ((uint16_t) data[1] << 8) | (uint16_t) data[2];

	if (adb_dev[dev_addr].client_sess == NULL)
		return;

	async_exch_t *exch =
	    async_exchange_begin(adb_dev[dev_addr].client_sess);
	async_msg_1(exch, ADB_REG_NOTIF, reg_val);
	async_exchange_end(exch);
}

static void cuda_autopoll_set(bool enable)
{
	instance->snd_buf[0] = PT_CUDA;
	instance->snd_buf[1] = CPT_AUTOPOLL;
	instance->snd_buf[2] = enable ? 0x01 : 0x00;
	instance->snd_bytes = 3;
	instance->bidx = 0;

	cuda_send_start();
}

static void cuda_send_start(void)
{
	cuda_t *dev = instance->cuda;

	assert(instance->xstate == cx_listen);

	if (instance->snd_bytes == 0)
		return;

	/* Check for incoming data. */
	if ((pio_read_8(&dev->b) & TREQ) == 0)
		return;

	pio_write_8(&dev->acr, pio_read_8(&dev->acr) | SR_OUT);
	pio_write_8(&dev->sr, instance->snd_buf[0]);
	pio_write_8(&dev->b, pio_read_8(&dev->b) & ~TIP);

	instance->xstate = cx_send_start;
}

/** @}
 */
