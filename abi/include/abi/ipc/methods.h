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

/** @addtogroup genericipc
 * @{
 */
/** @file
 */

#ifndef ABI_IPC_METHODS_H_
#define ABI_IPC_METHODS_H_

#include <abi/cap.h>

/* Well known phone descriptors */
#define PHONE_NS  (CAP_NIL + 1)

/** Kernel IPC interfaces
 *
 */
#define IPC_IF_KERNEL  0

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

	/** Protocol for initializing callback connections.
	 *
	 * Calling process asks the callee to create a callback connection,
	 * so that it can start initiating new messages.
	 *
	 * The protocol for negotiating is:
	 * - sys_connect_to_me - sends a message IPC_M_CONNECT_TO_ME
	 * - recipient         - upon receipt tries to allocate new phone
	 *                       - if it fails, responds with ELIMIT
	 *                     - passes call to userspace. If userspace
	 *                       responds with error, phone is deallocated and
	 *                       error is sent back to caller. Otherwise 
	 *                       the call is accepted and the response is sent back.
	 *                     - the hash of the allocated phone is passed to userspace
	 *                       (on the receiving side) as ARG5 of the call.
	 */
	IPC_M_CONNECT_TO_ME,

	/** Protocol for initializing new foward connections.
	 *
	 * Calling process asks the callee to create for him a new connection.
	 * E.g. the caller wants a name server to connect him to print server.
	 *
	 * The protocol for negotiating is:
	 * - sys_connect_me_to - send a synchronous message to name server
	 *                       indicating that it wants to be connected to some
	 *                       service
	 *                     - arg1/2/3 are user specified, arg5 contains
	 *                       address of the phone that should be connected
	 *                       (TODO: it leaks to userspace)
	 *  - recipient        - if ipc_answer == 0, then accept connection
	 *                     - otherwise connection refused
	 *                     - recepient may forward message.
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

	/** Receive as_area over IPC.
	 * 
	 * - ARG1 - destination as_area size
	 * - ARG2 - user defined argument
	 *
	 * on answer, the recipient must set:
	 *
	 * - ARG1 - source as_area base address
	 * - ARG2 - flags that will be used for sharing
	 * - ARG3 - dst as_area lower bound
	 * - ARG4 - dst as_area base address (filled automatically by kernel)
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
	 * - ARG2 - dst as_area base adress pointer
	 *          (filled automatically by the kernel)
	 */
	IPC_M_SHARE_OUT,

	/** Receive data from another address space over IPC.
	 *
	 * - ARG1 - destination virtual address in the source address space
	 * - ARG2 - size of data to be received, may be cropped by the recipient 
	 *
	 * on answer, the recipient must set:
	 *
	 * - ARG1 - source virtual address in the destination address space
	 * - ARG2 - final size of data to be copied
	 */
	IPC_M_DATA_READ,

	/** Send data to another address space over IPC.
	 *
	 * - ARG1 - source address space virtual address
	 * - ARG2 - size of data to be copied, may be overriden by the recipient
	 *
	 * on answer, the recipient must set:
	 *
	 * - ARG1 - final destination address space virtual address
	 * - ARG2 - final size of data to be copied
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
#define IPC_M_LAST_SYSTEM  511

#endif

/** @}
 */
