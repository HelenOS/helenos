/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server output
 */

#ifndef TYPES_DISPLAY_OUTPUT_H
#define TYPES_DISPLAY_OUTPUT_H

#include <adt/list.h>
#include <fibril_synch.h>

/** Display server output */
typedef struct ds_output {
	/** Discovery lock */
	fibril_mutex_t lock;
	/** Display devices (of ds_ddev_t) */
	list_t ddevs;
	/** Display to which we will be adding discovered devices */
	struct ds_display *def_display;
} ds_output_t;

#endif

/** @}
 */
