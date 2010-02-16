/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Generic module skeleton implementation.
 */

#include <async.h>
#include <stdio.h>
#include <task.h>

#include <ipc/ipc.h>

#include "err.h"
#include "modules.h"

/** @name External module functions.
 *  This functions have to be implemented in every module.
 */
/*@{*/

/** External message processing function.
 *  Should process the messages.
 *  The function has to be defined in each module.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @returns Other error codes as defined for each specific module message function.
 */
extern int	module_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count );

/** External function to print the module name.
 *  Should print the module name.
 *  The function has to be defined in each module.
 */
extern void	module_print_name( void );

/** External module startup function.
 *  Should start and initialize the module and register the given client connection function.
 *  The function has to be defined in each module.
 *  @param[in] client_connection The client connection function to be registered.
 */
extern int	module_start( async_client_conn_t client_connection );

/*@}*/

/** Default thread for new connections.
 *  @param[in] iid The initial message identifier.
 *  @param[in] icall The initial message call structure.
 */
void	client_connection( ipc_callid_t iid, ipc_call_t * icall );

/**	Starts the module.
 *  @param argc The count of the command line arguments. Ignored parameter.
 *  @param argv The command line parameters. Ignored parameter.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for each specific module start function.
 */
int	main( int argc, char * argv[] );

void client_connection( ipc_callid_t iid, ipc_call_t * icall ){
	ipc_callid_t	callid;
	ipc_call_t		call;
	ipc_call_t		answer;
	int				answer_count;
	int				res;

	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	ipc_answer_0( iid, EOK );

	while( true ){
		refresh_answer( & answer, & answer_count );

		callid = async_get_call( & call );
		res = module_message( callid, & call, & answer, & answer_count );

		if( IPC_GET_METHOD( call ) == IPC_M_PHONE_HUNGUP ) return;

		answer_call( callid, res, & answer, answer_count );
	}
}

int main( int argc, char * argv[] ){
	ERROR_DECLARE;

	printf("Task %d - ", task_get_id());
	module_print_name();
	printf( "\n" );
	if( ERROR_OCCURRED( module_start( client_connection ))){
		printf( " - ERROR %i\n", ERROR_CODE );
		return ERROR_CODE;
	}
	return EOK;
}

/** @}
 */
