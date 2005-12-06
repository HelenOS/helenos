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
static cmd_info_t help_info;

/** Data and methods for 'description' command. */
static int cmd_desc(cmd_arg_t *argv);
static void desc_help(void);
static cmd_info_t desc_info;
static char desc_buf[MAX_CMDLINE+1];
static cmd_arg_t desc_argv = {
	.type = ARG_TYPE_STRING,
	.buffer = desc_buf,
	.len = sizeof(desc_buf)
};

/** Data and methods for 'halt' command. */
static int cmd_halt(cmd_arg_t *argv);
static cmd_info_t halt_info;

/** Initialize kconsole data structures. */
void kconsole_init(void)
{
	spinlock_initialize(&cmd_lock);
	list_initialize(&cmd_head);
	
	help_info.name = "help";
	help_info.description = "List supported commands.";
	help_info.func = cmd_help;
	help_info.help = NULL;
	help_info.argc = 0;
	help_info.argv = NULL;

	spinlock_initialize(&help_info.lock);
	link_initialize(&help_info.link);

	if (!cmd_register(&help_info))
		panic("could not register command %s\n", help_info.name);


	desc_info.name = "describe";
	desc_info.description = "Describe specified command.";
	desc_info.help = desc_help;
	desc_info.func = cmd_desc;
	desc_info.argc = 1;
	desc_info.argv = &desc_argv;
	
	spinlock_initialize(&desc_info.lock);
	link_initialize(&desc_info.link);
	
	if (!cmd_register(&desc_info))
		panic("could not register command %s\n", desc_info.name);
	
	
	halt_info.name = "halt";
	halt_info.description = "Halt the kernel.";
	halt_info.func = cmd_halt;
	halt_info.help = NULL;
	halt_info.argc = 0;
	halt_info.argv = NULL;

	spinlock_initialize(&halt_info.lock);
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
	
	ipl = interrupts_disable();
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
			interrupts_restore(ipl);
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
			interrupts_restore(ipl);
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
	interrupts_restore(ipl);
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

	ipl = interrupts_disable();
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
		interrupts_restore(ipl);
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
			interrupts_restore(ipl);
			return NULL;
		}
		
		switch (cmd->argv[i].type) {
		    case ARG_TYPE_STRING:
		    	buf = cmd->argv[i].buffer;
		    	strncpy(buf, (const char *) &cmdline[start], min((end - start) + 1, cmd->argv[i].len - 1));
			buf[min((end - start) + 1, cmd->argv[i].len - 1)] = '\0';
			break;
		    case ARG_TYPE_INT:
		    case ARG_TYPE_INVALID:
		    default:
			panic("invalid argument type\n");
			break;
		}
	}
	
	start = end + 1;
	if (parse_argument(cmdline, len, &start, &end)) {
		printf("Too many arguments.\n");
		spinlock_unlock(&cmd->lock);
		interrupts_restore(ipl);
		return NULL;
	}
	
	spinlock_unlock(&cmd->lock);
	interrupts_restore(ipl);
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

	ipl = interrupts_disable();
	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);
		
		printf("%s - %s\n", hlp->name, hlp->description);

		spinlock_unlock(&hlp->lock);
	}
	
	spinlock_unlock(&cmd_lock);	
	interrupts_restore(ipl);

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

	ipl = interrupts_disable();
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
	interrupts_restore(ipl);

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
