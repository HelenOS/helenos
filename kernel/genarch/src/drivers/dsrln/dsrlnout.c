/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Dummy serial line output.
 */

#include <genarch/drivers/dsrln/dsrlnout.h>
#include <console/chardev.h>
#include <arch/asm.h>
#include <stdlib.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <str.h>
#include <ddi/ddi.h>

typedef struct {
	parea_t parea;
	ioport8_t *base;
} dsrlnout_instance_t;

static void dsrlnout_write(outdev_t *dev, const char *s, size_t n)
{
	dsrlnout_instance_t *instance = (dsrlnout_instance_t *) dev->data;

	if (instance->parea.mapped && !console_override)
		return;

	const char *top = s + n;
	assert(top >= s);

	for (; s < top; s++) {
		if (*s == '\n')
			pio_write_8(instance->base, '\r');

		pio_write_8(instance->base, (uint8_t) *s);
	}
}

static outdev_operations_t dsrlndev_ops = {
	.write = dsrlnout_write,
	.redraw = NULL,
	.scroll_up = NULL,
	.scroll_down = NULL
};

outdev_t *dsrlnout_init(ioport8_t *base, uintptr_t base_phys)
{
	outdev_t *dsrlndev = malloc(sizeof(outdev_t));
	if (!dsrlndev)
		return NULL;

	dsrlnout_instance_t *instance = malloc(sizeof(dsrlnout_instance_t));
	if (!instance) {
		free(dsrlndev);
		return NULL;
	}

	outdev_initialize("dsrlndev", dsrlndev, &dsrlndev_ops);
	dsrlndev->data = instance;

	instance->base = base;
	ddi_parea_init(&instance->parea);
	instance->parea.pbase = base_phys;
	instance->parea.frames = 1;
	instance->parea.unpriv = false;
	instance->parea.mapped = false;
	ddi_parea_register(&instance->parea);

	if (!fb_exported) {
		/*
		 * This is the necessary evil until
		 * the userspace driver is entirely
		 * self-sufficient.
		 */
		sysinfo_set_item_val("fb", NULL, true);
		sysinfo_set_item_val("fb.kind", NULL, 3);
		sysinfo_set_item_val("fb.address.physical", NULL, KA2PA(base));

		fb_exported = true;
	}

	return dsrlndev;
}

/** @}
 */
