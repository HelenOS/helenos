/*
 * Copyright (c) 2013 Martin Sucha
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

/** @addtogroup genericlog
 * @{
 */
/** @file
 */

#include <sysinfo/sysinfo.h>
#include <synch/spinlock.h>
#include <typedefs.h>
#include <ddi/irq.h>
#include <ddi/ddi.h>
#include <ipc/event.h>
#include <ipc/irq.h>
#include <arch.h>
#include <panic.h>
#include <putchar.h>
#include <atomic.h>
#include <syscall/copy.h>
#include <errno.h>
#include <str.h>
#include <print.h>
#include <printf/printf_core.h>
#include <stdarg.h>
#include <log.h>
#include <console/console.h>
#include <abi/log.h>
#include <mm/slab.h>

#define LOG_PAGES    8
#define LOG_LENGTH   (LOG_PAGES * PAGE_SIZE)
#define LOG_ENTRY_HEADER_LENGTH (sizeof(size_t) + sizeof(uint32_t))

/** Cyclic buffer holding the data for kernel log */
uint8_t log_buffer[LOG_LENGTH] __attribute__((aligned(PAGE_SIZE)));

/** Kernel log initialized */
static atomic_t log_inited = {false};

/** Position in the cyclic buffer where the first log entry starts */
size_t log_start = 0;

/** Sum of length of all log entries currently stored in the cyclic buffer */
size_t log_used = 0;

/** Log spinlock */
SPINLOCK_STATIC_INITIALIZE_NAME(log_lock, "log_lock");

/** Overall count of logged messages, which may overflow as needed */
static uint32_t log_counter = 0;

/** Starting position of the entry currently being written to the log */
static size_t log_current_start = 0;

/** Length (including header) of the entry currently being written to the log */
static size_t log_current_len = 0;

/** Start of the next entry to be handed to uspace starting from log_start */
static size_t next_for_uspace = 0;

static void log_update(void *);

/** Initialize kernel logging facility
 *
 */
void log_init(void)
{	
	event_set_unmask_callback(EVENT_KLOG, log_update);
	atomic_set(&log_inited, true);
}

static size_t log_copy_from(uint8_t *data, size_t pos, size_t len) {
	for (size_t i = 0; i < len; i++, pos = (pos + 1) % LOG_LENGTH) {
		data[i] = log_buffer[pos];
	}
	return pos;
}

static size_t log_copy_to(const uint8_t *data, size_t pos, size_t len) {
	for (size_t i = 0; i < len; i++, pos = (pos + 1) % LOG_LENGTH) {
		log_buffer[pos] = data[i];
	}
	return pos;
}

/** Append data to the currently open log entry.
 * 
 * This function requires that the log_lock is acquired by the caller.
 */
static void log_append(const uint8_t *data, size_t len)
{
	/* Cap the length so that the entry entirely fits into the buffer */
	if (len > LOG_LENGTH - log_current_len) {
		len = LOG_LENGTH - log_current_len;
	}
	
	if (len == 0)
		return;
	
	size_t log_free = LOG_LENGTH - log_used - log_current_len;
	
	/* Discard older entries to make space, if necessary */
	while (len > log_free) {
		size_t entry_len;
		log_copy_from((uint8_t *) &entry_len, log_start, sizeof(size_t));
		log_start = (log_start + entry_len) % LOG_LENGTH;
		log_used -= entry_len;
		log_free += entry_len;
		next_for_uspace -= entry_len;
	}
	
	size_t pos = (log_current_start + log_current_len) % LOG_LENGTH;
	log_copy_to(data, pos, len);
	log_current_len += len;
}

/** Begin writing an entry to the log.
 * 
 * This acquires the log and output buffer locks, so only calls to log_* functions should
 * be used until calling log_end.
 */
void log_begin(log_facility_t fac, log_level_t level)
{
	spinlock_lock(&log_lock);
	spinlock_lock(&kio_lock);
	
	log_current_start = (log_start + log_used) % LOG_LENGTH;
	log_current_len = 0;
	
	/* Write header of the log entry, the length will be written in log_end() */
	log_append((uint8_t *) &log_current_len, sizeof(size_t));
	log_append((uint8_t *) &log_counter, sizeof(uint32_t));
	uint32_t fac32 = fac;
	uint32_t lvl32 = level;
	log_append((uint8_t *) &fac32, sizeof(uint32_t));
	log_append((uint8_t *) &lvl32, sizeof(uint32_t));
	
	log_counter++;
}

/** Finish writing an entry to the log.
 * 
 * This releases the log and output buffer locks.
 */
void log_end(void) {
	/* Set the length in the header to correct value */
	log_copy_to((uint8_t *) &log_current_len, log_current_start, sizeof(size_t));
	log_used += log_current_len;
	
	kio_push_char('\n');
	spinlock_unlock(&kio_lock);
	spinlock_unlock(&log_lock);
	
	/* This has to be called after we released the locks above */
	kio_flush();
	kio_update(NULL);
	log_update(NULL);
}

