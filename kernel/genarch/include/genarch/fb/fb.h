/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#ifndef KERN_FB_H_
#define KERN_FB_H_

#include <typedefs.h>
#include <abi/fb/visuals.h>
#include <console/chardev.h>

/**
 * Properties of the framebuffer device.
 */
typedef struct fb_properties {
	/** Physical address of the framebuffer device. */
	uintptr_t addr;

	/**
	 * Address where the first (top left) pixel is mapped,
	 * relative to "addr".
	 */
	unsigned int offset;

	/** Screen width in pixels. */
	unsigned int x;

	/** Screen height in pixels. */
	unsigned int y;

	/** Bytes per one scanline. */
	unsigned int scan;

	/** Color model. */
	visual_t visual;
} fb_properties_t;

extern outdev_t *fb_init(fb_properties_t *props);

#endif

/** @}
 */
