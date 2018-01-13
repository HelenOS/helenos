/*
 * Copyright (c) 2015 Jakub Jermar
 * Copyright (c) 2013 Jiri Svoboda
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
 * MSIM ddisk block device driver
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <device/hw_res_parsed.h>
#include <ddi.h>
#include <bd_srv.h>
#include <fibril_synch.h>
#include <as.h>

#define NAME	"ddisk"

#define DDISK_FUN_NAME	"a"

#define DDISK_BLOCK_SIZE	512

#define DDISK_STAT_IRQ_PENDING	0x4
#define DDISK_CMD_READ		0x1
#define DDISK_CMD_WRITE		0x2
#define DDISK_CMD_IRQ_DEASSERT	0x4

static errno_t ddisk_dev_add(ddf_dev_t *);
static errno_t ddisk_dev_remove(ddf_dev_t *);
static errno_t ddisk_dev_gone(ddf_dev_t *);
static errno_t ddisk_fun_online(ddf_fun_t *);
static errno_t ddisk_fun_offline(ddf_fun_t *);

static void ddisk_bd_connection(ipc_callid_t, ipc_call_t *, void *);

static void ddisk_irq_handler(ipc_call_t *, ddf_dev_t *);

static driver_ops_t driver_ops = {
	.dev_add = ddisk_dev_add,
	.dev_remove = ddisk_dev_remove,
	.dev_gone = ddisk_dev_gone,
	.fun_online = ddisk_fun_online,
	.fun_offline = ddisk_fun_offline
};

static driver_t ddisk_driver = {
	.name = NAME,
	.driver_ops = &driver_ops
};

typedef struct {
	int irq;
	uintptr_t base;
} ddisk_res_t;

typedef struct {
	ioport32_t dma_buffer;
	ioport32_t block;
	union {
		ioport32_t status;
		ioport32_t command;
	};
	ioport32_t size;
} ddisk_regs_t;

typedef struct {
	fibril_mutex_t lock;

	fibril_condvar_t io_cv;
	bool io_busy;

	ssize_t size;
	size_t blocks;

	uintptr_t dma_buffer_phys;
	void *dma_buffer;

	ddf_dev_t *dev;
	ddf_fun_t *fun;

	ddisk_res_t ddisk_res;
	ddisk_regs_t *ddisk_regs;

	int irq_cap;

	bd_srvs_t bds;
} ddisk_t;

static errno_t ddisk_bd_open(bd_srvs_t *, bd_srv_t *);
static errno_t ddisk_bd_close(bd_srv_t *);
static errno_t ddisk_bd_read_blocks(bd_srv_t *, aoff64_t, size_t, void *, size_t);
static errno_t ddisk_bd_write_blocks(bd_srv_t *, aoff64_t, size_t, const void *,
    size_t);
static errno_t ddisk_bd_get_block_size(bd_srv_t *, size_t *);
static errno_t ddisk_bd_get_num_blocks(bd_srv_t *, aoff64_t *);

bd_ops_t ddisk_bd_ops = {
	.open = ddisk_bd_open,
	.close = ddisk_bd_close,
	.read_blocks = ddisk_bd_read_blocks,
	.write_blocks = ddisk_bd_write_blocks,
	.get_block_size = ddisk_bd_get_block_size,
	.get_num_blocks = ddisk_bd_get_num_blocks,
};

irq_pio_range_t ddisk_irq_pio_ranges[] = {
	{
		.base = 0,
		.size = sizeof(ddisk_regs_t)
	}
};

irq_cmd_t ddisk_irq_commands[] = {
	{
		.cmd = CMD_PIO_READ_32,
		.addr = NULL,
		.dstarg = 1
	},
	{
		.cmd = CMD_AND,
		.srcarg = 1,
		.value = DDISK_STAT_IRQ_PENDING,
		.dstarg = 2
	},
	{
		.cmd = CMD_PREDICATE,
		.srcarg = 2,
		.value = 2 
	},
	{
		/* Deassert the DMA interrupt. */
		.cmd = CMD_PIO_WRITE_32,
		.value = DDISK_CMD_IRQ_DEASSERT,
		.addr = NULL 
	},
	{
		.cmd = CMD_ACCEPT
	}
};

