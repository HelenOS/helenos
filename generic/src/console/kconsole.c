/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <console/kconsole.h>
#include <console/console.h>
#include <console/chardev.h>
#include <print.h>
#include <panic.h>
#include <typedefs.h>
#include <arch/types.h>
#include <list.h>
#include <arch.h>
#include <func.h>
#include <macros.h>
#include <debug.h>
#include <symtab.h>

#define MAX_CMDLINE	256

/** Simple kernel console.
 *
 * The console is realized by kernel thread kconsole.
 * It doesn't understand any useful command on its own,
 * but makes it possible for other kernel subsystems to
 * register their own commands.
 */
 
/** Locking.
 *
 * There is a list of cmd_info_t structures. This list
 * is protected by cmd_lock spinlock. Note that specially
 * the link elements of cmd_info_t are protected by
 * this lock.
 *
 * Each cmd_info_t also has its own lock, which protects
 * all elements thereof except the link element.
 *
 * cmd_lock must be acquired before any cmd_info lock.
 * When locking two cmd info structures, structure with
 * lower address must be locked first.
 */
 
spinlock_t cmd_lock;	/**< Lock protecting command list. */
link_t cmd_head;	/**< Command list. */

static cmd_info_t *parse_cmdline(char *cmdline, size_t len);
static bool parse_argument(char *cmdline, size_t len, index_t *start, index_t *end);

/** Data and methods for 'help' command. */
static int cmd_help(cmd_arg_t *argv);
static cmd_info_t help_info = {
	.name = "help",
	.description = "List of supported commands.",
	.func = cmd_help,
	.argc = 0
};

/** Data and methods for 'description' command. */
static int cmd_desc(cmd_arg_t *argv);
static void desc_help(void);
static char desc_buf[MAX_CMDLINE+1];
static cmd_arg_t desc_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = desc_buf,
	.len = sizeof(desc_buf)
};
static cmd_info_t desc_info = {
	.name = "describe",
	.description = "Describe specified command.",
	.help = desc_help,
	.func = cmd_desc,
	.argc = 1,
	.argv = &desc_argv
};

/** Data and methods for 'symaddr' command. */
static int cmd_symaddr(cmd_arg_t *argv);
static char symaddr_buf[MAX_CMDLINE+1];
static cmd_arg_t symaddr_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = symaddr_buf,
	.len = sizeof(symaddr_buf)
};
static cmd_info_t symaddr_info = {
	.name = "symaddr",
	.description = "Return symbol address.",
	.func = cmd_symaddr,
	.argc = 1,
	.argv = &symaddr_argv
};

/** Call0 - call function with no parameters */
static char call0_buf[MAX_CMDLINE+1];
static char carg1_buf[MAX_CMDLINE+1];
static char carg2_buf[MAX_CMDLINE+1];

static int cmd_call0(cmd_arg_t *argv);
static cmd_arg_t call0_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = call0_buf,
	.len = sizeof(call0_buf)
};
static cmd_info_t call0_info = {
	.name = "call0",
	.description = "call0 <function> -> call function().",
	.func = cmd_call0,
	.argc = 1,
	.argv = &call0_argv
};

static int cmd_call1(cmd_arg_t *argv);
static cmd_arg_t call1_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = call0_buf,
		.len = sizeof(call0_buf)
	},
	{ 
		.type = ARG_TYPE_VAR,
		.buffer = carg1_buf,
		.len = sizeof(carg1_buf)
	}
};
static cmd_info_t call1_info = {
	.name = "call1",
	.description = "call1 <function> <arg1> -> call function(arg1).",
	.func = cmd_call1,
	.argc = 2,
	.argv = call1_argv
};

static int cmd_call2(cmd_arg_t *argv);
static cmd_arg_t call2_argv[] = {
	{
		.type = ARG_TYPE_STRING,
		.buffer = call0_buf,
		.len = sizeof(call0_buf)
	},
	{ 
		.type = ARG_TYPE_VAR,
		.buffer = carg1_buf,
		.len = sizeof(carg1_buf)
	},
	{ 
		.type = ARG_TYPE_VAR,
		.buffer = carg2_buf,
		.len = sizeof(carg2_buf)
	}
};
static cmd_info_t call2_info = {
	.name = "call2",
	.description = "call2 <function> <arg1> <arg2> -> call function(arg1,arg2).",
	.func = cmd_call2,
	.argc = 3,
	.argv = call2_argv
};


