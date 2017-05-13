/*
 * Copyright (c) 2013 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */
#ifndef DRV_EHCI_HW_STRUCT_LINK_POINTER_H
#define DRV_EHCI_HW_STRUCT_LINK_POINTER_H

#include <stdint.h>

/** EHCI link pointer, used by many data structures */
typedef volatile uint32_t link_pointer_t;

#define LINK_POINTER_ADDRESS_MASK   0xfffffff0 /* upper 28 bits */

#define LINK_POINTER_TERMINATE_FLAG   (1 << 0)

enum {
	LINK_POINTER_TYPE_iTD  = 0x0 << 1,
	LINK_POINTER_TYPE_QH   = 0x1 << 1,
	LINK_POINTER_TYPE_siTD = 0x2 << 1,
	LINK_POINTER_TYPE_FSTN = 0x3 << 1,
	LINK_POINTER_TYPE_MASK = 0x3 << 1,
};

#define LINK_POINTER_QH(address) \
	((address & LINK_POINTER_ADDRESS_MASK) | LINK_POINTER_TYPE_QH)

#define LINK_POINTER_TD(address) \
	(address & LINK_POINTER_ADDRESS_MASK)

#define LINK_POINTER_TERM \
	((link_pointer_t)LINK_POINTER_TERMINATE_FLAG)

#endif
/**
 * @}
 */
