/*
 * Copyright (c) 2012 Adam Hraska
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

/** @addtogroup test
 * @{
 */

/**
 * @file rcutest.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mem.h>
#include <errno.h>
#include <thread.h>
#include <assert.h>

#include <fibril.h>

#include <rcu.h>


static void invoke_rcu(void)
{
	rcu_read_lock();
	rcu_read_lock();
	/* nop */
	rcu_read_unlock();
	rcu_read_unlock();
	
	rcu_synchronize();
	
	printf("Woohoo, did not crash after invoking rcu functions.\n");
}

static void print_usage(void)
{
	printf("Usage: rcutest \n");
}


int main(int argc, char **argv)
{
	if (argc >= 2) {
		print_usage();
		return 1;
	}
	
	rcu_register_fibril();
	
	invoke_rcu();
	/* todo: impl */
	
	rcu_deregister_fibril();
	
	return 0;
}


/**
 * @}
 */
