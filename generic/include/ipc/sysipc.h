/*
 * Copyright (C) 2006 Ondrej Palkovsky
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

#ifndef __SYSIPC_H__
#define __SYSIPC_H__

#include <ipc/ipc.h>

__native sys_ipc_call_sync_fast(__native phoneid, __native method, 
				__native arg1, ipc_data_t *data);
__native sys_ipc_call_sync(__native phoneid, ipc_data_t *question, 
			   ipc_data_t *reply);
__native sys_ipc_call_async_fast(__native phoneid, __native method, 
				 __native arg1, __native arg2);
__native sys_ipc_call_async(__native phoneid, ipc_data_t *data);
__native sys_ipc_answer_fast(__native callid, __native retval, 
			     __native arg1, __native arg2);
__native sys_ipc_answer(__native callid, ipc_data_t *data);
__native sys_ipc_wait_for_call(ipc_data_t *calldata, __native flags);
__native sys_ipc_forward_fast(__native callid, __native phoneid,
			      __native method, __native arg1);
__native sys_ipc_hangup(int phoneid);
__native sys_ipc_register_irq(__native irq);
__native sys_ipc_unregister_irq(__native irq);

void irq_ipc_bind_arch(__native irq);

#endif
