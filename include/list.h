/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __LIST_H__
#define __LIST_H__

#include <arch/types.h>
#include <typedefs.h>

struct link {
	link_t *prev;
	link_t *next;
};


#define link_initialize(link) { \
	(link)->prev = NULL; \
	(link)->next = NULL; \
}

#define list_initialize(head) { \
	(head)->prev = (head); \
	(head)->next = (head); \
}

#define list_prepend(link, head) { \
	(link)->next = (head)->next; \
	(link)->prev = (head); \
	(head)->next->prev = (link); \
	(head)->next = (link); \
}

#define list_append(link, head) { \
	(link)->prev = (head)->prev; \
	(link)->next = (head); \
	(head)->prev->next = (link); \
	(head)->prev = (link); \
}

#define list_remove(link) { \
	(link)->next->prev = (link)->prev; \
	(link)->prev->next = (link)->next; \
	link_initialize(link); \
}

#define list_empty(head) (((head)->next == (head))?1:0)

#define list_get_instance(link,type,member) (type *)(((__u8*)(link))-((__u8*)&(((type *)NULL)->member)))

extern int list_member(link_t *link, link_t *head);
extern void list_concat(link_t *head1, link_t *head2);

#endif
