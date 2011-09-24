/*
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

#include <errno.h>

#include "sb16.h"

/* ISA interrupts should be edge-triggered so there should be no need for
 * irq code magic */
static const irq_cmd_t irq_cmds[] = {{ .cmd = CMD_ACCEPT }};
static const irq_code_t irq_code =
    { .cmdcount = 1, .cmds = (irq_cmd_t*)irq_cmds };

/*----------------------------------------------------------------------------*/
irq_code_t * sb16_irq_code(void)
{
	return (irq_code_t*)&irq_code;
}
/*----------------------------------------------------------------------------*/
int sb16_init_sb16(sb16_drv_t *drv, void *regs, size_t size)
{
	// TODO Setup registers
	// TODO Initialize dsp: get version number
	// TODO Initialize dsp: reset DSP
	// TODO Initialize mixer
	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb16_init_mpu(sb16_drv_t *drv, void *regs, size_t size)
{
	return ENOTSUP;
}