/** Data and methods for 'halt' command. */
static int cmd_halt(cmd_arg_t *argv);
static cmd_info_t halt_info = {
	.name = "halt",
	.description = "Halt the kernel.",
	.func = cmd_halt,
	.argc = 0
};

/** Initialize kconsole data structures. */
void kconsole_init(void)
{
	spinlock_initialize(&cmd_lock, "kconsole_cmd");
	list_initialize(&cmd_head);
	
	spinlock_initialize(&help_info.lock, "kconsole_help");
	link_initialize(&help_info.link);
	if (!cmd_register(&help_info))
		panic("could not register command %s\n", help_info.name);


	spinlock_initialize(&desc_info.lock, "kconsole_desc");
	link_initialize(&desc_info.link);
	if (!cmd_register(&desc_info))
		panic("could not register command %s\n", desc_info.name);
	
	spinlock_initialize(&symaddr_info.lock, "kconsole_symaddr");
	link_initialize(&symaddr_info.link);
	if (!cmd_register(&symaddr_info))
		panic("could not register command %s\n", symaddr_info.name);

	spinlock_initialize(&call0_info.lock, "kconsole_call0");
	link_initialize(&call0_info.link);
	if (!cmd_register(&call0_info))
		panic("could not register command %s\n", call0_info.name);

	spinlock_initialize(&call1_info.lock, "kconsole_call1");
	link_initialize(&call1_info.link);
	if (!cmd_register(&call1_info))
		panic("could not register command %s\n", call1_info.name);


	spinlock_initialize(&call2_info.lock, "kconsole_call2");
	link_initialize(&call2_info.link);
	if (!cmd_register(&call2_info))
		panic("could not register command %s\n", call2_info.name);
	
	spinlock_initialize(&halt_info.lock, "kconsole_halt");
	link_initialize(&halt_info.link);
	if (!cmd_register(&halt_info))
		panic("could not register command %s\n", halt_info.name);
}


/** Register kconsole command.
 *
 * @param cmd Structure describing the command.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_register(cmd_info_t *cmd)
{
	ipl_t ipl;
	link_t *cur;
	
	spinlock_lock(&cmd_lock);
	
	/*
	 * Make sure the command is not already listed.
	 */
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);

		if (hlp == cmd) {
			/* The command is already there. */
			spinlock_unlock(&cmd_lock);
			return 0;
		}

		/* Avoid deadlock. */
		if (hlp < cmd) {
			spinlock_lock(&hlp->lock);
			spinlock_lock(&cmd->lock);
		} else {
			spinlock_lock(&cmd->lock);
			spinlock_lock(&hlp->lock);
		}
		
		if ((strncmp(hlp->name, cmd->name, strlen(cmd->name)) == 0)) {
			/* The command is already there. */
			spinlock_unlock(&hlp->lock);
			spinlock_unlock(&cmd->lock);
			spinlock_unlock(&cmd_lock);
			return 0;
		}
		
		spinlock_unlock(&hlp->lock);
		spinlock_unlock(&cmd->lock);
	}
	
	/*
	 * Now the command can be added.
	 */
	list_append(&cmd->link, &cmd_head);
	
	spinlock_unlock(&cmd_lock);
	return 1;
}

/** Kernel console managing thread.
 *
 * @param arg Not used.
 */
void kconsole(void *arg)
{
	char cmdline[MAX_CMDLINE+1];
	cmd_info_t *cmd_info;
	count_t len;

	if (!stdin) {
		printf("%s: no stdin\n", __FUNCTION__);
		return;
	}
	
	while (true) {
		printf("%s> ", __FUNCTION__);
		if (!(len = gets(stdin, cmdline, sizeof(cmdline))))
			continue;
		cmdline[len] = '\0';
		cmd_info = parse_cmdline(cmdline, len);
		if (!cmd_info)
			continue;
		(void) cmd_info->func(cmd_info->argv);
	}
}

