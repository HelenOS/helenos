/*
 * Copyright (c) 2010 Jan Vesely
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

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */

#ifndef DRV_UHCI_HW_STRUCT_LINK_POINTER_H
#define DRV_UHCI_HW_STRUCT_LINK_POINTER_H

#include <stdint.h>

/** UHCI link pointer, used by many data structures */
typedef uint32_t link_pointer_t;

#define LINK_POINTER_TERMINATE_FLAG (1 << 0)
#define LINK_POINTER_QUEUE_HEAD_FLAG (1 << 1)
#define LINK_POINTER_ZERO_BIT_FLAG (1 << 2)
#define LINK_POINTER_VERTICAL_FLAG (1 << 2)
#define LINK_POINTER_RESERVED_FLAG (1 << 3)

#define LINK_POINTER_ADDRESS_MASK 0xfffffff0 /* upper 28 bits */

#define LINK_POINTER_QH(address) \
	((address & LINK_POINTER_ADDRESS_MASK) | LINK_POINTER_QUEUE_HEAD_FLAG)

#define LINK_POINTER_TD(address) \
	(address & LINK_POINTER_ADDRESS_MASK)

#define LINK_POINTER_TERM \
	((link_pointer_t)LINK_POINTER_TERMINATE_FLAG)

#endif

/**
 * @}
 */
