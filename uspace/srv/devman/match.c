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

#include <str.h>

#include "devman.h"

/** Compute compound score of driver and device.
 *
 * @param driver Match id of the driver.
 * @param device Match id of the device.
 * @return Compound score.
 * @retval 0 No match at all.
 */
static int compute_match_score(match_id_t *driver, match_id_t *device)
{
	if (str_cmp(driver->id, device->id) == 0) {
		/*
		 * The strings match, return the product of their scores.
		 */
		return driver->score * device->score;
	} else {
		/*
		 * Different strings, return zero.
		 */
		return 0;
	}
}

int get_match_score(driver_t *drv, dev_node_t *dev)
{
	link_t *drv_head = &drv->match_ids.ids.head;
	link_t *dev_head = &dev->pfun->match_ids.ids.head;
	
	if (list_empty(&drv->match_ids.ids) ||
	    list_empty(&dev->pfun->match_ids.ids)) {
		return 0;
	}
	
	/*
	 * Go through all pairs, return the highest score obtained.
	 */
	int highest_score = 0;
	
	link_t *drv_link = drv->match_ids.ids.head.next;
	while (drv_link != drv_head) {
		link_t *dev_link = dev_head->next;
		while (dev_link != dev_head) {
			match_id_t *drv_id = list_get_instance(drv_link, match_id_t, link);
			match_id_t *dev_id = list_get_instance(dev_link, match_id_t, link);
			
			int score = compute_match_score(drv_id, dev_id);
			if (score > highest_score) {
				highest_score = score;
			}

			dev_link = dev_link->next;
		}
		
		drv_link = drv_link->next;
	}
	
	return highest_score;
}

/** @}
 */