static int parse_int_arg(char *text, size_t len, __native *result)
{
	char symname[MAX_SYMBOL_NAME];
	__address symaddr;
	bool isaddr = false;
	
	/* If we get a name, try to find it in symbol table */
	if (text[0] < '0' | text[0] > '9') {
		if (text[0] == '&') {
			isaddr = true;
			text++;len--;
		}
		strncpy(symname, text, min(len+1, MAX_SYMBOL_NAME));
		symaddr = get_symbol_addr(symname);
		if (!symaddr) {
			printf("Symbol %s not found.\n",symname);
			return -1;
		}
		if (symaddr == (__address) -1) {
			printf("Duplicate symbol %s.\n",symname);
			symtab_print_search(symname);
			return -1;
		}
		if (isaddr)
			*result = (__native)symaddr;
		else
			*result = *((__native *)symaddr);
	} else /* It's a number - convert it */
		*result = atoi(text);
	return 0;
}

/** Parse command line.
 *
 * @param cmdline Command line as read from input device.
 * @param len Command line length.
 *
 * @return Structure describing the command.
 */
cmd_info_t *parse_cmdline(char *cmdline, size_t len)
{
	index_t start = 0, end = 0;
	cmd_info_t *cmd = NULL;
	link_t *cur;
	ipl_t ipl;
	int i;
	
	if (!parse_argument(cmdline, len, &start, &end)) {
		/* Command line did not contain alphanumeric word. */
		return NULL;
	}

	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);
		
		if (strncmp(hlp->name, &cmdline[start], (end - start) + 1) == 0) {
			cmd = hlp;
			break;
		}
		
		spinlock_unlock(&hlp->lock);
	}
	
	spinlock_unlock(&cmd_lock);	
	
	if (!cmd) {
		/* Unknown command. */
		printf("Unknown command.\n");
		return NULL;
	}

	/* cmd == hlp is locked */
	
	/*
	 * The command line must be further analyzed and
	 * the parameters therefrom must be matched and
	 * converted to those specified in the cmd info
	 * structure.
	 */

	for (i = 0; i < cmd->argc; i++) {
		char *buf;
		start = end + 1;
		if (!parse_argument(cmdline, len, &start, &end)) {
			printf("Too few arguments.\n");
			spinlock_unlock(&cmd->lock);
			return NULL;
		}
		
		switch (cmd->argv[i].type) {
		case ARG_TYPE_STRING:
		    	buf = cmd->argv[i].buffer;
		    	strncpy(buf, (const char *) &cmdline[start], min((end - start) + 2, cmd->argv[i].len));
			buf[min((end - start) + 1, cmd->argv[i].len - 1)] = '\0';
			break;
		case ARG_TYPE_INT: 
			if (parse_int_arg(cmdline+start, end-start+1, 
					  &cmd->argv[i].intval))
				return NULL;
			break;
		case ARG_TYPE_VAR:
			if (start != end && cmdline[start] == '"' && cmdline[end] == '"') {
				buf = cmd->argv[i].buffer;
				strncpy(buf, (const char *) &cmdline[start+1], 
					min((end-start), cmd->argv[i].len));
				buf[min((end - start), cmd->argv[i].len - 1)] = '\0';
				cmd->argv[i].intval = (__native) buf;
				cmd->argv[i].vartype = ARG_TYPE_STRING;
			} else if (!parse_int_arg(cmdline+start, end-start+1, 
						 &cmd->argv[i].intval))
				cmd->argv[i].vartype = ARG_TYPE_INT;
			else {
				printf("Unrecognized variable argument.\n");
				return NULL;
			}
			break;
		case ARG_TYPE_INVALID:
		default:
			printf("invalid argument type\n");
			return NULL;
			break;
		}
	}
	
	start = end + 1;
	if (parse_argument(cmdline, len, &start, &end)) {
		printf("Too many arguments.\n");
		spinlock_unlock(&cmd->lock);
		return NULL;
	}
	
	spinlock_unlock(&cmd->lock);
	return cmd;
}

/** Parse argument.
 *
 * Find start and end positions of command line argument.
 *
 * @param cmdline Command line as read from the input device.
 * @param len Number of characters in cmdline.
 * @param start On entry, 'start' contains pointer to the index 
 *        of first unprocessed character of cmdline.
 *        On successful exit, it marks beginning of the next argument.
 * @param end Undefined on entry. On exit, 'end' points to the last character
 *        of the next argument.
 *
 * @return false on failure, true on success.
 */
