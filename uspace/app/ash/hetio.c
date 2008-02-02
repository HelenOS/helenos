/*
 * Termios command line History and Editting for NetBSD sh (ash)
 * Copyright (c) 1999
 *	Main code:	Adam Rogoyski <rogoyski@cs.utexas.edu> 
 *	Etc:		Dave Cinege <dcinege@psychosis.com>
 *
 * You may use this code as you wish, so long as the original author(s)
 * are attributed in any redistributions of the source code.
 * This code is 'as is' with no warranty.
 * This code may safely be consumed by a BSD or GPL license.
 *
 * v 0.5  19990328	Initial release 
 *
 * Future plans: Simple file and path name completion. (like BASH)
 *
 */

/*
Usage and Known bugs:
	Terminal key codes are not extensive, and more will probably
	need to be added. This version was created on Debian GNU/Linux 2.x.
	Delete, Backspace, Home, End, and the arrow keys were tested
	to work in an Xterm and console. Ctrl-A also works as Home.
	Ctrl-E also works as End. The binary size increase is <3K.
	
	Editting will not display correctly for lines greater then the 
	terminal width. (more then one line.) However, history will.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>

#include "input.h"
#include "output.h"

#ifdef HETIO

#include "hetio.h"

   
#define  MAX_HISTORY   15			/* Maximum length of the linked list for the command line history */

#define ESC	27
#define DEL	127

static struct history *his_front = NULL;	/* First element in command line list */
static struct history *his_end = NULL;		/* Last element in command line list */
static struct termios old_term, new_term;	/* Current termio and the previous termio before starting ash */

static int history_counter = 0;			/* Number of commands in history list */
static int reset_term = 0;			/* Set to true if the terminal needs to be reset upon exit */
//static int hetio_inter = 0;
int hetio_inter = 0;

struct history
{
   char *s;
   struct history *p;
   struct history *n;
};


void input_delete    (int);
void input_home      (int *);
void input_end       (int *, int);
void input_backspace (int *, int *);



void hetio_init(void)
{
	hetio_inter = 1;
}


void hetio_reset_term(void)
{
	if (reset_term)
		tcsetattr(1, TCSANOW, &old_term);
}


void setIO(struct termios *new, struct termios *old)	/* Set terminal IO to canonical mode, and save old term settings. */
{
	tcgetattr(0, old);
	memcpy(new, old, sizeof(*new));
	new->c_cc[VMIN] = 1;
	new->c_cc[VTIME] = 0;
	new->c_lflag &= ~ICANON; /* unbuffered input */
	new->c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, new);
}

void input_home(int *cursor)				/* Command line input routines */
{
 	while (*cursor > 0) {
		out1c('\b');
		--*cursor;
	}
	flushout(&output);
}


void input_delete(int cursor)
{
	int j = 0;

	memmove(parsenextc + cursor, parsenextc + cursor + 1,
		BUFSIZ - cursor - 1);
	for (j = cursor; j < (BUFSIZ - 1); j++) {
		if (!*(parsenextc + j))
			break;
		else
			out1c(*(parsenextc + j));
	}

	out1str(" \b");
	
	while (j-- > cursor)
		out1c('\b');
	flushout(&output);
}


void input_end(int *cursor, int len)
{
	while (*cursor < len) {
		out1str("\033[C");
		++*cursor;
	}
	flushout(&output);
}


void
input_backspace(int *cursor, int *len)
{
	int j = 0;

	if (*cursor > 0) {
		out1str("\b \b");
		--*cursor;
		memmove(parsenextc + *cursor, parsenextc + *cursor + 1, 
			BUFSIZ - *cursor + 1);
		
		for (j = *cursor; j < (BUFSIZ - 1); j++) {
			if (!*(parsenextc + j))
				break;
			else
				out1c(*(parsenextc + j));
		}
		
		out1str(" \b");
		
		while (j-- > *cursor)
			out1c('\b');
		
		--*len;
		flushout(&output);
	}
}

