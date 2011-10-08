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
#include <malloc.h>
#include <assert.h>
#include <errno.h>

static void hw_res_parse_add_irq(hw_res_list_parsed_t *out, hw_resource_t *res,
    int flags)
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

static void hw_res_parse_add_io_range(hw_res_list_parsed_t *out,
    hw_resource_t *res, int flags)
{
	assert(res && (res->type == IO_RANGE));
	
	uint64_t address = res->res.io_range.address;
	endianness_t endianness = res->res.io_range.endianness;
	size_t size = res->res.io_range.size;
	
	if ((size == 0) && (!(flags & HW_RES_KEEP_ZERO_AREA)))
		return;
	
	int keep_duplicit = flags & HW_RES_KEEP_DUPLICIT;
	size_t count = out->io_ranges.count;
	
	if (!keep_duplicit) {
		for (size_t i = 0; i < count; i++) {
			uint64_t s_address = out->io_ranges.ranges[i].address;
			size_t s_size = out->io_ranges.ranges[i].size;
			
			if ((address == s_address) && (size == s_size))
				return;
		}
	}
	
	out->io_ranges.ranges[count].address = address;
	out->io_ranges.ranges[count].endianness = endianness;
	out->io_ranges.ranges[count].size = size;
	out->io_ranges.count++;
}

static void hw_res_parse_add_mem_range(hw_res_list_parsed_t *out,
    hw_resource_t *res, int flags)
{
	assert(res && (res->type == MEM_RANGE));
	
	uint64_t address = res->res.mem_range.address;
	endianness_t endianness = res->res.mem_range.endianness;
	size_t size = res->res.mem_range.size;
	
	if ((size == 0) && (!(flags & HW_RES_KEEP_ZERO_AREA)))
		return;
	
	int keep_duplicit = flags & HW_RES_KEEP_DUPLICIT;
	size_t count = out->mem_ranges.count;
	
	if (!keep_duplicit) {
		for (size_t i = 0; i < count; ++i) {
			uint64_t s_address = out->mem_ranges.ranges[i].address;
			size_t s_size = out->mem_ranges.ranges[i].size;
			
			if ((address == s_address) && (size == s_size))
				return;
		}
	}
	
	out->mem_ranges.ranges[count].address = address;
	out->mem_ranges.ranges[count].endianness = endianness;
	out->mem_ranges.ranges[count].size = size;
	out->mem_ranges.count++;
}

/** Parse list of hardware resources
 *
 * @param      hw_resources Original structure resource
 * @param[out] out          Output parsed resources
 * @param      flags        Flags of the parsing.
 *                          HW_RES_KEEP_ZERO_AREA for keeping
 *                          zero-size areas, HW_RES_KEEP_DUPLICITIES
 *                          for keep duplicit areas
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
int hw_res_list_parse(hw_resource_list_t *hw_resources,
    hw_res_list_parsed_t *out, int flags)
{
	if ((!hw_resources) || (!out))
		return EINVAL;
	
	size_t res_count = hw_resources->count;
	hw_res_list_parsed_clean(out);
	
	out->irqs.irqs = malloc(res_count * sizeof(int));
	out->io_ranges.ranges = malloc(res_count * sizeof(io_range_t));
	out->mem_ranges.ranges = malloc(res_count * sizeof(mem_range_t));
	
	for (size_t i = 0; i < res_count; ++i) {
		hw_resource_t *resource = &(hw_resources->resources[i]);
		
		switch (resource->type) {
		case INTERRUPT:
			hw_res_parse_add_irq(out, resource, flags);
			break;
		case IO_RANGE:
			hw_res_parse_add_io_range(out, resource, flags);
			break;
		case MEM_RANGE:
			hw_res_parse_add_mem_range(out, resource, flags);
			break;
		default:
			return EINVAL;
		}
	}
	
	return EOK;
};

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
int hw_res_get_list_parsed(async_sess_t *sess,
    hw_res_list_parsed_t *hw_res_parsed, int flags)
{
	if (!hw_res_parsed)
		return EBADMEM;
	
	hw_resource_list_t hw_resources;
	hw_res_list_parsed_clean(hw_res_parsed);
	bzero(&hw_resources, sizeof(hw_resource_list_t));
	
	int rc = hw_res_get_resource_list(sess, &hw_resources);
	if (rc != EOK)
		return rc;
	
	rc = hw_res_list_parse(&hw_resources, hw_res_parsed, flags);
	hw_res_clean_resource_list(&hw_resources);
	
	return rc;
};

/** @}
 */
