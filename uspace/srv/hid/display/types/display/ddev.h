/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server display device type
 */

#ifndef TYPES_DISPLAY_DDEV_H
#define TYPES_DISPLAY_DDEV_H

#include <adt/list.h>
#include <ddev.h>
#include <gfx/context.h>
#include <loc.h>

/** Display server display device */
typedef struct ds_ddev {
	/** Parent display */
	struct ds_display *display;
	/** Link to display->ddevs */
	link_t lddevs;
	/** Link to output->ddevs */
	link_t loutdevs;
	/** Device GC */
	gfx_context_t *gc;
	/** Display device information */
	ddev_info_t info;
	/** Service ID */
	service_id_t svc_id;
	/** Service name */
	char *svc_name;
	/** Underlying display device */
	ddev_t *dd;
} ds_ddev_t;

#endif

/** @}
 */