int hetio_read_input(int fd)
{
	int nr = 0;

	if (!hetio_inter) {		/* Are we an interactive shell? */
		return -255;		
	} else {
		int len = 0;
		int j = 0;
		int cursor = 0;
		int break_out = 0;
		int ret = 0;
		char c = 0;
		struct history *hp = his_end;

		if (!reset_term) {
			setIO(&new_term, &old_term);
			reset_term = 1;
		} else {
			tcsetattr(0, TCSANOW, &new_term);
		}
		
		memset(parsenextc, 0, BUFSIZ);
		
		while (1) {
			if ((ret = read(fd, &c, 1)) < 1)
				return ret;
			
			switch (c) {
   				case 1:		/* Control-A Beginning of line */
   					input_home(&cursor);
					break;
				case 5:		/* Control-E EOL */
					input_end(&cursor, len);
					break;
				case 4:		/* Control-D */
#ifndef CTRL_D_DELETE
					return 0;
#else
					if (cursor != len) {
						input_delete(cursor);
						len--;
					}
					break;
#endif
				case '\b':	/* Backspace */
				case DEL:
					input_backspace(&cursor, &len);
					break;
				case '\n':	/* Enter */
					*(parsenextc + len++ + 1) = c;
					out1c(c);
					flushout(&output);
					break_out = 1;
					break;
				case ESC:	/* escape sequence follows */
					if ((ret = read(fd, &c, 1)) < 1)
						return ret;
										
					if (c == '[' || c == 'O' ) {    /* 91 */
						if ((ret = read(fd, &c, 1)) < 1)
							return ret;
						
						switch (c) {
							case 'A':
								if (hp && hp->p) {		/* Up */
									hp = hp->p;
									goto hop;
								}
								break;
							case 'B':
								if (hp && hp->n && hp->n->s) {	/* Down */
									hp = hp->n;
									goto hop;
								}
								break;

hop:						/* hop */							
								len = strlen(parsenextc);

								for (; cursor > 0; cursor--)		/* return to begining of line */
									out1c('\b');

		   						for (j = 0; j < len; j++)		/* erase old command */
									out1c(' ');

								for (j = len; j > 0; j--)		/* return to begining of line */
									out1c('\b');

								strcpy (parsenextc, hp->s);		/* write new command */
								len = strlen (hp->s);
								out1str(parsenextc);
								flushout(&output);
								cursor = len;
								break;
							case 'C':		/* Right */
      								if (cursor < len) {
									out1str("\033[C");
									cursor++;
									flushout(&output);
						 		}
								break;
							case 'D':		/* Left */
								if (cursor > 0) {
									out1str("\033[D");
									cursor--;
									flushout(&output);
								}
								break;
							case '3':		/* Delete */
								if (cursor != len) {
									input_delete(cursor);
									len--;
								}
								break;								
							case 'H':		/* Home (xterm) */
							case '1':		/* Home (Ctrl-A) */
      								input_home(&cursor);
								break;
							case 'F':		/* End (xterm_ */
							case '4':		/* End (Ctrl-E) */
								input_end(&cursor, len);
								break;
						}
						if (c == '1' || c == '3' || c == '4')
							if ((ret = read(fd, &c, 1)) < 1)
								return ret;  /* read 126 (~) */
					}
			
					c = 0;
					break;
		
				default:				/* If it's regular input, do the normal thing */
	       
					if (!isprint(c))		/* Skip non-printable characters */
						break;
							       
	       				if (len >= (BUFSIZ - 2))	/* Need to leave space for enter */
		  				break;
	       		
					len++;
			
					if (cursor == (len - 1)) {	/* Append if at the end of the line */
						*(parsenextc + cursor) = c;
					} else {			/* Insert otherwise */
						memmove(parsenextc + cursor + 1, parsenextc + cursor,
							len - cursor - 1);
					
						*(parsenextc + cursor) = c;
			
						for (j = cursor; j < len; j++)
							out1c(*(parsenextc + j));
						for (; j > cursor; j--)
							out1str("\033[D");
					}
		
					cursor++;
					out1c(c);
					flushout(&output);
					break;
			}
			
			if (break_out)		/* Enter is the command terminator, no more input. */
				break;
		}
	
		nr = len + 1;
		tcsetattr(0, TCSANOW, &old_term);
		
		
		if (*(parsenextc)) {		/* Handle command history log */
			struct history *h = his_end;
  
			if (!h) {       /* No previous history */
				h = his_front = malloc(sizeof (struct history));
				h->n = malloc(sizeof (struct history));
				h->p = NULL;
				h->s = strdup(parsenextc);

				h->n->p = h;
				h->n->n = NULL;
				h->n->s = NULL;
				his_end = h->n;
				history_counter++;
			} else {	/* Add a new history command */
  
				h->n = malloc(sizeof (struct history)); 

				h->n->p = h;
				h->n->n = NULL;
				h->n->s = NULL;
				h->s = strdup(parsenextc);
				his_end = h->n;

				if (history_counter >= MAX_HISTORY) {	/* After max history, remove the last known command */
					struct history *p = his_front->n;
					
					p->p = NULL;
					free(his_front->s);
					free(his_front);
					his_front = p;
				} else {
					history_counter++;
				}
			}
		}
	} 

	return nr;
}
#endif
