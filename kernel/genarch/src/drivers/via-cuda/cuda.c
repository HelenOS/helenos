/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup genarch
 * @{
 */
/** @file
 */

#include <genarch/drivers/via-cuda/cuda.h>
#include <console/chardev.h>
#include <ddi/irq.h>
#include <arch/asm.h>
#include <mm/slab.h>
#include <ddi/device.h>

static irq_ownership_t cuda_claim(irq_t *irq)
{
	return IRQ_DECLINE;
}

static void cuda_irq_handler(irq_t *irq)
{
}

cuda_instance_t *cuda_init(cuda_t *dev, inr_t inr, cir_t cir, void *cir_arg)
{
	cuda_instance_t *instance
	    = malloc(sizeof(cuda_instance_t), FRAME_ATOMIC);
	if (instance) {
		instance->cuda = dev;
		instance->kbrdin = NULL;
		
		irq_initialize(&instance->irq);
		instance->irq.devno = device_assign_devno();
		instance->irq.inr = inr;
		instance->irq.claim = cuda_claim;
		instance->irq.handler = cuda_irq_handler;
		instance->irq.instance = instance;
		instance->irq.cir = cir;
		instance->irq.cir_arg = cir_arg;
	}
	
	return instance;
}

void cuda_wire(cuda_instance_t *instance, indev_t *kbrdin)
{
}


/** @}
 */
