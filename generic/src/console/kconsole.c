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
#include <console/cmd.h>
#include <print.h>
#include <panic.h>
#include <typedefs.h>
#include <arch/types.h>
#include <adt/list.h>
#include <arch.h>
#include <macros.h>
#include <debug.h>
#include <func.h>
#include <symtab.h>
#include <macros.h>

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
 
SPINLOCK_INITIALIZE(cmd_lock);	/**< Lock protecting command list. */
LIST_INITIALIZE(cmd_head);	/**< Command list. */

static cmd_info_t *parse_cmdline(char *cmdline, size_t len);
static bool parse_argument(char *cmdline, size_t len, index_t *start, index_t *end);
static char history[KCONSOLE_HISTORY][MAX_CMDLINE] = {};

/** Initialize kconsole data structures. */
void kconsole_init(void)
{
	int i;

	cmd_init();
	for (i=0; i<KCONSOLE_HISTORY; i++)
		history[i][0] = '\0';
}


/** Register kconsole command.
 *
 * @param cmd Structure describing the command.
 *
 * @return 0 on failure, 1 on success.
 */
int cmd_register(cmd_info_t *cmd)
{
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
		if ((strncmp(hlp->name, 
			     cmd->name, max(strlen(cmd->name),
					    strlen(hlp->name))) == 0)) {
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

/** Print count times a character */
static void rdln_print_c(char ch, int count)
{
	int i;
	for (i=0;i<count;i++)
		putchar(ch);
}

/** Insert character to string */
static void insert_char(char *str, char ch, int pos)
{
	int i;
	
	for (i=strlen(str);i > pos; i--)
		str[i] = str[i-1];
	str[pos] = ch;
}

/** Try to find a command beginning with prefix */
static const char * cmdtab_search_one(const char *name,link_t **startpos)
{
	int namelen = strlen(name);
	const char *curname;

	spinlock_lock(&cmd_lock);

	if (!*startpos)
		*startpos = cmd_head.next;

	for (;*startpos != &cmd_head;*startpos = (*startpos)->next) {
		cmd_info_t *hlp;
		hlp = list_get_instance(*startpos, cmd_info_t, link);

		curname = hlp->name;
		if (strlen(curname) < namelen)
			continue;
		if (strncmp(curname, name, namelen) == 0) {
			spinlock_unlock(&cmd_lock);	
			return curname+namelen;
		}
	}
	spinlock_unlock(&cmd_lock);	
	return NULL;
}


/** Command completion of the commands 
 *
 * @param name - string to match, changed to hint on exit
 * @return number of found matches
 */
static int cmdtab_compl(char *name)
{
	char output[MAX_SYMBOL_NAME+1];
	link_t *startpos = NULL;
	const char *foundtxt;
	int found = 0;
	int i;

	output[0] = '\0';
	while ((foundtxt = cmdtab_search_one(name, &startpos))) {
		startpos = startpos->next;
		if (!found)
			strncpy(output, foundtxt, strlen(foundtxt)+1);
		else {
			for (i=0; output[i] && foundtxt[i] && output[i]==foundtxt[i]; i++)
				;
			output[i] = '\0';
		}
		found++;
	}
	if (!found)
		return 0;

	if (found > 1 && !strlen(output)) {
		printf("\n");
		startpos = NULL;
		while ((foundtxt = cmdtab_search_one(name, &startpos))) {
			cmd_info_t *hlp;
			hlp = list_get_instance(startpos, cmd_info_t, link);
			printf("%s - %s\n", hlp->name, hlp->description);
			startpos = startpos->next;
		}
	}
	strncpy(name, output, MAX_SYMBOL_NAME);
	return found;
	
}

static char * clever_readline(const char *prompt, chardev_t *input)
{
	static int histposition = 0;

	char tmp[MAX_CMDLINE+1];
	int curlen = 0, position = 0;
	char *current = history[histposition];
	int i;
	char mod; /* Command Modifier */
	char c;

	printf("%s> ", prompt);
	while (1) {
		c = _getc(input);
		if (c == '\n') {
			putchar(c);
			break;
		} if (c == '\b') { /* Backspace */
			if (position == 0)
				continue;
			for (i=position; i<curlen;i++)
				current[i-1] = current[i];
			curlen--;
			position--;
			putchar('\b');
			for (i=position;i<curlen;i++)
				putchar(current[i]);
			putchar(' ');
			rdln_print_c('\b',curlen-position+1);
			continue;
		}
		if (c == '\t') { /* Tabulator */
			int found;

			/* Move to the end of the word */
			for (;position<curlen && current[position]!=' ';position++)
				putchar(current[position]);
			/* Copy to tmp last word */
			for (i=position-1;i >= 0 && current[i]!=' ' ;i--)
				;
			/* If word begins with * or &, skip it */
			if (tmp[0] == '*' || tmp[0] == '&')
				for (i=1;tmp[i];i++)
					tmp[i-1] = tmp[i];
			i++; /* I is at the start of the word */
			strncpy(tmp, current+i, position-i+1);

			if (i==0) { /* Command completion */
				found = cmdtab_compl(tmp);
			} else { /* Symtab completion */
				found = symtab_compl(tmp);
			}

			if (found == 0) 
				continue;
			for (i=0;tmp[i] && curlen < MAX_CMDLINE;i++,curlen++)
				insert_char(current, tmp[i], i+position);

			if (strlen(tmp) || found==1) { /* If we have a hint */
				for (i=position;i<curlen;i++) 
					putchar(current[i]);
				position += strlen(tmp);
				/* Add space to end */
				if (found == 1 && position == curlen && \
				    curlen < MAX_CMDLINE) {
					current[position] = ' ';
					curlen++;
					position++;
					putchar(' ');
				}
			} else { /* No hint, table was printed */
				printf("%s> ", prompt);
				for (i=0; i<curlen;i++)
					putchar(current[i]);
				position += strlen(tmp);
			}
			rdln_print_c('\b', curlen-position);
			continue;
		}
		if (c == 0x1b) { /* Special command */
			mod = _getc(input);
			c = _getc(input);

			if (mod != 0x5b && mod != 0x4f)
				continue;

			if (c == 0x33 && _getc(input) == 0x7e) {
				/* Delete */
				if (position == curlen)
					continue;
				for (i=position+1; i<curlen;i++) {
					putchar(current[i]);
					current[i-1] = current[i];
				}
				putchar(' ');
				rdln_print_c('\b',curlen-position);
				curlen--;
			}
			else if (c == 0x48) { /* Home */
				rdln_print_c('\b',position);
				position = 0;
			} 
			else if (c == 0x46) {  /* End */
				for (i=position;i<curlen;i++)
					putchar(current[i]);
				position = curlen;
			}
			else if (c == 0x44) { /* Left */
				if (position > 0) {
					putchar('\b');
					position--;
				}
				continue;
			}
			else if (c == 0x43) { /* Right */
				if (position < curlen) {
					putchar(current[position]);
					position++;
				}
				continue;
			}
			else if (c == 0x41 || c == 0x42) { 
                                /* Up,down */
				rdln_print_c('\b',position);
				rdln_print_c(' ',curlen);
				rdln_print_c('\b',curlen);
				if (c == 0x41) /* Up */
					histposition--;
				else
					histposition++;
				if (histposition < 0)
					histposition = KCONSOLE_HISTORY -1 ;
				else
					histposition =  histposition % KCONSOLE_HISTORY;
				current = history[histposition];
				printf("%s", current);
				curlen = strlen(current);
				position = curlen;
				continue;
			}
			continue;
		}
		if (curlen >= MAX_CMDLINE)
			continue;

		insert_char(current, c, position);

		curlen++;
		for (i=position;i<curlen;i++)
			putchar(current[i]);
		position++;
		rdln_print_c('\b',curlen-position);
	} 
	if (curlen) {
		histposition++;
		histposition = histposition % KCONSOLE_HISTORY;
	}
	current[curlen] = '\0';
	return current;
}

/** Kernel console managing thread.
 *
 * @param arg Not used.
 */
void kconsole(void *prompt)
{
	cmd_info_t *cmd_info;
	count_t len;
	char *cmdline;

	if (!stdin) {
		printf("%s: no stdin\n", __FUNCTION__);
		return;
	}
	
	while (true) {
		cmdline = clever_readline(prompt, stdin);
		len = strlen(cmdline);
		if (!len)
			continue;
		cmd_info = parse_cmdline(cmdline, len);
		if (!cmd_info)
			continue;
		if (strncmp(cmd_info->name,"exit", \
			    min(strlen(cmd_info->name),5)) == 0)
			break;
		(void) cmd_info->func(cmd_info->argv);
	}
}

static int parse_int_arg(char *text, size_t len, __native *result)
{
	char symname[MAX_SYMBOL_NAME];
	__address symaddr;
	bool isaddr = false;
	bool isptr = false;
	
	/* If we get a name, try to find it in symbol table */
	if (text[0] == '&') {
		isaddr = true;
		text++;len--;
	} else if (text[0] == '*') {
		isptr = true;
		text++;len--;
	}
	if (text[0] < '0' || text[0] > '9') {
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
	} else /* It's a number - convert it */
		*result = atoi(text);

	if (isaddr)
		*result = (__native)symaddr;
	else if (isptr)
		*result = **((__native **)symaddr);
	else
		*result = *((__native *)symaddr);

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
	int i;
	int error = 0;
	
	if (!parse_argument(cmdline, len, &start, &end)) {
		/* Command line did not contain alphanumeric word. */
		return NULL;
	}

	spinlock_lock(&cmd_lock);
	
	for (cur = cmd_head.next; cur != &cmd_head; cur = cur->next) {
		cmd_info_t *hlp;
		
		hlp = list_get_instance(cur, cmd_info_t, link);
		spinlock_lock(&hlp->lock);
		
		if (strncmp(hlp->name, &cmdline[start], max(strlen(hlp->name),
							    end-start+1)) == 0) {
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
		
		error = 0;
		switch (cmd->argv[i].type) {
		case ARG_TYPE_STRING:
		    	buf = cmd->argv[i].buffer;
		    	strncpy(buf, (const char *) &cmdline[start], min((end - start) + 2, cmd->argv[i].len));
			buf[min((end - start) + 1, cmd->argv[i].len - 1)] = '\0';
			break;
		case ARG_TYPE_INT: 
			if (parse_int_arg(cmdline+start, end-start+1, 
					  &cmd->argv[i].intval))
				error = 1;
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
				error = 1;
			}
			break;
		case ARG_TYPE_INVALID:
		default:
			printf("invalid argument type\n");
			error = 1;
			break;
		}
	}
	
	if (error) {
		spinlock_unlock(&cmd->lock);
		return NULL;
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
