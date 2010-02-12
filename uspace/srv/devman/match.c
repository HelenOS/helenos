/*
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup devman
 * @{
 */
 
 
#include <string.h>

#include "devman.h"


int get_match_score(driver_t *drv, node_t *dev)
{	
	link_t* drv_head = &drv->match_ids.ids;	
	link_t* dev_head = &dev->match_ids.ids;
	
	if (list_empty(drv_head) || list_empty(dev_head)) {
		return 0;
	}
	
	link_t* drv_link = drv->match_ids.ids.next;
	link_t* dev_link = dev->match_ids.ids.next;
	
	match_id_t *drv_id = list_get_instance(drv_link, match_id_t, link);
	match_id_t *dev_id = list_get_instance(dev_link, match_id_t, link);
	
	int score_next_drv = 0;
	int score_next_dev = 0;
	
	do {
		if (0 == str_cmp(drv_id->id, dev_id->id)) { 	// we found a match	
			// return the score of the match
			return drv_id->score * dev_id->score;
		}
		
		// compute the next score we get, if we advance in the driver's list of match ids 
		if (drv_head != drv_link->next) {
			score_next_drv = dev_id->score * list_get_instance(drv_link->next, match_id_t, link)->score;			
		} else {
			score_next_drv = 0;
		}
		
		// compute the next score we get, if we advance in the device's list of match ids 
		if (dev_head != dev_link->next) {
			score_next_dev = drv_id->score * list_get_instance(dev_link->next, match_id_t, link)->score;			
		} else {
			score_next_dev = 0;
		}
		
		// advance in one of the two lists, so we get the next highest score
		if (score_next_drv > score_next_dev) {
			drv_link = drv_link->next;
			drv_id = list_get_instance(drv_link, match_id_t, link);
		} else {
			dev_link = dev_link->next;
			dev_id = list_get_instance(dev_link, match_id_t, link);
		}
		
	} while (drv_head != drv_link->next && dev_head != dev_link->next);
	
	return 0;
}

void add_match_id(match_id_list_t *ids, match_id_t *id) 
{
	match_id_t *mid = NULL;
	link_t *link = ids->ids.next;	
	
	while (link != &ids->ids) {
		mid = list_get_instance(link, match_id_t,link);
		if (mid->score < id->score) {
			break;
		}	
		link = link->next;
	}
	
	list_insert_before(&id->link, link);	
}

void clean_match_ids(match_id_list_t *ids)
{
	link_t *link = NULL;
	match_id_t *id;
	
	while(!list_empty(&ids->ids)) {
		link = ids->ids.next;
		list_remove(link);		
		id = list_get_instance(link, match_id_t, link);
		delete_match_id(id);		
	}	
}

