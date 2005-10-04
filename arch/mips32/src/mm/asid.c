/*
 * Copyright (C) 2005 Martin Decky
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

#include <arch/mm/asid.h>
#include <synch/spinlock.h>
#include <arch.h>
#include <debug.h>

#define ASIDS	256

static spinlock_t asid_usage_lock;
static count_t asid_usage[ASIDS];	/**< Usage tracking array for ASIDs */

/** Get ASID
 *
 * Get the least used ASID.
 *
 * @return ASID
 */
asid_t asid_get(void)
{
	pri_t pri;
	int i, j;
	count_t min;
	
	min = (unsigned) -1;
	
	pri = cpu_priority_high();
	spinlock_lock(&asid_usage_lock);
	
	for (i=0, j = 0; (i<ASIDS); i++) {
		if (asid_usage[i] < min) {
			j = i;
			min = asid_usage[i];
			if (!min)
				break;
		}
	}

	asid_usage[i]++;

	spinlock_unlock(&asid_usage_lock);
	cpu_priority_restore(pri);

	return i;
}

/** Release ASID
 *
 * Release ASID by decrementing its usage count.
 *
 * @param asid ASID.
 */
void asid_put(asid_t asid)
{
	pri_t pri;

	pri = cpu_priority_high();
	spinlock_lock(&asid_usage_lock);

	ASSERT(asid_usage[asid] > 0);
	asid_usage[asid]--;

	spinlock_unlock(&asid_usage_lock);
	cpu_priority_restore(pri);
}
