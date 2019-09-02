/*
 * Copyright (c) 2015 Michal Koutny
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AS IS'' AND ANY EXPRESS OR
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

#ifndef SYSMAN_SYSMAN_H
#define SYSMAN_SYSMAN_H

#define INITRD_DEVICE       "bd/initrd"
#define INITRD_MOUNT_POINT  "/"
#define INITRD_CFG_PATH     "/cfg/sysman"

// TODO configurable
#define TARGET_INIT     "initrd.tgt"
#define TARGET_ROOTFS   "rootfs.tgt"
#define TARGET_DEFAULT  "default.tgt"
#define TARGET_SHUTDOWN "shutdown.tgt"

#define UNIT_MNT_INITRD "initrd.mnt"
#define UNIT_CFG_INITRD "init.cfg"

#include "job.h"
#include "unit.h"

typedef void (*event_handler_t)(void *);
typedef void (*callback_handler_t)(void *object, void *data);

extern void sysman_events_init(void);
extern int sysman_events_loop(void *);
extern errno_t sysman_run_job(unit_t *, unit_state_t, int, callback_handler_t, void *);

extern void sysman_raise_event(event_handler_t, void *);
extern void sysman_process_queue(void);
extern errno_t sysman_object_observer(void *, callback_handler_t, void *);
extern errno_t sysman_move_observers(void *, void *);
extern size_t sysman_observers_count(void *);

// TODO move particular events to separate file? (or move event impl there?)

extern void sysman_event_job_process(void *);
extern void sysman_event_job_finished(void *);
extern void sysman_event_unit_exposee_created(void *);
extern void sysman_event_unit_failed(void *);
extern void sysman_event_unit_state_changed(void *);

#endif
