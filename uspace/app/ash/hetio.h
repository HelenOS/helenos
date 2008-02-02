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

void hetio_init(void);
int hetio_read_input(int fd);
void hetio_reset_term(void);

extern int hetio_inter;
