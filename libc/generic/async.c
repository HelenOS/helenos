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

ipc_wait_t call_func(args)
{
}

ipc_wait_t fire_function(args)
{
	stack = malloc(stacksize);
	setup(stack);
	add_to_list_of_ready_funcs(stack);
	if (threads_waiting_for_message)
		send_message_to_self_to_one_up();
}

void discard_result(ipc_wait_t funcid)
{
}

int wait_result(ipc_wait_t funcid);
{
	save_context(self);
restart:
	if result_available() {
		if in_list_of_ready(self):
			tear_off_list(self);
		return retval;
	}
	add_to_waitlist_of(funcid);

	take_something_from_list_of_ready();
	if something {
		
		restore_context(something);
        } else { /* nothing */
		wait_for_call();
		if (answer) {
			mark_result_ready();
			put_waiting_thread_to_waitlist();

			goto restart;
		}
	}
	
}


int ipc_call_sync(args)
{
	return ipc_wait(call_func(args));
}
