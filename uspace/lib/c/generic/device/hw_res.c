/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2010 Lenka Trochtova
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

/** Get bus flags.
 *
 * @param sess HW res session
 * @param rflags Place to store the flags
 *
 * @return Error code.
 *
 */
errno_t hw_res_get_flags(async_sess_t *sess, hw_res_flags_t *rflags)
{
	async_exch_t *exch = async_exchange_begin(sess);

	sysarg_t flags;
	const errno_t ret = async_req_1_1(exch, DEV_IFACE_ID(HW_RES_DEV_IFACE),
	    HW_RES_GET_FLAGS, &flags);

	async_exchange_end(exch);

	if (ret == EOK)
		*rflags = flags;

	return ret;
}

/** @}
 */
