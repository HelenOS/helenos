/*
 * Copyright (c) 2011 Jan Vesely
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
/**  @addtogroup libusbhost
 * @{
 */
/** @file
 *
 * Bandwidth calculation functions. Shared among uhci, ohci and ehci drivers.
 */

#ifndef LIBUSBHOST_HOST_BANDWIDTH_H
#define LIBUSBHOST_HOST_BANDWIDTH_H

#include <usb/usb.h>

#include <stddef.h>

/** Bytes per second in FULL SPEED */
#define BANDWIDTH_TOTAL_USB11 (12000000 / 8)
/** 90% of total bandwidth is available for periodic transfers */
#define BANDWIDTH_AVAILABLE_USB11 ((BANDWIDTH_TOTAL_USB11 * 9) / 10)

/** Number of nanoseconds in one microframe */
#define BANDWIDTH_TOTAL_USB20 (125000)
/** 90% of total bandwidth is available for periodic transfers */
#define BANDWIDTH_AVAILABLE_USB20  ((BANDWIDTH_TOTAL_USB20 * 9) / 10)

typedef struct endpoint endpoint_t;

extern ssize_t bandwidth_count_usb11(endpoint_t *, size_t);

extern ssize_t bandwidth_count_usb20(endpoint_t *, size_t);

#endif
/**
 * @}
 */
