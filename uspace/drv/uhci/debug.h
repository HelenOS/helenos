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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_DEBUG_H
#define DRV_UHCI_DEBUG_H

#include <usb/debug.h>

#include "name.h"

enum debug_levels {
	DEBUG_LEVEL_FATAL_ERROR = 1,
	DEBUG_LEVEL_ERROR = 2,
	DEBUG_LEVEL_WARNING = 3,
	DEBUG_LEVEL_INFO = 4,
	DEBUG_LEVEL_VERBOSE = 5,
	DEBUG_LEVEL_MAX = DEBUG_LEVEL_VERBOSE
};

#define uhci_printf( level, fmt, args...) \
	usb_dprintf( NAME, level, fmt, ##args )

#define uhci_print_error( fmt, args... ) \
	usb_dprintf( NAME, DEBUG_LEVEL_ERROR, fmt, ##args )

#define uhci_print_info( fmt, args... ) \
	usb_dprintf( NAME, DEBUG_LEVEL_INFO, fmt, ##args )

#define uhci_print_verbose( fmt, args... ) \
	usb_dprintf( NAME, DEBUG_LEVEL_VERBOSE, fmt, ##args )

#define UHCI_GET_STR_FLAG(reg, flag, msg_set, msg_unset) \
	((((reg) & (flag)) > 0) ? (msg_set) : (msg_unset))


#endif
/**
 * @}
 */
