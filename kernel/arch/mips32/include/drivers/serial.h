/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup mips32	
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_SERIAL_H_
#define KERN_mips32_SERIAL_H_

#include <console/chardev.h>

#define SERIAL_ADDRESS    0xB8000000

#define SERIAL_MAX        4
#define SERIAL_COM1       0x3f8
#define SERIAL_COM1_IRQ   4
#define SERIAL_COM2       0x2f8
#define SERIAL_COM2_IRQ   3

#define P_WRITEB(where, what)     (*((volatile char *) (SERIAL_ADDRESS + where)) = what)
#define P_READB(where)            (*((volatile char *) (SERIAL_ADDRESS + where)))

#define SERIAL_READ(x)            P_READB(x)
#define SERIAL_WRITE(x, c)        P_WRITEB(x, c)

/* Interrupt enable register */
#define SERIAL_READ_IER(x)              (P_READB((x) + 1))
#define SERIAL_WRITE_IER(x,c)           (P_WRITEB((x) + 1, c))

/* Interrupt identification register */
#define SERIAL_READ_IIR(x)             (P_READB((x) + 2))

/* Line status register */
#define SERIAL_READ_LSR(x)             (P_READB((x) + 5))
#define TRANSMIT_EMPTY_BIT      5          

typedef struct {
	int port;
	int irq;
}serial_t;

extern void serial_console(devno_t devno);
extern int serial_init(void);

#endif

/** @}
 */
