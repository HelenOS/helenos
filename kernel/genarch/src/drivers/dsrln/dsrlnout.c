/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void dsrlnout_putuchar(outdev_t *dev, const char32_t ch)
{
	dsrlnout_instance_t *instance = (dsrlnout_instance_t *) dev->data;

	if ((!instance->parea.mapped) || (console_override)) {
		if (ascii_check(ch))
			pio_write_8(instance->base, ch);
		else
			pio_write_8(instance->base, U_SPECIAL);
	}
}

static outdev_operations_t dsrlndev_ops = {
	.write = dsrlnout_putuchar,
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