irq_code_t ddisk_irq_code = {
	.rangecount = 1,
	.ranges = ddisk_irq_pio_ranges,
	.cmdcount = sizeof(ddisk_irq_commands) / sizeof(irq_cmd_t),
	.cmds = ddisk_irq_commands,
};

void ddisk_irq_handler(ipc_call_t *icall, ddf_dev_t *dev)
{
	ddf_msg(LVL_DEBUG, "ddisk_irq_handler(), status=%" PRIx32,
	    (uint32_t) IPC_GET_ARG1(*icall));

	ddisk_t *ddisk = (ddisk_t *) ddf_dev_data_get(dev);
	
	fibril_mutex_lock(&ddisk->lock);
	fibril_condvar_broadcast(&ddisk->io_cv);
	fibril_mutex_unlock(&ddisk->lock);
}

errno_t ddisk_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

errno_t ddisk_bd_close(bd_srv_t *bd)
{
	return EOK;
}

static
errno_t ddisk_rw_block(ddisk_t *ddisk, bool read, aoff64_t ba, void *buf)
{
	fibril_mutex_lock(&ddisk->lock);

	ddf_msg(LVL_DEBUG, "ddisk_rw_block(): read=%d, ba=%" PRId64 ", buf=%p",
	    read, ba, buf);

	if (ba >= ddisk->blocks)
		return ELIMIT;

	while (ddisk->io_busy)
		fibril_condvar_wait(&ddisk->io_cv, &ddisk->lock);

	ddisk->io_busy = true;

	if (!read)
		memcpy(ddisk->dma_buffer, buf, DDISK_BLOCK_SIZE);
	
	pio_write_32(&ddisk->ddisk_regs->dma_buffer,
	    ddisk->dma_buffer_phys);
	pio_write_32(&ddisk->ddisk_regs->block, (uint32_t) ba);
	pio_write_32(&ddisk->ddisk_regs->command,
	    read ? DDISK_CMD_READ : DDISK_CMD_WRITE);

	fibril_condvar_wait(&ddisk->io_cv, &ddisk->lock);

	if (read)
		memcpy(buf, ddisk->dma_buffer, DDISK_BLOCK_SIZE);

	ddisk->io_busy = false;
	fibril_condvar_signal(&ddisk->io_cv);
	fibril_mutex_unlock(&ddisk->lock);

	return EOK;
}

static
errno_t ddisk_bd_rw_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt, void *buf,
    size_t size, bool is_read)
{
	ddisk_t *ddisk = (ddisk_t *) bd->srvs->sarg;
	aoff64_t i;
	errno_t rc;

	if (size < cnt * DDISK_BLOCK_SIZE)
		return EINVAL;		

	for (i = 0; i < cnt; i++) {
		rc = ddisk_rw_block(ddisk, is_read, ba + i,
		    buf + i * DDISK_BLOCK_SIZE);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

errno_t ddisk_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt, void *buf,
    size_t size)
{
	return ddisk_bd_rw_blocks(bd, ba, cnt, buf, size, true);
}

errno_t ddisk_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	return ddisk_bd_rw_blocks(bd, ba, cnt, (void *) buf, size, false);
}

errno_t ddisk_bd_get_block_size(bd_srv_t *bd, size_t *rsize)
{
	*rsize = DDISK_BLOCK_SIZE; 
	return EOK;
}

errno_t ddisk_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *rnb)
{
	ddisk_t *ddisk = (ddisk_t *) bd->srvs->sarg;

	*rnb = ddisk->blocks;
	return EOK;	
}

static errno_t ddisk_get_res(ddf_dev_t *dev, ddisk_res_t *ddisk_res)
{
	async_sess_t *parent_sess;
	hw_res_list_parsed_t hw_res;
	errno_t rc;

	parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL)
		return ENOMEM;

	hw_res_list_parsed_init(&hw_res);
	rc = hw_res_get_list_parsed(parent_sess, &hw_res, 0);
	if (rc != EOK)
		return rc;

	if ((hw_res.mem_ranges.count != 1) || (hw_res.irqs.count != 1)) {
		rc = EINVAL;
		goto error;
	}

	addr_range_t *regs = &hw_res.mem_ranges.ranges[0];
	ddisk_res->base = RNGABS(*regs);
	ddisk_res->irq = hw_res.irqs.irqs[0]; 

	if (RNGSZ(*regs) < sizeof(ddisk_regs_t)) {
		rc = EINVAL;
		goto error;
	}

	rc = EOK;
