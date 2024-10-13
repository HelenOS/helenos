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

#include "virtio-blk.h"

#include <stdio.h>
#include <stdint.h>

#include <as.h>
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <pci_dev_iface.h>
#include <fibril_synch.h>

#include <bd_srv.h>

#include <virtio-pci.h>

#define NAME	"virtio-blk"

#define VIRTIO_BLK_NUM_QUEUES	1

#define RQ_QUEUE	0

/*
 * VIRTIO_BLK requests need at least two descriptors so that device-read-only
 * buffers are separated from device-writable buffers. For convenience, we
 * always use three descriptors for the request header, buffer and footer.
 * We therefore organize the virtque so that first RQ_BUFFERS descriptors are
 * used for request headers, the following RQ_BUFFERS descriptors are used
 * for in/out buffers and the last RQ_BUFFERS descriptors are used for request
 * footers.
 */
#define REQ_HEADER_DESC(descno)	(0 * RQ_BUFFERS + (descno))
#define REQ_BUFFER_DESC(descno)	(1 * RQ_BUFFERS + (descno))
#define REQ_FOOTER_DESC(descno)	(2 * RQ_BUFFERS + (descno))

static errno_t virtio_blk_dev_add(ddf_dev_t *dev);

static driver_ops_t virtio_blk_driver_ops = {
	.dev_add = virtio_blk_dev_add
};

static driver_t virtio_blk_driver = {
	.name = NAME,
	.driver_ops = &virtio_blk_driver_ops
};

/** VirtIO block IRQ handler.
 *
 * @param icall IRQ event notification
 * @param arg Argument (virtio_blk_t *)
 */
static void virtio_blk_irq_handler(ipc_call_t *icall, void *arg)
{
	virtio_blk_t *virtio_blk = (virtio_blk_t *)arg;
	virtio_dev_t *vdev = &virtio_blk->virtio_dev;

	uint16_t descno;
	uint32_t len;

	while (virtio_virtq_consume_used(vdev, RQ_QUEUE, &descno, &len)) {
		assert(descno < RQ_BUFFERS);
		fibril_mutex_lock(&virtio_blk->completion_lock[descno]);
		fibril_condvar_signal(&virtio_blk->completion_cv[descno]);
		fibril_mutex_unlock(&virtio_blk->completion_lock[descno]);
	}
}

