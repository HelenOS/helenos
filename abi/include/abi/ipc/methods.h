/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_IPC_METHODS_H_
#define _ABI_IPC_METHODS_H_

#include <abi/cap.h>

/* Well known phone descriptors */
static cap_phone_handle_t const PHONE_NS = (cap_phone_handle_t) (CAP_NIL + 1);

/** Kernel IPC interfaces
 *
 */
enum {
	IPC_IF_KERNEL = 0,
};

/** System-specific IPC methods
 *
 * These methods have special behaviour. These methods also
 * have the implicit kernel interface zero (0).
 *
 */
enum {
	/** This message is sent to answerbox when the phone is hung up
	 *
	 * The numerical value zero (0) of this method is important,
	 * so as the value can be easily tested in conditions.
	 */
	IPC_M_PHONE_HUNGUP = 0,

	/** Protocol for initializing new callback connections.
	 *
	 * Sender asks the recipient to create a new connection from
	 * the recipient to the sender.
	 *
	 * Sender:
	 *  - uspace: arg1 .. callback iface
	 *            arg2 .. <custom>
	 *            arg3 .. <custom>
	 *            arg4 .. <unused>
	 *            arg5 .. sender-assigned label
	 *  - kernel: arg5 .. new recipient's connection phone capability
	 *
	 * recipient:
	 *  - uspace: arg1 .. <unused>
	 *            arg2 .. <unused>
	 *            arg3 .. <unused>
	 *            arg4 .. <unused>
	 *            arg5 .. <unused>
	 *
	 */
	IPC_M_CONNECT_TO_ME,

	/** Protocol for initializing new forward connections.
	 *
	 * Sender asks the recipient to create a new connection from
	 * the sender to the recipient. The message can be forwarded,
	 * thus the immediate recipient acts as a broker and the connection
	 * is created to the final recipient.
	 *
	 * Sender:
	 *  - uspace: arg1 .. iface
	 *            arg2 .. <custom>
	 *            arg3 .. <custom>
	 *            arg4 .. flags (e.g. IPC_FLAG_BLOCKING)
	 *            arg5 .. unused
	 *
	 * Recipient:
	 *  - uspace: arg1 .. <unused>
	 *            arg2 .. <unused>
	 *            arg3 .. <unused>
	 *            arg4 .. <unused>
	 *            arg5 .. recipient-assigned label
	 *  - kernel: arg5 .. new sender's connection phone capability
	 *
	 */
	IPC_M_CONNECT_ME_TO,

	/** Share a single page over IPC.
	 *
	 * - ARG1 - page-aligned offset from the beginning of the memory object
	 * - ARG2 - page size
	 * - ARG3 - user defined memory object ID
	 * - ARG4 - user defined memory object ID
	 * - ARG5 - user defined memory object ID
	 *
	 * on answer, the recipient must set:
	 *
	 * - ARG1 - source user page address
	 */
	IPC_M_PAGE_IN,

	/** Receive an address space area over IPC.
	 *
	 * Sender:
	 *  - uspace: arg1 .. address space area size
	 *            arg2 .. sender's address space area starting address
	 *                    lower bound
	 *            arg3 .. <custom>
	 *            arg4 .. <unused>
	 *            arg5 .. <unused>
	 *
	 * Recipient:
	 *  - uspace: arg1 .. recipient's address space area starting address
	 *            arg2 .. shared address space areas sharing flags
	 *            arg3 .. <unused>
	 *            arg4 .. <unused>
	 *  - kernel: arg5 .. new sender's address space area starting address
	 *
	 */
	IPC_M_SHARE_IN,

	/** Send as_area over IPC.
	 *
	 * - ARG1 - source as_area base address
	 * - ARG2 - size of source as_area (filled automatically by kernel)
	 * - ARG3 - flags of the as_area being sent
	 *
	 * on answer, the recipient must set:
	 *
	 * - ARG1 - dst as_area lower bound
	 * - ARG2 - dst as_area base address pointer
	 *          (filled automatically by the kernel)
	 */
	IPC_M_SHARE_OUT,

	/** Receive data from another address space over IPC.
	 *
	 * Sender:
	 *  - uspace: arg1 .. sender's destination buffer address
	 *            arg2 .. sender's destination buffer size
	 *            arg3 .. flags (IPC_XF_RESTRICT)
	 *            arg4 .. <unused>
	 *            arg5 .. <unused>
	 *
	 * Recipient:
	 *  - uspace: arg1 .. recipient's source buffer address
	 *            arg2 .. recipient's source buffer size
	 *            arg3 .. <unused>
	 *            arg4 .. <unused>
	 *            arg5 .. <unused>
	 *
	 */
	IPC_M_DATA_READ,

	/** Send data to another address space over IPC.
	 *
	 * Sender:
	 *  - uspace: arg1 .. sender's source buffer address
	 *            arg2 .. sender's source buffer size
	 *            arg3 .. flags (IPC_XF_RESTRICT)
	 *            arg4 .. <unused>
	 *            arg5 .. <unused>
	 *
	 * Recipient:
	 *  - uspace: arg1 .. recipient's destination buffer address
	 *            arg2 .. recipient's destination buffer size
	 *            arg3 .. <unused>
	 *            arg4 .. <unused>
	 *            arg5 .. <unused>
	 *
	 */
	IPC_M_DATA_WRITE,

	/** Authorize change of recipient's state in a third party task.
	 *
	 * - ARG1 - user protocol defined data
	 * - ARG2 - user protocol defined data
	 * - ARG3 - user protocol defined data
	 * - ARG5 - sender's phone to the third party task
	 *
	 * on EOK answer, the recipient must set:
	 *
	 * - ARG1 - recipient's phone to the third party task
	 */
	IPC_M_STATE_CHANGE_AUTHORIZE,

	/** Debug the recipient.
	 *
	 * - ARG1 - specifies the debug method (from udebug_method_t)
	 * - other arguments are specific to the debug method
	 */
	IPC_M_DEBUG,
};

/** Last system IPC method */
enum {
	IPC_M_LAST_SYSTEM = 511,
};

#endif

/** @}
 */
