/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup libdrv
 * @{
 */

/** @file
 */

#include <async.h>
#include <errno.h>
#include <macros.h>

#include "ddf/interrupt.h"
#include "private/driver.h"

errno_t register_interrupt_handler(ddf_dev_t *dev, int irq,
    interrupt_handler_t *handler, const irq_code_t *irq_code,
    cap_irq_handle_t *handle)
{
	return async_irq_subscribe(irq, (async_notification_handler_t) handler,
	    dev, irq_code, handle);
}

errno_t unregister_interrupt_handler(ddf_dev_t *dev, cap_irq_handle_t handle)
{
	return async_irq_unsubscribe(handle);
}

/**
 * @}
 */
