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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_I8259_H_
#define KERN_ia32_I8259_H_

#include <typedefs.h>
#include <arch/interrupt.h>

#define PIC_PIC0PORT1  ((ioport8_t *) 0x20U)
#define PIC_PIC0PORT2  ((ioport8_t *) 0x21U)
#define PIC_PIC1PORT1  ((ioport8_t *) 0xa0U)
#define PIC_PIC1PORT2  ((ioport8_t *) 0xa1U)

/* ICW1 bits */
#define PIC_ICW1           (1 << 4)
#define PIC_ICW1_NEEDICW4  (1 << 0)

/* OCW4 bits */
#define PIC_OCW4           (0 << 3)
#define PIC_OCW4_NSEOI     (1 << 5)

extern void i8259_init(void);
extern void pic_enable_irqs(uint16_t);
extern void pic_disable_irqs(uint16_t);
extern void pic_eoi(void);

#endif

/** @}
 */
