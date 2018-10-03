/*
 * Copyright (c) 2017 Jiri Svoboda
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

/** @addtogroup uspace_drv_sun4v_con
 * @{
 */
/** @file
 */

#ifndef SUN4V_CON_H
#define SUN4V_CON_H

#include <async.h>
#include <ddf/driver.h>
#include <io/chardev_srv.h>
#include <loc.h>
#include <stdint.h>

#include "niagara_buf.h"

/** Sun4v console resources */
typedef struct {
	uintptr_t in_base;
	uintptr_t out_base;
} sun4v_con_res_t;

/** Sun4v console */
typedef struct {
	async_sess_t *client_sess;
	ddf_dev_t *dev;
	chardev_srvs_t cds;
	sun4v_con_res_t res;
	/** Virtual address of the shared input buffer */
	niagara_input_buffer_t *input_buffer;
	/** Virtual address of the shared input buffer */
	niagara_output_buffer_t *output_buffer;
} sun4v_con_t;

extern errno_t sun4v_con_add(sun4v_con_t *, sun4v_con_res_t *);
extern errno_t sun4v_con_remove(sun4v_con_t *);
extern errno_t sun4v_con_gone(sun4v_con_t *);

#endif

/** @}
 */
