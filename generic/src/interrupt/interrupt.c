/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#include <interrupt.h>
#include <debug.h>
#include <console/kconsole.h>
#include <console/console.h>
#include <console/chardev.h>
#include <console/cmd.h>
#include <panic.h>
#include <print.h>
#include <symtab.h>

static struct {
	const char *name;
	iroutine f;
} exc_table[IVT_ITEMS];

SPINLOCK_INITIALIZE(exctbl_lock);

/** Register exception handler
 * 
 * @param n Exception number
 * @param name Description 
 * @param f Exception handler
 */
iroutine exc_register(int n, const char *name, iroutine f)
{
	ASSERT(n < IVT_ITEMS);
	
	iroutine old;
	
	spinlock_lock(&exctbl_lock);

	old = exc_table[n].f;
	exc_table[n].f = f;
	exc_table[n].name = name;

	spinlock_unlock(&exctbl_lock);	

	return old;
}

/** Dispatch exception according to exception table
 *
 * Called directly from the assembler code.
 * CPU is interrupts_disable()'d.
 */
void exc_dispatch(int n, istate_t *istate)
{
	ASSERT(n < IVT_ITEMS);
	
	exc_table[n].f(n + IVT_FIRST, istate);
}

/** Default 'null' exception handler */
static void exc_undef(int n, istate_t *istate)
{
	panic("Unhandled exception %d.", n);
}

/** kconsole cmd - print all exceptions */
static int exc_print_cmd(cmd_arg_t *argv)
{
	int i;
	char *symbol;

	spinlock_lock(&exctbl_lock);
	printf("Exc Description Handler\n");
	for (i=0; i < IVT_ITEMS; i++) {
		symbol = get_symtab_entry((__native)exc_table[i].f);
		if (!symbol)
			symbol = "not found";
		printf("%d %s %p(%s)\n", i + IVT_FIRST, exc_table[i].name,
		       exc_table[i].f,symbol);		
		if (!((i+1) % 20)) {
			printf("Press any key to continue.");
			spinlock_unlock(&exctbl_lock);
			getc(stdin);
			spinlock_lock(&exctbl_lock);
			printf("\n");
		}
	}
	spinlock_unlock(&exctbl_lock);
	
	return 1;
}

static cmd_info_t exc_info = {
	.name = "exc",
	.description = "Print exception table.",
	.func = exc_print_cmd,
	.help = NULL,
	.argc = 0,
	.argv = NULL
};

/** Initialize generic exception handling support */
void exc_init(void)
{
	int i;

	for (i=0;i < IVT_ITEMS; i++)
		exc_register(i, "undef", (iroutine) exc_undef);

	cmd_initialize(&exc_info);
	if (!cmd_register(&exc_info))
		panic("could not register command %s\n", exc_info.name);
}

