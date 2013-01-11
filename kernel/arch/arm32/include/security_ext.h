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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Security Extensions Routines
 */

#ifndef KERN_arm32_SECURITY_EXT_H_
#define KERN_arm32_SECURITY_EXT_H_

#include <arch/cp15.h>
#include <arch/regutils.h>

static inline bool sec_ext_is_implemented()
{
#ifdef PROCESSOR_armv7_a
	const uint32_t idpfr = ID_PFR1_read() & ID_PFR1_SEC_EXT_MASK;
	return idpfr == ID_PFR1_SEC_EXT || idpfr == ID_PFR1_SEC_EXT_RFR;
#endif
	return false;
}

static inline bool sec_ext_is_monitor_mode()
{
	return (current_status_reg_read() & MODE_MASK) == MONITOR_MODE;
}

static inline bool sec_ext_is_secure()
{
	return sec_ext_is_implemented()
	    && (sec_ext_is_monitor_mode() || !(SCR_read() & SCR_NS_FLAG));
}

typedef enum {
	SECURITY_CALL_ENABLE_CP10_11 = 0xaaaa
} sec_ext_call_t;

static inline void sec_ext_call(sec_ext_call_t call)
{
	asm volatile ("mov r0, %0\nsmc #0" ::"r"(call));
}

int sec_ext_handle_call(sec_ext_call_t call);

#endif

/** @}
 */