static errno_t virtio_blk_register_interrupt(ddf_dev_t *dev)
{
	virtio_blk_t *virtio_blk = (virtio_blk_t *) ddf_dev_data_get(dev);
	virtio_dev_t *vdev = &virtio_blk->virtio_dev;

	async_sess_t *parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL)
		return ENOMEM;

	hw_res_list_parsed_t res;
	hw_res_list_parsed_init(&res);

	hw_res_list_parsed_init(&res);
	errno_t rc = hw_res_get_list_parsed(parent_sess, &res, 0);
	if (rc != EOK)
		return rc;

	if (res.irqs.count < 1) {
		hw_res_list_parsed_clean(&res);
		return EINVAL;
	}

	virtio_blk->irq = res.irqs.irqs[0];
	hw_res_list_parsed_clean(&res);

	irq_pio_range_t pio_ranges[] = {
		{
			.base = vdev->isr_phys,
			.size = sizeof(vdev->isr_phys),
		}
	};

	irq_cmd_t irq_commands[] = {
		{
			.cmd = CMD_PIO_READ_8,
			.addr = (void *) vdev->isr_phys,
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

	irq_code_t irq_code = {
		.rangecount = sizeof(pio_ranges) / sizeof(irq_pio_range_t),
		.ranges = pio_ranges,
		.cmdcount = sizeof(irq_commands) / sizeof(irq_cmd_t),
		.cmds = irq_commands
	};

	return register_interrupt_handler(dev, virtio_blk->irq,
	    virtio_blk_irq_handler, (void *)virtio_blk, &irq_code,
	    &virtio_blk->irq_handle);
}

static errno_t virtio_blk_bd_open(bd_srvs_t *bds, bd_srv_t *bd)
{
	return EOK;
}

static errno_t virtio_blk_bd_close(bd_srv_t *bd)
{
	return EOK;
}

static errno_t virtio_blk_rw_block(virtio_blk_t *virtio_blk, bool read,
    aoff64_t ba, void *buf)
{
	virtio_dev_t *vdev = &virtio_blk->virtio_dev;

	/*
	 * Allocate a descriptor.
	 *
	 * The allocated descno will determine the header descriptor
	 * (REQ_HEADER_DESC), the buffer descriptor (REQ_BUFFER_DESC) and the
	 * footer (REQ_FOOTER_DESC) descriptor.
	 */
	fibril_mutex_lock(&virtio_blk->free_lock);
	uint16_t descno = virtio_alloc_desc(vdev, RQ_QUEUE,
	    &virtio_blk->rq_free_head);
	while (descno == (uint16_t) -1U) {
		fibril_condvar_wait(&virtio_blk->free_cv,
		    &virtio_blk->free_lock);
		descno = virtio_alloc_desc(vdev, RQ_QUEUE,
		    &virtio_blk->rq_free_head);
	}
	fibril_mutex_unlock(&virtio_blk->free_lock);

	assert(descno < RQ_BUFFERS);

	/* Setup the request header */
	virtio_blk_req_header_t *req_header =
	    (virtio_blk_req_header_t *) virtio_blk->rq_header[descno];
	memset(req_header, 0, sizeof(virtio_blk_req_header_t));
	pio_write_le32(&req_header->type,
	    read ? VIRTIO_BLK_T_IN : VIRTIO_BLK_T_OUT);
	pio_write_le64(&req_header->sector, ba);

	/* Copy write data to the request. */
	if (!read)
		memcpy(virtio_blk->rq_buf[descno], buf, VIRTIO_BLK_BLOCK_SIZE);

	fibril_mutex_lock(&virtio_blk->completion_lock[descno]);

	/*
	 * Set the descriptors, chain them in the virtqueue and notify the
	 * device.
	 */
	virtio_virtq_desc_set(vdev, RQ_QUEUE, REQ_HEADER_DESC(descno),
	    virtio_blk->rq_header_p[descno], sizeof(virtio_blk_req_header_t),
	    VIRTQ_DESC_F_NEXT, REQ_BUFFER_DESC(descno));
	virtio_virtq_desc_set(vdev, RQ_QUEUE, REQ_BUFFER_DESC(descno),
	    virtio_blk->rq_buf_p[descno], VIRTIO_BLK_BLOCK_SIZE,
	    VIRTQ_DESC_F_NEXT | (read ? VIRTQ_DESC_F_WRITE : 0),
	    REQ_FOOTER_DESC(descno));
	virtio_virtq_desc_set(vdev, RQ_QUEUE, REQ_FOOTER_DESC(descno),
	    virtio_blk->rq_footer_p[descno], sizeof(virtio_blk_req_footer_t),
	    VIRTQ_DESC_F_WRITE, 0);
	virtio_virtq_produce_available(vdev, RQ_QUEUE, descno);

	/*
	 * Wait for the completion of the request.
	 */
	fibril_condvar_wait(&virtio_blk->completion_cv[descno],
	    &virtio_blk->completion_lock[descno]);
	fibril_mutex_unlock(&virtio_blk->completion_lock[descno]);

	errno_t rc;
	virtio_blk_req_footer_t *footer =
	    (virtio_blk_req_footer_t *) virtio_blk->rq_footer[descno];
	switch (footer->status) {
	case VIRTIO_BLK_S_OK:
		rc = EOK;
		break;
	case VIRTIO_BLK_S_IOERR:
		rc = EIO;
		break;
	case VIRTIO_BLK_S_UNSUPP:
		rc = ENOTSUP;
		break;
	default:
		ddf_msg(LVL_DEBUG, "device returned unknown status=%d\n",
		    (int) footer->status);
		rc = EIO;
		break;
	}

	/* Copy read data from the request */
	if (rc == EOK && read)
		memcpy(buf, virtio_blk->rq_buf[descno], VIRTIO_BLK_BLOCK_SIZE);

	/* Free the descriptor and buffer */
	fibril_mutex_lock(&virtio_blk->free_lock);
	virtio_free_desc(vdev, RQ_QUEUE, &virtio_blk->rq_free_head, descno);
	fibril_condvar_signal(&virtio_blk->free_cv);
	fibril_mutex_unlock(&virtio_blk->free_lock);

	return rc;
}

static errno_t virtio_blk_bd_rw_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size, bool read)
{
	virtio_blk_t *virtio_blk = (virtio_blk_t *) bd->srvs->sarg;
	aoff64_t i;
	errno_t rc;

	if (size != cnt * VIRTIO_BLK_BLOCK_SIZE)
		return EINVAL;

	for (i = 0; i < cnt; i++) {
		rc = virtio_blk_rw_block(virtio_blk, read, ba + i,
		    buf + i * VIRTIO_BLK_BLOCK_SIZE);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

static errno_t virtio_blk_bd_read_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    void *buf, size_t size)
{
	return virtio_blk_bd_rw_blocks(bd, ba, cnt, buf, size, true);
}

static errno_t virtio_blk_bd_write_blocks(bd_srv_t *bd, aoff64_t ba, size_t cnt,
    const void *buf, size_t size)
{
	return virtio_blk_bd_rw_blocks(bd, ba, cnt, (void *) buf, size, false);
}

static errno_t virtio_blk_bd_get_block_size(bd_srv_t *bd, size_t *size)
{
	*size = VIRTIO_BLK_BLOCK_SIZE;
	return EOK;
}

static errno_t virtio_blk_bd_get_num_blocks(bd_srv_t *bd, aoff64_t *nb)
{
	virtio_blk_t *virtio_blk = (virtio_blk_t *) bd->srvs->sarg;
	virtio_blk_cfg_t *blkcfg = virtio_blk->virtio_dev.device_cfg;
	*nb = pio_read_le64(&blkcfg->capacity);
	return EOK;
}

bd_ops_t virtio_blk_bd_ops = {
	.open = virtio_blk_bd_open,
	.close = virtio_blk_bd_close,
	.read_blocks = virtio_blk_bd_read_blocks,
	.write_blocks = virtio_blk_bd_write_blocks,
	.get_block_size = virtio_blk_bd_get_block_size,
	.get_num_blocks = virtio_blk_bd_get_num_blocks,
};

static errno_t virtio_blk_initialize(ddf_dev_t *dev)
{
	virtio_blk_t *virtio_blk = ddf_dev_data_alloc(dev,
	    sizeof(virtio_blk_t));
	if (!virtio_blk)
		return ENOMEM;

	fibril_mutex_initialize(&virtio_blk->free_lock);
	fibril_condvar_initialize(&virtio_blk->free_cv);

	for (unsigned i = 0; i < RQ_BUFFERS; i++) {
		fibril_mutex_initialize(&virtio_blk->completion_lock[i]);
		fibril_condvar_initialize(&virtio_blk->completion_cv[i]);
	}

	bd_srvs_init(&virtio_blk->bds);
	virtio_blk->bds.ops = &virtio_blk_bd_ops;
	virtio_blk->bds.sarg = virtio_blk;

	errno_t rc = virtio_pci_dev_initialize(dev, &virtio_blk->virtio_dev);
	if (rc != EOK)
		return rc;

	virtio_dev_t *vdev = &virtio_blk->virtio_dev;
	virtio_pci_common_cfg_t *cfg = virtio_blk->virtio_dev.common_cfg;

	/*
	 * Register IRQ
	 */
	rc = virtio_blk_register_interrupt(dev);
	if (rc != EOK)
		goto fail;

	/* Reset the device and negotiate the feature bits */
	rc = virtio_device_setup_start(vdev, 0);
	if (rc != EOK)
		goto fail;

	/* Perform device-specific setup */

	/*
	 * Discover and configure the virtqueue
	 */
	uint16_t num_queues = pio_read_le16(&cfg->num_queues);
	if (num_queues != VIRTIO_BLK_NUM_QUEUES) {
		ddf_msg(LVL_NOTE, "Unsupported number of virtqueues: %u",
		    num_queues);
		rc = ELIMIT;
		goto fail;
	}

	vdev->queues = calloc(sizeof(virtq_t), num_queues);
	if (!vdev->queues) {
		rc = ENOMEM;
		goto fail;
	}

	/* For each in/out request we need 3 descriptors */
	rc = virtio_virtq_setup(vdev, RQ_QUEUE, 3 * RQ_BUFFERS);
	if (rc != EOK)
		goto fail;

	/*
	 * Setup DMA buffers
	 */
	rc = virtio_setup_dma_bufs(RQ_BUFFERS, sizeof(virtio_blk_req_header_t),
	    true, virtio_blk->rq_header, virtio_blk->rq_header_p);
	if (rc != EOK)
		goto fail;
	rc = virtio_setup_dma_bufs(RQ_BUFFERS, VIRTIO_BLK_BLOCK_SIZE,
	    true, virtio_blk->rq_buf, virtio_blk->rq_buf_p);
	if (rc != EOK)
		goto fail;
	rc = virtio_setup_dma_bufs(RQ_BUFFERS, sizeof(virtio_blk_req_footer_t),
	    false, virtio_blk->rq_footer, virtio_blk->rq_footer_p);
	if (rc != EOK)
		goto fail;

	/*
	 * Put all request descriptors on a free list. Because of the
	 * correspondence between the request, buffer and footer descriptors,
	 * we only need to manage allocations for one set: the request header
	 * descriptors.
	 */
	virtio_create_desc_free_list(vdev, RQ_QUEUE, RQ_BUFFERS,
	    &virtio_blk->rq_free_head);

	/*
	 * Enable IRQ
	 */
	rc = hw_res_enable_interrupt(ddf_dev_parent_sess_get(dev),
	    virtio_blk->irq);
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "Failed to enable interrupt");
		goto fail;
	}

	ddf_msg(LVL_NOTE, "Registered IRQ %d", virtio_blk->irq);

	/* Go live */
	virtio_device_setup_finalize(vdev);

	return EOK;

