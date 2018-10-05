/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup kernel_ppc32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_TLB_H_
#define KERN_ppc32_TLB_H_

#include <arch/interrupt.h>

#define WIMG_GUARDED    0x01
#define WIMG_COHERENT   0x02
#define WIMG_NO_CACHE   0x04
#define WIMG_WRITETHRU  0x08

typedef struct {
	unsigned int v : 1;          /**< Valid */
	unsigned int vsid : 24;      /**< Virtual Segment ID */
	unsigned int h : 1;          /**< Primary/secondary hash */
	unsigned int api : 6;        /**< Abbreviated Page Index */
	unsigned int rpn : 20;       /**< Real Page Number */
	unsigned int reserved0 : 3;
	unsigned int r : 1;          /**< Reference */
	unsigned int c : 1;          /**< Change */
	unsigned int wimg : 4;       /**< Access control */
	unsigned int reserved1 : 1;
	unsigned int pp : 2;         /**< Page protection */
} phte_t;

typedef struct {
	unsigned int v : 1;
	unsigned int vsid : 24;
	unsigned int reserved0 : 1;
	unsigned int api : 6;
} ptehi_t;

typedef struct {
	unsigned int rpn : 20;
	unsigned int xpn : 3;
	unsigned int reserved0 : 1;
	unsigned int c : 1;
	unsigned int wimg : 4;
	unsigned int x : 1;
	unsigned int pp : 2;
} ptelo_t;

extern void tlb_refill(unsigned int, istate_t *);

#endif

/** @}
 */