error:
	hw_res_list_parsed_clean(&hw_res);
	return rc;
}

static errno_t ddisk_fun_create(ddisk_t *ddisk)
{
	errno_t rc;
	ddf_fun_t *fun = NULL;

	fun = ddf_fun_create(ddisk->dev, fun_exposed, DDISK_FUN_NAME);
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating DDF function.");
		rc = ENOMEM;
		goto error;
	}

	/* Set up a connection handler. */
	ddf_fun_set_conn_handler(fun, ddisk_bd_connection);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding DDF function %s: %s",
		    DDISK_FUN_NAME, str_error(rc));
		goto error;
	}

	ddf_fun_add_to_category(fun, "disk");
	ddisk->fun = fun;

	return EOK;
error:
	if (fun != NULL)
		ddf_fun_destroy(fun);

	return rc;
}

static errno_t ddisk_fun_remove(ddisk_t *ddisk)
{
	errno_t rc;

	if (ddisk->fun == NULL)
		return EOK;

	ddf_msg(LVL_DEBUG, "ddisk_fun_remove(%p, '%s')", ddisk,
	    DDISK_FUN_NAME);
	rc = ddf_fun_offline(ddisk->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error offlining function '%s'.",
		    DDISK_FUN_NAME);
		goto error;
	}

	rc = ddf_fun_unbind(ddisk->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.",
		    DDISK_FUN_NAME);
		goto error;
	}

	ddf_fun_destroy(ddisk->fun);
	ddisk->fun = NULL;
	rc = EOK;

error:
	return rc;
}

static errno_t ddisk_fun_unbind(ddisk_t *ddisk)
{
	errno_t rc;

	if (ddisk->fun == NULL)
		return EOK;

	ddf_msg(LVL_DEBUG, "ddisk_fun_unbind(%p, '%s')", ddisk,
	    DDISK_FUN_NAME);
	rc = ddf_fun_unbind(ddisk->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed unbinding function '%s'.",
		    DDISK_FUN_NAME);
		goto error;
	}

	ddf_fun_destroy(ddisk->fun);
	ddisk->fun = NULL;
	rc = EOK;

error:
	return rc;
}

/** Add new device
 *
 * @param  dev New device
 * @return     EOK on success or an error code.
 */