fail:
	virtio_teardown_dma_bufs(virtio_blk->rq_header);
	virtio_teardown_dma_bufs(virtio_blk->rq_buf);
	virtio_teardown_dma_bufs(virtio_blk->rq_footer);

	virtio_device_setup_fail(vdev);
	virtio_pci_dev_cleanup(vdev);
	return rc;
}

static void virtio_blk_uninitialize(ddf_dev_t *dev)
{
	virtio_blk_t *virtio_blk = (virtio_blk_t *) ddf_dev_data_get(dev);

	virtio_teardown_dma_bufs(virtio_blk->rq_header);
	virtio_teardown_dma_bufs(virtio_blk->rq_buf);
	virtio_teardown_dma_bufs(virtio_blk->rq_footer);

	virtio_device_setup_fail(&virtio_blk->virtio_dev);
	virtio_pci_dev_cleanup(&virtio_blk->virtio_dev);
}

static void virtio_blk_bd_connection(ipc_call_t *icall, void *arg)
{
	virtio_blk_t *virtio_blk;
	ddf_fun_t *fun = (ddf_fun_t *) arg;

	virtio_blk = (virtio_blk_t *) ddf_dev_data_get(ddf_fun_get_dev(fun));
	bd_conn(icall, &virtio_blk->bds);
}

static errno_t virtio_blk_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_NOTE, "%s %s (handle = %zu)", __func__,
	    ddf_dev_get_name(dev), ddf_dev_get_handle(dev));

	errno_t rc = virtio_blk_initialize(dev);
	if (rc != EOK)
		return rc;

	ddf_fun_t *fun = ddf_fun_create(dev, fun_exposed, "port0");
	if (fun == NULL) {
		rc = ENOMEM;
		goto uninitialize;
	}

	ddf_fun_set_conn_handler(fun, virtio_blk_bd_connection);

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding device function");
		goto destroy;
	}

	rc = ddf_fun_add_to_category(fun, "disk");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function to category");
		goto unbind;
	}

	ddf_msg(LVL_NOTE, "The %s device has been successfully initialized.",
	    ddf_dev_get_name(dev));

	return EOK;

unbind:
	ddf_fun_unbind(fun);
destroy:
	ddf_fun_destroy(fun);
uninitialize:
	virtio_blk_uninitialize(dev);
	return rc;
}

int main(void)
{
	printf("%s: HelenOS virtio-blk driver\n", NAME);

	(void) ddf_log_init(NAME);
	return ddf_driver_main(&virtio_blk_driver);
}
