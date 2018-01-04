/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef LIBDRV_OPS_HW_RES_H_
#define LIBDRV_OPS_HW_RES_H_

#include <device/hw_res.h>
#include <stddef.h>
#include <stdint.h>
#include "../ddf/driver.h"

typedef struct {
	hw_resource_list_t *(*get_resource_list)(ddf_fun_t *);
	errno_t (*enable_interrupt)(ddf_fun_t *, int);
	errno_t (*disable_interrupt)(ddf_fun_t *, int);
	errno_t (*clear_interrupt)(ddf_fun_t *, int);
	errno_t (*dma_channel_setup)(ddf_fun_t *, unsigned, uint32_t, uint32_t, uint8_t);
	errno_t (*dma_channel_remain)(ddf_fun_t *, unsigned, size_t *);
} hw_res_ops_t;

#endif

/**
 * @}
 */