static void log_update(void *event)
{
	if (!atomic_get(&log_inited))
		return;
	
	spinlock_lock(&log_lock);
	if (next_for_uspace < log_used)
		event_notify_0(EVENT_KLOG, true);
	spinlock_unlock(&log_lock);
}

static int log_printf_str_write(const char *str, size_t size, void *data)
{
	size_t offset = 0;
	size_t chars = 0;
	
	while (offset < size) {
		kio_push_char(str_decode(str, &offset, size));
		chars++;
	}
	
	log_append((const uint8_t *)str, size);
	
	return chars;
}

static int log_printf_wstr_write(const wchar_t *wstr, size_t size, void *data)
{
	char buffer[16];
	size_t offset = 0;
	size_t chars = 0;
	
	for (offset = 0; offset < size; offset += sizeof(wchar_t), chars++) {
		kio_push_char(wstr[chars]);
		
		size_t buffer_offset = 0;
		errno_t rc = chr_encode(wstr[chars], buffer, &buffer_offset, 16);
		if (rc != EOK) {
			return EOF;
		}
		
		log_append((const uint8_t *)buffer, buffer_offset);
	}
	
	return chars;
}

/** Append a message to the currently being written entry.
 * 
 * Requires that an entry has been started using log_begin()
 */
int log_vprintf(const char *fmt, va_list args)
{
	int ret;
	
	printf_spec_t ps = {
		log_printf_str_write,
		log_printf_wstr_write,
		NULL
	};
	
	
	ret = printf_core(fmt, &ps, args);
	
	return ret;
}

/** Append a message to the currently being written entry.
 * 
 * Requires that an entry has been started using log_begin()
 */
int log_printf(const char *fmt, ...)
{
	int ret;
	va_list args;
	
	va_start(args, fmt);
	ret = log_vprintf(fmt, args);
	va_end(args);
	
	return ret;
}

/** Log a message to the kernel log.
 * 
 * This atomically appends a log entry.
 * The resulting message should not contain a trailing newline, as the log
 * entries are explicitly delimited when stored in the log.
 */
int log(log_facility_t fac, log_level_t level, const char *fmt, ...)
{
	int ret;
	va_list args;
	
	log_begin(fac, level);
	
	va_start(args, fmt);
	ret = log_vprintf(fmt, args);
	va_end(args);
	
	log_end();
	
	return ret;
}

/** Control of the log from uspace
 *
 */
sys_errno_t sys_klog(sysarg_t operation, void *buf, size_t size,
    sysarg_t level, size_t *uspace_nread)
{
	char *data;
	errno_t rc;
	
	if (size > PAGE_SIZE)
		return (sys_errno_t) ELIMIT;
	
	switch (operation) {
		case KLOG_WRITE:
			data = (char *) malloc(size + 1, 0);
			if (!data)
				return (sys_errno_t) ENOMEM;
			
			rc = copy_from_uspace(data, buf, size);
			if (rc) {
				free(data);
				return (sys_errno_t) rc;
			}
			data[size] = 0;
			
			if (level >= LVL_LIMIT)
				level = LVL_NOTE;
			
			log(LF_USPACE, level, "%s", data);
			
			free(data);
			return EOK;
		case KLOG_READ:
			data = (char *) malloc(size, 0);
			if (!data)
				return (sys_errno_t) ENOMEM;
			
			size_t entry_len = 0;
			size_t copied = 0;
			
			rc = EOK;
	
			spinlock_lock(&log_lock);
			
			while (next_for_uspace < log_used) {
				size_t pos = (log_start + next_for_uspace) % LOG_LENGTH;
				log_copy_from((uint8_t *) &entry_len, pos, sizeof(size_t));
				
				if (entry_len > PAGE_SIZE) {
					/* 
					 * Since we limit data transfer
					 * to uspace to a maximum of PAGE_SIZE
					 * bytes, skip any entries larger
					 * than this limit to prevent
					 * userspace being stuck trying to
					 * read them.
					 */
					next_for_uspace += entry_len;
					continue;
				}
				
				if (size < copied + entry_len) {
					if (copied == 0)
						rc = EOVERFLOW;
					break;
				}
				
				log_copy_from((uint8_t *) (data + copied), pos, entry_len);
				copied += entry_len;
				next_for_uspace += entry_len;
			}
			
			spinlock_unlock(&log_lock);
			
			if (rc != EOK) {
				free(data);
				return (sys_errno_t) rc;
			}
			
			rc = copy_to_uspace(buf, data, size);
			
			free(data);
			
			if (rc != EOK)
				return (sys_errno_t) rc;
			
			return copy_to_uspace(uspace_nread, &copied, sizeof(copied));
			return EOK;
		default:
			return (sys_errno_t) ENOTSUP;
	}
}


/** @}
 */
