/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
