/*
 * Copyright (c) 2001-2004 Jakub Jermar
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
/** @file
 */

#ifndef KERN_I8259_H_
#define KERN_I8259_H_

#include <typedefs.h>
#include <arch/interrupt.h>
#include <stdbool.h>

/* ICW1 bits */
#define PIC_ICW1           (1 << 4)
#define PIC_ICW1_NEEDICW4  (1 << 0)

/* OCW3 bits */
#define PIC_OCW3           (1 << 3)
#define PIC_OCW3_READ_ISR  (3 << 0)

/* OCW4 bits */
#define PIC_OCW4           (0 << 3)
#define PIC_OCW4_NSEOI     (1 << 5)

#define PIC0_IRQ_COUNT      8
#define PIC1_IRQ_COUNT      8

#define PIC0_IRQ_PIC1       2

typedef struct {
	ioport8_t port1;
	ioport8_t port2;
} __attribute__((packed)) i8259_t;

extern void i8259_init(i8259_t *, i8259_t *, unsigned int);
extern void pic_enable_irqs(uint16_t);
extern void pic_disable_irqs(uint16_t);
extern void pic_eoi(unsigned int);
extern bool pic_is_spurious(unsigned int);
extern void pic_handle_spurious(unsigned int);

#endif

/** @}
 */
