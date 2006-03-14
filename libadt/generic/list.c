/*
 * Copyright (C) 2004 Jakub Jermar
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

#include <list.h>


/** Check for membership
 *
 * Check whether link is contained in the list head.
 * The membership is defined as pointer equivalence.
 *
 * @param link Item to look for.
 * @param head List to look in.
 *
 * @return true if link is contained in head, false otherwise.
 *
 */
int list_member(const link_t *link, const link_t *head)
{
	int found = false;
	link_t *hlp = head->next;
	
	while (hlp != head) {
		if (hlp == link) {
			found = true;
			break;
		}
		hlp = hlp->next;
	}
	
	return found;
}


/** Concatenate two lists
 *
 * Concatenate lists head1 and head2, producing a single
 * list head1 containing items from both (in head1, head2
 * order) and empty list head2.
 *
 * @param head1 First list and concatenated output
 * @param head2 Second list and empty output.
 *
 */
void list_concat(link_t *head1, link_t *head2)
{
	if (list_empty(head2))
		return;

	head2->next->prev = head1->prev;
	head2->prev->next = head1;	
	head1->prev->next = head2->next;
	head1->prev = head2->prev;
	list_initialize(head2);
}
