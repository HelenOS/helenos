/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief Keyboard layout interface.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef KBD_LAYOUT_H_
#define KBD_LAYOUT_H_

#include <io/console.h>

/** Layout instance state */
typedef struct layout {
	/** Ops structure */
	struct layout_ops *ops;

	/* Layout-private data */
	void *layout_priv;
} layout_t;

/** Layout ops */
typedef struct layout_ops {
	errno_t (*create)(layout_t *);
	void (*destroy)(layout_t *);
	char32_t (*parse_ev)(layout_t *, kbd_event_t *);
} layout_ops_t;

extern layout_ops_t us_qwerty_ops;
extern layout_ops_t us_dvorak_ops;
extern layout_ops_t cz_ops;
extern layout_ops_t ar_ops;
extern layout_ops_t fr_azerty_ops;

extern layout_t *layout_create(layout_ops_t *);
extern void layout_destroy(layout_t *);
extern char32_t layout_parse_ev(layout_t *, kbd_event_t *);

#endif

/**
 * @}
 */