bool parse_argument(char *cmdline, size_t len, index_t *start, index_t *end)
{
	int i;
	bool found_start = false;
	
	ASSERT(start != NULL);
	ASSERT(end != NULL);
	
	for (i = *start; i < len; i++) {
		if (!found_start) {
			if (is_white(cmdline[i]))
				(*start)++;
			else
				found_start = true;
		} else {
			if (is_white(cmdline[i]))
				break;
		}
	}
	*end = i - 1;

	return found_start;
}


/** List supported commands.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_help(cmd_arg_t *argv)
{
	link_t *cur;
	ipl_t ipl;

	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);
		
		printf("%s - %s\n", hlp->name, hlp->description);

		spinlock_unlock(&hlp->lock);
	}
	
	spinlock_unlock(&cmd_lock);	

	return 1;
}

/** Describe specified command.
 *
 * @param argv Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_desc(cmd_arg_t *argv)
{
	link_t *cur;
	ipl_t ipl;

	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);

		if (strncmp(hlp->name, (const char *) argv->buffer, strlen(hlp->name)) == 0) {
			printf("%s - %s\n", hlp->name, hlp->description);
			if (hlp->help)
				hlp->help();
			spinlock_unlock(&hlp->lock);
			break;
		}

		spinlock_unlock(&hlp->lock);
	}
	
	spinlock_unlock(&cmd_lock);	

	return 1;
}

/** Search symbol table */
int cmd_symaddr(cmd_arg_t *argv)
{
	__address symaddr;
	char *symbol;

	symtab_print_search(argv->buffer);
	
	return 1;
}

/** Call function with zero parameters */
int cmd_call0(cmd_arg_t *argv)
{
	__address symaddr;
	char *symbol;
	__native (*f)(void);

	symaddr = get_symbol_addr(argv->buffer);
	if (!symaddr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (symaddr == (__address) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		symbol = get_symtab_entry(symaddr);
		printf("Calling f(): 0x%p: %s\n", symaddr, symbol);
		f =  (__native (*)(void)) symaddr;
		printf("Result: 0x%X\n", f());
	}
	
	return 1;
}

/** Call function with one parameter */
int cmd_call1(cmd_arg_t *argv)
{
	__address symaddr;
	char *symbol;
	__native (*f)(__native);
	__native arg1 = argv[1].intval;

	symaddr = get_symbol_addr(argv->buffer);
	if (!symaddr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (symaddr == (__address) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		symbol = get_symtab_entry(symaddr);
		printf("Calling f(0x%x): 0x%p: %s\n", arg1, symaddr, symbol);
		f =  (__native (*)(__native)) symaddr;
		printf("Result: 0x%x\n", f(arg1));
	}
	
	return 1;
}

/** Call function with two parameters */
int cmd_call2(cmd_arg_t *argv)
{
	__address symaddr;
	char *symbol;
	__native (*f)(__native,__native);
	__native arg1 = argv[1].intval;
	__native arg2 = argv[2].intval;

	symaddr = get_symbol_addr(argv->buffer);
	if (!symaddr)
		printf("Symbol %s not found.\n", argv->buffer);
	else if (symaddr == (__address) -1) {
		symtab_print_search(argv->buffer);
		printf("Duplicate symbol, be more specific.\n");
	} else {
		symbol = get_symtab_entry(symaddr);
		printf("Calling f(0x%x,0x%x): 0x%p: %s\n", 
		       arg1, arg2, symaddr, symbol);
		f =  (__native (*)(__native,__native)) symaddr;
		printf("Result: 0x%x\n", f(arg1, arg2));
	}
	
	return 1;
}


/** Print detailed description of 'describe' command. */
void desc_help(void)
{
	printf("Syntax: describe command_name\n");
}

/** Halt the kernel.
 *
 * @param argv Argument vector (ignored).
 *
 * @return 0 on failure, 1 on success (never returns).
 */
int cmd_halt(cmd_arg_t *argv)
{
	halt();
	return 1;
}
