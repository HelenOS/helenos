/*
 * Copyright (c) 2007 Jan Hudecek
 * Copyright (c) 2008 Martin Decky
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

/** @addtogroup genericproc
 * @{
 */
/** @file tasklet.c
 *  @brief Tasklet implementation
 */

#include <proc/tasklet.h>
#include <synch/spinlock.h>
#include <mm/slab.h>
#include <config.h>

/** Spinlock protecting list of tasklets */
SPINLOCK_INITIALIZE(tasklet_lock);

/** Array of tasklet lists for every CPU */
tasklet_descriptor_t **tasklet_list;

void tasklet_init(void)
{
	unsigned int i;
	
	tasklet_list = malloc(sizeof(tasklet_descriptor_t *) * config.cpu_count, 0);
	if (!tasklet_list)
		panic("Error initializing tasklets.");
	
	for (i = 0; i < config.cpu_count; i++)
		tasklet_list[i] = NULL;
	
	spinlock_initialize(&tasklet_lock, "tasklet_lock");
}


/** @}
 */
