/*
 * SPDX-FileCopyrightText: 2013 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

/**
 * @file
 * @brief
 */

#include <vfs/inbox.h>
#include <vfs/vfs.h>

#include <adt/list.h>
#include <str.h>
#include <stdlib.h>
#include <loader/pcb.h>
#include "../private/io.h"
#include <errno.h>

/*
 * This is an attempt at generalization of the "standard files" concept to
 * arbitrary names.  When loading a task, the parent can put arbitrary files to
 * an "inbox" through IPC calls, every file in an inbox has a name assigned
 * (e.g. "stdin", "stdout", "stderr", "data", "logfile", etc.). The client then
 * retrieves those files from inbox by name. "stdin", "stdout" and "stderr" are
 * handled automatically by libc to initialize standard <stdio.h> streams and
 * legacy file descriptors 0, 1, 2. Other names are subject to conventions and
 * application-specific rules.
 */

typedef struct {
	link_t link;
	char *name;
	int file;
} inbox_entry;

static LIST_INITIALIZE(inb_list);

/** Inserts a named file into the inbox.
 *
 * If a file with the same name is already present, it overwrites it and returns
 * the original value. Otherwise, it returns -1.  If the argument 'file' is -1,
 * nothing is inserted and the file with specified name is removed and returned
 * if present.
 *
 * @param name Name of the inbox entry.
 * @param file File to insert or -1.
 * @return Original value of the entry or -1 if not originally present.
 */
int inbox_set(const char *name, int file)
{
	inbox_entry *next = NULL;
	int result;

	list_foreach(inb_list, link, inbox_entry, e) {
		int cmp = str_cmp(e->name, name);
		switch (cmp) {
		case -1:
			continue;
		case 0:
			result = e->file;
			if (file == -1) {
				free(e->name);
				/* Safe because we exit immediately. */
				list_remove(&e->link);
			} else {
				e->file = file;
			}
			return result;
		case 1:
			next = e;
			goto out;
		}
	}

out:
	if (file == -1)
		return -1;

	inbox_entry *entry = calloc(sizeof(inbox_entry), 1);
	entry->name = str_dup(name);
	entry->file = file;

	if (next == NULL) {
		list_append((link_t *)entry, &inb_list);
	} else {
		list_insert_before((link_t *)entry, (link_t *)next);
	}
	return -1;
}

/** Retrieve a file with this name.
 *
 * @param name Name of the entry.
 * @return Requested file or -1 if not set.
 */
int inbox_get(const char *name)
{
	list_foreach(inb_list, link, inbox_entry, e) {
		int cmp = str_cmp(e->name, name);
		switch (cmp) {
		case 0:
			return e->file;
		case 1:
			return -1;
		}
	}

	return -1;
}

/** Writes names of entries that are currently set into an array provided by the
 * user.
 *
 * @param names Array in which names are stored.
 * @param capacity Capacity of the array.
 * @return Number of entries written. If capacity == 0, return the total number
 *         of entries.
 */
int inbox_list(const char **names, int capacity)
{
	if (capacity == 0)
		return list_count(&inb_list);

	int used = 0;

	list_foreach(inb_list, link, inbox_entry, e) {
		if (used == capacity)
			return used;
		names[used] = e->name;
		used++;
	}

	return used;
}

void __inbox_init(struct pcb_inbox_entry *entries, int count)
{
	for (int i = 0; i < count; i++) {
		int original = inbox_set(entries[i].name, entries[i].file);
		if (original >= 0)
			vfs_put(original);
	}
}

/**
 * @}
 */
