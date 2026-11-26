/*
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup uspace_drv_dwapb
 * @{
 */
/** @file
 * @brief Synopsys DesignWare DW_apb_uart driver.
 */

#ifndef DWAPB_H_
#define DWAPB_H_

#include <adt/circ_buf.h>
#include <ddi.h>
#include <fibril_synch.h>
#include <io/chardev_srv.h>
#include <ddf/driver.h>

enum {
	dwapb_buf_size = 128
};

typedef struct {
	uintptr_t base;
	int irq;
} dwapb_res_t;

typedef struct {
	/** DDF device */
	ddf_dev_t *dev;
	/** Character device service structure */
	chardev_srvs_t cds;
	/** Hardware resources */
	dwapb_res_t res;
	/** IRQ code */
	irq_code_t irq_code;
	/** Circular buffer control structure */
	circ_buf_t cbuf;
	/** Circular buffer for received data */
	uint8_t buf[dwapb_buf_size];
	/** Buffer access lock */
	fibril_mutex_t buf_lock;
	/** Buffer condition variable */
	fibril_condvar_t buf_cv;
	/** Memory-mapped registers */
	void *regs;
	/** IRQ handle */
	cap_irq_handle_t irq_handle;
} dwapb_t;

extern errno_t dwapb_add(dwapb_t *, dwapb_res_t *);
extern errno_t dwapb_remove(dwapb_t *);
extern errno_t dwapb_gone(dwapb_t *);

#endif

/** @}
 */
