/*
 * Copyright (C) 2006 Martin Decky
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

/** @addtogroup ppc32mm	
 * @{
 */
/** @file
 */

#ifndef __ppc32_TLB_H__
#define __ppc32_TLB_H__

typedef struct {
	unsigned v : 1;          /**< Valid */
	unsigned vsid : 24;      /**< Virtual Segment ID */
	unsigned h : 1;          /**< Primary/secondary hash */
	unsigned api : 6;        /**< Abbreviated Page Index */
	unsigned rpn : 20;       /**< Real Page Number */
	unsigned reserved0 : 3;
	unsigned r : 1;          /**< Reference */
	unsigned c : 1;          /**< Change */
	unsigned wimg : 4;       /**< Access control */
	unsigned reserved1 : 1;
	unsigned pp : 2;         /**< Page protection */
} phte_t;

extern void pht_refill(int n, istate_t *istate);
extern bool pht_real_refill(int n, istate_t *istate) __attribute__ ((section("K_UNMAPPED_TEXT_START")));
extern void pht_init(void);

#endif

/** @}
 */