static errno_t ddisk_dev_add(ddf_dev_t *dev)
{
	ddisk_t *ddisk;
	ddisk_res_t res;
	errno_t rc;

	/*
	 * Get our resources.
	 */
	rc = ddisk_get_res(dev, &res);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Invalid HW resource configuration.");
		return EINVAL;
	}

	/*
	 * Allocate soft state.
	 */
	ddisk = ddf_dev_data_alloc(dev, sizeof(ddisk_t));
	if (!ddisk) {
		ddf_msg(LVL_ERROR, "Failed allocating soft state.");
		rc = ENOMEM;
		goto error;
	}

	/*
	 * Initialize soft state.
	 */
	fibril_mutex_initialize(&ddisk->lock);
	ddisk->dev = dev;
	ddisk->ddisk_res = res;

	fibril_condvar_initialize(&ddisk->io_cv);
	ddisk->io_busy = false;

	bd_srvs_init(&ddisk->bds);
	ddisk->bds.ops = &ddisk_bd_ops;
	ddisk->bds.sarg = ddisk;

	ddisk->irq_cap = -1;

	/*
	 * Enable access to ddisk's PIO registers.
	 */
	void *vaddr;
	rc = pio_enable((void *) res.base, sizeof(ddisk_regs_t), &vaddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot initialize device I/O space.");
		goto error;
	}
	ddisk->ddisk_regs = vaddr;

	ddisk->size = (int32_t) pio_read_32(&ddisk->ddisk_regs->size);
	ddisk->blocks = ddisk->size / DDISK_BLOCK_SIZE;

	if (ddisk->size <= 0) {
		ddf_msg(LVL_WARN, "No disk detected.");
		rc = EIO;
		goto error;
	}

	/*
	 * Allocate DMA buffer.
	 */
	ddisk->dma_buffer = AS_AREA_ANY;
	rc = dmamem_map_anonymous(DDISK_BLOCK_SIZE, DMAMEM_4GiB,
	    AS_AREA_READ | AS_AREA_WRITE, 0, &ddisk->dma_buffer_phys,
	    &ddisk->dma_buffer);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Cannot allocate DMA memory.");
		goto error;
	}

	ddf_msg(LVL_NOTE, "Allocated DMA buffer at %p virtual and %p physical.",
	    ddisk->dma_buffer, (void *) ddisk->dma_buffer_phys);

	/*
	 * Create an exposed function.
	 */
	rc = ddisk_fun_create(ddisk);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed initializing ddisk controller.");
		rc = EIO;
		goto error;
	}

	/*
 	 * Register IRQ handler.
 	 */
	ddisk_regs_t *res_phys = (ddisk_regs_t *) res.base;
	ddisk_irq_pio_ranges[0].base = res.base;
	ddisk_irq_commands[0].addr = (void *) &res_phys->status;
	ddisk_irq_commands[3].addr = (void *) &res_phys->command;
	rc = register_interrupt_handler(dev, ddisk->ddisk_res.irq,
	    ddisk_irq_handler, &ddisk_irq_code, &ddisk->irq_cap);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to register interrupt handler.");
		goto error;
	}

	/*
	 * Success, report what we have found.
	 */
	ddf_msg(LVL_NOTE,
	    "Device at %p with %zd blocks (%zuB) using interrupt %d",
	    (void *) ddisk->ddisk_res.base, ddisk->blocks,
	    ddisk->size, ddisk->ddisk_res.irq);

	return EOK;

error:
	if (ddisk->ddisk_regs)
		pio_disable(ddisk->ddisk_regs, sizeof(ddisk_regs_t));
	if (ddisk->dma_buffer)
		dmamem_unmap_anonymous(ddisk->dma_buffer);

	return rc;
}


static errno_t ddisk_dev_remove_common(ddisk_t *ddisk, bool surprise)
{
	errno_t rc;

	if (!surprise)
		rc = ddisk_fun_remove(ddisk);
	else
		rc = ddisk_fun_unbind(ddisk);

	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Unable to cleanup function '%s'.",
		    DDISK_FUN_NAME);
		return rc;
	}

	unregister_interrupt_handler(ddisk->dev, ddisk->irq_cap);
	
	rc = pio_disable(ddisk->ddisk_regs, sizeof(ddisk_regs_t));
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Unable to disable PIO.");
		return rc;	
	}

	dmamem_unmap_anonymous(ddisk->dma_buffer);

	return EOK;
}

static errno_t ddisk_dev_remove(ddf_dev_t *dev)
{
	ddisk_t *ddisk = (ddisk_t *) ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "ddisk_dev_remove(%p)", dev);
	return ddisk_dev_remove_common(ddisk, false);
}

static errno_t ddisk_dev_gone(ddf_dev_t *dev)
{
	ddisk_t *ddisk = (ddisk_t *) ddf_dev_data_get(dev);

	ddf_msg(LVL_DEBUG, "ddisk_dev_gone(%p)", dev);
	return ddisk_dev_remove_common(ddisk, true);
}

static errno_t ddisk_fun_online(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "ddisk_fun_online()");
	return ddf_fun_online(fun);
}

static errno_t ddisk_fun_offline(ddf_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "ddisk_fun_offline()");
	return ddf_fun_offline(fun);
}

/** Block device connection handler */
static void ddisk_bd_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ddisk_t *ddisk;
	ddf_fun_t *fun = (ddf_fun_t *) arg;

	ddisk = (ddisk_t *) ddf_dev_data_get(ddf_fun_get_dev(fun));
	bd_conn(iid, icall, &ddisk->bds);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS MSIM ddisk device driver\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&ddisk_driver);
}
