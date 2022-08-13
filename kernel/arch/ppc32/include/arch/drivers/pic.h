/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_PIC_H_
#define KERN_ppc32_PIC_H_

#include <typedefs.h>
#include <ddi/irq.h>

#define PIC_PENDING_LOW   8
#define PIC_PENDING_HIGH  4
#define PIC_MASK_LOW      9
#define PIC_MASK_HIGH     5
#define PIC_ACK_LOW       10
#define PIC_ACK_HIGH      6

extern void pic_init(uintptr_t, size_t, cir_t *, void **);
extern void pic_enable_interrupt(inr_t);
extern void pic_disable_interrupt(inr_t);
extern void pic_ack_interrupt(void *, inr_t);
extern uint8_t pic_get_pending(void);

#endif

/** @}
 */
