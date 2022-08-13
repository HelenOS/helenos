/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef DDF_INTERRUPT_H_
#define DDF_INTERRUPT_H_

#include <types/common.h>
#include <abi/ddi/irq.h>
#include <adt/list.h>
#include <ddi.h>
#include <fibril_synch.h>
#include "driver.h"
#include "../dev_iface.h"

/*
 * Interrupts
 */

typedef void interrupt_handler_t(ipc_call_t *, ddf_dev_t *);

extern errno_t register_interrupt_handler(ddf_dev_t *, int, interrupt_handler_t *,
    const irq_code_t *, cap_irq_handle_t *);
extern errno_t unregister_interrupt_handler(ddf_dev_t *, cap_irq_handle_t);

#endif

/**
 * @}
 */
