/*
 * SPDX-FileCopyrightText: 2017 Jaroslav Jindrak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * Scratchpad buffers are PAGE_SIZE sized page boundary aligned buffers
 * that are free to use by the xHC.
 *
 * This file provides means of allocation and deallocation of these
 * buffers.
 */

#ifndef XHCI_SCRATCHPAD_H
#define XHCI_SCRATCHPAD_H

#include <usb/dma_buffer.h>

typedef struct xhci_hc xhci_hc_t;

extern errno_t xhci_scratchpad_alloc(xhci_hc_t *);
extern void xhci_scratchpad_free(xhci_hc_t *);

#endif

/**
 * @}
 */
