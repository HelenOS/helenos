/*
 * Copyright (c) 2011 Jiri Michalec
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

#include <device/hw_res_parsed.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

static void hw_res_parse_add_dma_channel(hw_res_list_parsed_t *out,
    const hw_resource_t *res, int flags)
{
	assert(res);
	assert((res->type == DMA_CHANNEL_8) || (res->type == DMA_CHANNEL_16));

	const unsigned channel = (res->type == DMA_CHANNEL_8) ?
	    res->res.dma_channel.dma8 : res->res.dma_channel.dma16;
	const size_t count = out->dma_channels.count;
	const int keep_duplicit = flags & HW_RES_KEEP_DUPLICIT;

	if (!keep_duplicit) {
		for (size_t i = 0; i < count; ++i) {
			if (out->dma_channels.channels[i] == channel)
				return;
		}
	}

	out->dma_channels.channels[count] = channel;
	++out->dma_channels.count;
}

static void hw_res_parse_add_irq(hw_res_list_parsed_t *out,
    const hw_resource_t *res, int flags)
{
	assert(res && (res->type == INTERRUPT));

	int irq = res->res.interrupt.irq;
	size_t count = out->irqs.count;
	int keep_duplicit = flags & HW_RES_KEEP_DUPLICIT;

	if (!keep_duplicit) {
		for (size_t i = 0; i < count; i++) {
			if (out->irqs.irqs[i] == irq)
				return;
		}
	}

	out->irqs.irqs[count] = irq;
	out->irqs.count++;
}

static uint64_t absolutize(uint64_t addr, bool relative, uint64_t base)
{
	if (!relative)
		return addr;
	else
		return addr + base;
}

static uint64_t relativize(uint64_t addr, bool relative, uint64_t base)
{
	if (relative)
		return addr;
	else
		return addr - base;
}

static void hw_res_parse_add_io_range(hw_res_list_parsed_t *out,
    const pio_window_t *win, const hw_resource_t *res, int flags)
{
	endianness_t endianness;
	uint64_t absolute;
	uint64_t relative;
	size_t size;

	assert(res && (res->type == IO_RANGE));

	absolute = absolutize(res->res.io_range.address,
	    res->res.io_range.relative, win->io.base);
	relative = relativize(res->res.io_range.address,
	    res->res.io_range.relative, win->io.base);
	size = res->res.io_range.size;
	endianness = res->res.io_range.endianness;

	if ((size == 0) && (!(flags & HW_RES_KEEP_ZERO_AREA)))
		return;

	int keep_duplicit = flags & HW_RES_KEEP_DUPLICIT;
	size_t count = out->io_ranges.count;

	if (!keep_duplicit) {
		for (size_t i = 0; i < count; i++) {
			uint64_t s_address;
			size_t s_size;

			s_address = RNGABS(out->io_ranges.ranges[i]);
			s_size = RNGSZ(out->io_ranges.ranges[i]);

			if ((absolute == s_address) && (size == s_size))
				return;
		}
	}

	RNGABS(out->io_ranges.ranges[count]) = absolute;
	RNGREL(out->io_ranges.ranges[count]) = relative;
	RNGSZ(out->io_ranges.ranges[count]) = size;
	out->io_ranges.ranges[count].endianness = endianness;
	out->io_ranges.count++;
}

static void hw_res_parse_add_mem_range(hw_res_list_parsed_t *out,
    const pio_window_t *win, const hw_resource_t *res, int flags)
{
	endianness_t endianness;
	uint64_t absolute;
	uint64_t relative;
	size_t size;

	assert(res && (res->type == MEM_RANGE));

	absolute = absolutize(res->res.mem_range.address,
	    res->res.mem_range.relative, win->mem.base);
	relative = relativize(res->res.mem_range.address,
	    res->res.mem_range.relative, win->mem.base);
	size = res->res.mem_range.size;
	endianness = res->res.mem_range.endianness;

	if ((size == 0) && (!(flags & HW_RES_KEEP_ZERO_AREA)))
		return;

	int keep_duplicit = flags & HW_RES_KEEP_DUPLICIT;
	size_t count = out->mem_ranges.count;

	if (!keep_duplicit) {
		for (size_t i = 0; i < count; ++i) {
			uint64_t s_address;
			size_t s_size;

			s_address = RNGABS(out->mem_ranges.ranges[i]);
			s_size = RNGSZ(out->mem_ranges.ranges[i]);

			if ((absolute == s_address) && (size == s_size))
				return;
		}
	}

	RNGABS(out->mem_ranges.ranges[count]) = absolute;
	RNGREL(out->mem_ranges.ranges[count]) = relative;
	RNGSZ(out->mem_ranges.ranges[count]) = size;
	out->mem_ranges.ranges[count].endianness = endianness;
	out->mem_ranges.count++;
}

/** Parse list of hardware resources
 *
 * @param      win          PIO window.
 * @param      res          Original structure resource.
 * @param[out] out          Output parsed resources.
 * @param      flags        Flags of the parsing:
 *                          HW_RES_KEEP_ZERO_AREA for keeping zero-size areas,
 *                          HW_RES_KEEP_DUPLICITIES to keep duplicit areas.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t hw_res_list_parse(const pio_window_t *win,
    const hw_resource_list_t *res, hw_res_list_parsed_t *out, int flags)
{
	if (!res || !out)
		return EINVAL;

	size_t res_count = res->count;
	hw_res_list_parsed_clean(out);

	out->irqs.irqs = calloc(res_count, sizeof(int));
	out->dma_channels.channels = calloc(res_count, sizeof(int));
	out->io_ranges.ranges = calloc(res_count, sizeof(io_range_t));
	out->mem_ranges.ranges = calloc(res_count, sizeof(mem_range_t));
	if (!out->irqs.irqs || !out->dma_channels.channels ||
	    !out->io_ranges.ranges || !out->mem_ranges.ranges) {
		hw_res_list_parsed_clean(out);
		return ENOMEM;
	}

	for (size_t i = 0; i < res_count; ++i) {
		const hw_resource_t *resource = &res->resources[i];

		switch (resource->type) {
		case INTERRUPT:
			hw_res_parse_add_irq(out, resource, flags);
			break;
		case IO_RANGE:
			hw_res_parse_add_io_range(out, win, resource, flags);
			break;
		case MEM_RANGE:
			hw_res_parse_add_mem_range(out, win, resource, flags);
			break;
		case DMA_CHANNEL_8:
		case DMA_CHANNEL_16:
			hw_res_parse_add_dma_channel(out, resource, flags);
			break;
		default:
			hw_res_list_parsed_clean(out);
			return EINVAL;
		}
	}

	return EOK;
}

/** Get hw_resources from the parent device.
 *
 * The output must be inited, will be cleared
 *
 * @see get_hw_resources
 * @see hw_resources_parse
 *
 * @param sess                Session to the parent device
 * @param hw_resources_parsed Output list
 * @param flags               Parsing flags
 *
 * @return EOK if succeed, error code otherwise
 *
 */
errno_t hw_res_get_list_parsed(async_sess_t *sess,
    hw_res_list_parsed_t *hw_res_parsed, int flags)
{
	pio_window_t pio_window;
	errno_t rc;

	if (!hw_res_parsed)
		return EBADMEM;

	hw_resource_list_t hw_resources;
	hw_res_list_parsed_clean(hw_res_parsed);
	memset(&hw_resources, 0, sizeof(hw_resource_list_t));

	rc = pio_window_get(sess, &pio_window);
	if (rc != EOK)
		return rc;

	rc = hw_res_get_resource_list(sess, &hw_resources);
	if (rc != EOK)
		return rc;

	rc = hw_res_list_parse(&pio_window, &hw_resources, hw_res_parsed,
	    flags);
	hw_res_clean_resource_list(&hw_resources);

	return rc;
}

/** @}
 */
