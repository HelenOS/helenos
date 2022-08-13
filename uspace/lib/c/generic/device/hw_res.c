/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <device/hw_res.h>
#include <errno.h>
#include <async.h>
#include <stdlib.h>

errno_t hw_res_get_resource_list(async_sess_t *sess,
    hw_resource_list_t *hw_resources)
{
	sysarg_t count = 0;

	async_exch_t *exch = async_exchange_begin(sess);

	errno_t rc = async_req_1_1(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_GET_RESOURCE_LIST, &count);

	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	size_t size = count * sizeof(hw_resource_t);
	hw_resource_t *resources = (hw_resource_t *) malloc(size);
	if (resources == NULL) {
		// FIXME: This is protocol violation
		async_exchange_end(exch);
		return ENOMEM;
	}

	rc = async_data_read_start(exch, resources, size);
	async_exchange_end(exch);

	if (rc != EOK) {
		free(resources);
		return rc;
	}

	hw_resources->resources = resources;
	hw_resources->count = count;

	return EOK;
}

errno_t hw_res_enable_interrupt(async_sess_t *sess, int irq)
{
	async_exch_t *exch = async_exchange_begin(sess);

	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_ENABLE_INTERRUPT, irq);
	async_exchange_end(exch);

	return rc;
}

errno_t hw_res_disable_interrupt(async_sess_t *sess, int irq)
{
	async_exch_t *exch = async_exchange_begin(sess);

	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_DISABLE_INTERRUPT, irq);
	async_exchange_end(exch);

	return rc;
}

errno_t hw_res_clear_interrupt(async_sess_t *sess, int irq)
{
	async_exch_t *exch = async_exchange_begin(sess);

	errno_t rc = async_req_2_0(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_CLEAR_INTERRUPT, irq);
	async_exchange_end(exch);

	return rc;
}

/** Setup DMA channel to specified place and mode.
 *
 * @param channel DMA channel.
 * @param pa      Physical address of the buffer.
 * @param size    DMA buffer size.
 * @param mode    Mode of the DMA channel:
 *                 - Read or Write
 *                 - Allow automatic reset
 *                 - Use address decrement instead of increment
 *                 - Use SINGLE/BLOCK/ON DEMAND transfer mode
 *
 * @return Error code.
 *
 */
errno_t hw_res_dma_channel_setup(async_sess_t *sess,
    unsigned channel, uint32_t pa, uint32_t size, uint8_t mode)
{
	async_exch_t *exch = async_exchange_begin(sess);

	const uint32_t packed = (channel & 0xffff) | (mode << 16);
	const errno_t ret = async_req_4_0(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_DMA_CHANNEL_SETUP, packed, pa, size);

	async_exchange_end(exch);

	return ret;
}

/** Query remaining bytes in the buffer.
 *
 * @param channel DMA channel.
 *
 * @param[out] rem Number of bytes remaining in the buffer if positive.
 *
 * @return Error code.
 *
 */
errno_t hw_res_dma_channel_remain(async_sess_t *sess, unsigned channel, size_t *rem)
{
	async_exch_t *exch = async_exchange_begin(sess);

	sysarg_t remain;
	const errno_t ret = async_req_2_1(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_DMA_CHANNEL_REMAIN, channel, &remain);

	async_exchange_end(exch);

	if (ret == EOK)
		*rem = remain;

	return ret;
}

/** @}
 */
