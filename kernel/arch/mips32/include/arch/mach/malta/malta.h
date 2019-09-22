/*
 * Copyright (c) 2013 Jakub Jermar
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

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *  @brief MIPS Malta platform driver.
 */

#ifndef KERN_mips32_malta_H_
#define KERN_mips32_malta_H_

#include <arch/machine_func.h>
#include <arch/mm/page.h>
#include <typedefs.h>

#define MALTA_PCI_PHYSBASE	0x18000000UL

#define MALTA_PCI_BASE		PA2KSEG1(MALTA_PCI_PHYSBASE)
#define MALTA_GT64120_BASE	PA2KSEG1(0x1be00000UL)

#define PIC0_BASE		(MALTA_PCI_BASE + 0x20)
#define PIC1_BASE		(MALTA_PCI_BASE + 0xa0)

#define ISA_IRQ_COUNT		16

#define TTY_BASE		(MALTA_PCI_PHYSBASE + 0x3f8)
#define TTY_ISA_IRQ		4

#define GT64120_PCI0_INTACK	((ioport32_t *) (MALTA_GT64120_BASE + 0xc34))

extern struct mips32_machine_ops malta_machine_ops;

#endif

/** @}
 */
