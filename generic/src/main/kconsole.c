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

#include <main/kconsole.h>
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

#define MAX_CMDLINE	256

/** Simple kernel console.
 *
 * The console is realized by kernel thread kconsole.
 * It doesn't understand any commands on its own, but
 * makes it possible for other kernel subsystems to
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
static int cmd_help(cmd_arg_t *cmd);

static cmd_info_t help_info;

/** Initialize kconsole data structures. */
void kconsole_init(void)
{
	spinlock_initialize(&cmd_lock);
	list_initialize(&cmd_head);
	
	help_info.name = "help";
	help_info.description = "List supported commands.";
	help_info.func = cmd_help;
	help_info.argc = 0;
	help_info.argv = NULL;
	
	spinlock_initialize(&help_info.lock);
	link_initialize(&help_info.link);
	
	if (!cmd_register(&help_info))
		panic("could not register command\n");
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
		
		if ((strcmp(hlp->name, cmd->name, strlen(cmd->name)) == 0)) {
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
		len = gets(stdin, cmdline, sizeof(cmdline));
		cmdline[len] = '\0';
		cmd_info = parse_cmdline(cmdline, len);
		if (!cmd_info) {
			printf("?\n");
			continue;
		}
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
	bool found_start = false;
	cmd_info_t *cmd = NULL;
	link_t *cur;
	ipl_t ipl;
	int i;
	
	for (i = 0; i < len; i++) {
		if (!found_start) {
			if (is_white(cmdline[i]))
				start++;
			else
				found_start = true;
		} else {
			if (is_white(cmdline[i]))
				break;
		}
	}
	end = i - 1;
	
	if (!found_start) {
		/* Command line did not contain alphanumeric word. */
		return NULL;
	}

	ipl = interrupts_disable();
	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);
		
		if (strcmp(hlp->name, &cmdline[start], (end - start) + 1) == 0) {
			cmd = hlp;
			break;
		}
		
		spinlock_unlock(&hlp->lock);
	}
	
	spinlock_unlock(&cmd_lock);	
	
	if (!cmd) {
		/* Unknown command. */
		interrupts_restore(ipl);
		return NULL;
	}

	/* cmd == hlp is locked */
	
	/*
	 * TODO:
	 * The command line must be further analyzed and
	 * the parameters therefrom must be matched and
	 * converted to those specified in the cmd info
	 * structure.
	 */
	
	spinlock_unlock(&cmd->lock);
	interrupts_restore(ipl);
	return cmd;
}

/** List supported commands.
 *
 * @param cmd Argument vector.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_help(cmd_arg_t *cmd)
{
	link_t *cur;
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);
		
		printf("%s\t%s\n", hlp->name, hlp->description);

		spinlock_unlock(&hlp->lock);
	}
	
	spinlock_unlock(&cmd_lock);	
	interrupts_restore(ipl);

	return 1;
}
