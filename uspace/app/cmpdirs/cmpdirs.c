/*
 * Copyright (c) 2026 Vít Skalický
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

/** @addtogroup cmpdirs
 * @brief Recursively compare two directories for equality. Checks: directory and file tree, names, metadata and file content
 * @{
 */
/**
 * @file
 */

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <macros.h>
#include <vfs/vfs.h>
#include <mem.h>
#include <str.h>
#include <str_error.h>
#include <io/log.h>

#define NAME  "cmpdirs"
/** When reading the content of a file for comparison, how large should one chunk be. */
#define CHUNK_SIZE 0x4000
/** File names longer than this will be truncated. */
#define NAME_BUFFER_SIZE 256

static void print_usage(void);

/** Returns true if all directories and files in the filesystem tree of handle1 also exist in the filesystem tree of
 * handle2 and in reverse and all their contents and metadata is equal.
 * hande1 and handle2 are handles to a directory (or file). */
static errno_t trees_equal(int handle1, int handle2, bool* equal_out);

int main(int argc, char *argv[])
{
	int optres, errflg = 0;

	/* Parse command-line options */
	while ((optres = getopt(argc, argv, "h")) != -1) {
		switch (optres) {
		case 'h':
			print_usage();
			return 0;

		case '?':
			fprintf(stderr, "Unrecognized option: -%c\n", optopt);
			errflg++;
			break;

		default:
			fprintf(stderr,
			    "Unknown error while parsing command line options");
			errflg++;
			break;
		}
	}

	// advance over already parsed options
	argc -= optind;
	argv = argv + optind;

	if (argc != 2) {
		fprintf(stderr, "Wrong number of arguments. Two arguments are required.\n");
		errflg++;
	}

	if (errflg) {
		print_usage();
		return 1;
	}

	errno_t rc = EOK;

	// initialize logging
	rc = log_init(NAME);
	if (rc != EOK) goto close_end;

	// lookup both directories
	int handle1;
	int handle2;
	rc = vfs_lookup(argv[0], WALK_DIRECTORY | WALK_REGULAR | WALK_MOUNT_POINT, &handle1);
	if (rc != EOK) goto close_end;
	rc = vfs_lookup(argv[1], WALK_DIRECTORY | WALK_REGULAR | WALK_MOUNT_POINT, &handle2);
	if (rc != EOK) goto close1;

	bool equal;
	rc = trees_equal(handle1, handle2, &equal);
	if (rc != EOK) goto close2;
close2:
	vfs_put(handle2);
close1:
	vfs_put(handle1);
close_end:

	if (rc != EOK) {
		fprintf(stderr, "%d\n",rc);
		fprintf(stderr, "An error occurred: %s: %s\n", str_error_name(rc), str_error(rc));
		return 2;
	}

	return equal ? 0 : 1;
}

struct entry {
	int handle1;
	int handle2;
};

/** Growable stack. */
struct stack {
	/** Allocated memory */
	struct entry *buffer;
	/** How much is allocated */
	size_t capacity;
	/** How much is used */
	size_t size;
};

size_t STACK_INIT_CAPACITY = 256;

/** Create a new growable stack. Don't forget to free it in the end using stack_free */
static errno_t stack_new(struct stack* s) {
	s->size = 0;
	s->capacity = STACK_INIT_CAPACITY;
	s->buffer = malloc(sizeof(struct entry) * s->capacity);
	if (s->buffer == NULL) {
		return ENOMEM;
	}
	return EOK;
}

/** Destroy a growable stack and free its memory */
static void stack_free(struct stack* s) {
	if (s->buffer != NULL) {
		free(s->buffer);
	}
	s->buffer = NULL;
	s->size = 0;
	s->capacity = 0;
}

/** Append an entry to the stack. Allocates larger memory if needed. */
static errno_t stack_append(struct stack *s, struct entry e) {
	if (s->capacity == 0 || s->buffer == NULL) {
		// stack has already been freed before
		return ENOENT;
	}
	if (s->size == s->capacity) {
		// grow the stack
		// allocate a new buffer twice the capacity of the old one and switch them. Also free the old one
		struct entry *newbuffer = malloc(sizeof(struct entry) * s->capacity * 2);
		if (newbuffer == NULL) {
			return ENOMEM;
		}
		memcpy(newbuffer, s->buffer, sizeof(struct entry) * s->capacity);
		free(s->buffer);
		s->buffer = newbuffer;
		s->capacity *= 2;
	}
	assert(s->size < s->capacity);
	s->buffer[s->size] = e;
	s->size++;
	return EOK;
}

/** Returns the last entry in the stack and removes it from it. Does not free any memory */
static errno_t stack_pop(struct stack *s, struct entry *entry_out) {
	if (s->size == 0) {
		return EEMPTY;
	}
	s->size--;
	(*entry_out) = s->buffer[s->size];
	return EOK;
}

/** Writes the name of the entry at position pos in the directory represented by the handle into buffer out_name_buffer
 * and writes the number of byte written to the buffer into out_name_len. The name should be null-terminated, unless it was too long.
 *
 * It assumes:
 *	- handle is open for reading (vfs_open())
 *
 * There is no function for listing a directory in vfs.h and the functions from dirent.h kinda suck, so here I have my own. */
static errno_t list_dir(int handle, aoff64_t pos, size_t buffer_size, char* out_name_buffer, ssize_t *out_name_len) {
	errno_t rc = EOK;
	rc = vfs_read_short(handle, pos, out_name_buffer, buffer_size, out_name_len);
	return rc;
}

/** Compares two vfs_stat_t structures in the sense that they represent the same data possibly across different filesystems. */
static bool stat_equal(vfs_stat_t a, vfs_stat_t b) {
	return a.is_file == b.is_file &&
		a.is_directory == b.is_directory &&
		a.size == b.size;
}

/** Compares the contetn of 2 files. It assumes:
 *  - both handles represent a file, not dir
 *  - both files have the same size
 *  - both buffers are the size CHUNK_SIZE */
static errno_t content_equal(int handle1, vfs_stat_t stat1, char* buffer1, int handle2, vfs_stat_t stat2, char* buffer2, bool *equal_out) {
	errno_t rc = EOK;
	bool equal = true;
	aoff64_t pos = 0;

	//todo open the files?

	while (pos < stat1.size && equal) {
		ssize_t read1, read2;
		rc = vfs_read_short(handle1, pos, buffer1, CHUNK_SIZE, &read1);
		if (rc != EOK) break;
		rc = vfs_read_short(handle2, pos, buffer2, CHUNK_SIZE, &read2);
		if (rc != EOK) break;
		size_t read = min(read1, read2);
		equal = (0 == memcmp(buffer1, buffer2, read));
		pos += read;
		if (read1 == 0 && read2 == 0) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Reading of files ended before their end. Considering them equal, but it is not supposed to happen.");
			break; // this should not happen. Prevent infinite loop.
		}
		if (read == 0) {
			// something bad has happened - one has read 0 while other not and we are not at the end yet. That is not possible
			log_msg(LOG_DEFAULT, LVL_ERROR, "Reading of one file ended sooner than the other.");
			equal = false;
			break;
		}
	}
	*equal_out = equal;
	//todo close the files. do it here?
	return rc;
}

/** Checks if the directory/file tree of both handles is equal -- checks presence of files/dirs, content of all files, and metadata. */
static errno_t trees_equal(int root_handle1, int root_handle2, bool* equal_out) {
	// todo don't forget to check put/close all file handles
	// Performs a depth-first search on handle1 and compares it to handle2.
	errno_t rc = EOK;
	bool equal = true;
	// stack of handles to compare
	// All handles on the stack are NOT open yet, but must be put() in the end
	struct stack s;
	rc = stack_new(&s);
	if (rc != EOK) goto close_end;

	char *file_buffer1 = malloc(CHUNK_SIZE);
	if (file_buffer1 == NULL) {
		rc = ENOMEM;
		goto close1;
	}
	char *file_buffer2 = malloc(CHUNK_SIZE);
	if (file_buffer2 == NULL) {
		rc = ENOMEM;
		goto close2;
	}
	char name_buffer[NAME_BUFFER_SIZE];

	// no need to add the initial entry into stack - it's the initial value in the for cycle
	// rc = stack_append(&s, (struct entry){handle1, handle2});
	// if (rc != EOK) goto close3;

	// While there is something in the stack
	for (struct entry item = {root_handle1, root_handle2}; // initial item
		rc != EEMPTY; // was pop successful?
		rc = stack_pop(&s, &item)
		) {
		if (rc != EOK) {
			// other error than EEMPTY
			goto close3;
		}
		//TODO open the handles here

		// 1. compare metadata
		// 2. open handles for reading
		// 3. if files, compare contents
		// 4. if directories, check that they contain the same items and add all items to stack
		vfs_stat_t stat1;
		rc = vfs_stat(item.handle1, &stat1);
		if (rc != EOK) goto close_inner;
		vfs_stat_t stat2;
		rc = vfs_stat(item.handle2, &stat2);
		if (rc != EOK) goto close_inner;

		rc = vfs_open(item.handle1, MODE_READ);
		if (rc != EOK) goto close_inner;
		rc = vfs_open(item.handle2, MODE_READ);
		if (rc != EOK) goto close_inner;

		equal = equal && stat_equal(stat1, stat2);
		if (!equal) {
			goto close_inner;
		}
		if ((stat1.is_file && stat1.is_directory) || (!stat1.is_file && !stat1.is_directory)) { // sanity check
			// WTF
			log_msg(LOG_DEFAULT, LVL_FATAL, "Invalid entry encountered: It either claims to be both a directory and file at the same time or it claims to be neither.");
			rc = EINVAL;
			goto close_inner;
		}
		if (stat1.is_file) {
			// check file content
			bool equal2;
			rc = content_equal(item.handle1, stat1, file_buffer1, item.handle2, stat2, file_buffer2, &equal2);
			if (rc != EOK) goto close_inner;
			equal = equal && equal2;
		}else {
			// check directory listing and add its items to the stack

			// 1. iterate over entries of handle1 -> gives you entry name
			// 2. lookup it in handle1
			// 3. lookup it in handle2
			// 4. add both new handles to stack
			// 5. iterate again over entries of handel2 -> gives you entry name
			// 6. try to look it up in handle1 jut to check that it exists

			// 5. and 6. are probably redundant, since the sizes of both directories must be equal.

			for (ssize_t pos = 0;
				((size_t) pos) < stat1.size;
				pos++) {
				// 1.
				ssize_t read;
				rc = list_dir(item.handle1, pos, NAME_BUFFER_SIZE, name_buffer, &read);
				log_msg(LOG_DEFAULT, LVL_DEBUG2, "Read directory entry name: result %s, read size: %"PRIdn".", str_error_name(rc), read);
				if (rc != EOK) break;
				// check that the string is null-terminated before the end of the buffer
				if (str_nsize(name_buffer, NAME_BUFFER_SIZE) >= NAME_BUFFER_SIZE) {
					rc = ENAMETOOLONG;
					break;
				}
				// 2.
				int entry_handle1; // don't forget to put
				rc = vfs_walk(item.handle1, name_buffer, WALK_DIRECTORY | WALK_REGULAR | WALK_MOUNT_POINT, &entry_handle1);
				if (rc != EOK) break;
				// 3.
				int entry_handle2; // must be put
				rc= vfs_walk(item.handle2, name_buffer, WALK_DIRECTORY | WALK_REGULAR | WALK_MOUNT_POINT, &entry_handle2);
				if (rc != EOK) {
					if (rc == ENOENT) { // todo is ENOENT the correct one?
						rc = EOK;
						equal = false;
					}
					errno_t rc2 = vfs_put(entry_handle1);
					(void) rc2; // if put failed, I guess there is nothing I can do about it
					break;
				}

				// 4.
				struct entry newitem = {entry_handle1, entry_handle2};
				rc = stack_append(&s, newitem);
				if (rc != EOK) {
					errno_t rc2 = vfs_put(entry_handle1);
					errno_t rc3 = vfs_put(entry_handle2);
					(void) rc2; // if put failed, I guess there is nothing I can do about it
					(void) rc3;
					break;
				}
			}
			if (rc != EOK) goto close_inner;

			// 5.
			for (ssize_t pos = 0;
				((size_t) pos) < stat2.size; // must be equal to stat1.size anyway
				pos++) {
				// 5.
				ssize_t read;
				rc = list_dir(item.handle2, pos, NAME_BUFFER_SIZE, name_buffer, &read);
				log_msg(LOG_DEFAULT, LVL_DEBUG2, "Read directory entry name: result %s, read size: %"PRIdn".", str_error_name(rc), read);
				if (rc != EOK) break;
				// check that the string is null-terminated before the end of the buffer
				if (str_nsize(name_buffer, NAME_BUFFER_SIZE) >= NAME_BUFFER_SIZE) {
					rc = ENAMETOOLONG;
					break;
				}
				// 6.
				int entry_handle; // don't forget to put
				rc = vfs_walk(item.handle2, name_buffer, WALK_DIRECTORY | WALK_REGULAR | WALK_MOUNT_POINT, &entry_handle);
				errno_t rc2 = vfs_put(entry_handle);
				(void) rc2; // if put failed, I guess there is nothing I can do about it
				if (rc != EOK) {
					if (rc == ENOENT) { // todo is ENOENT the correct one?
						rc = EOK;
						equal = false;
					}
					break;
				}
			}
			if (rc != EOK) goto close_inner;
		}

	close_inner:
		if (item.handle1 != root_handle1) {
			errno_t rc2 = vfs_put(item.handle1);
			(void) rc2; // if put failed, I guess there is nothing I can do about it
		}
		if (item.handle2 != root_handle2) {
			errno_t rc2 = vfs_put(item.handle2);
			(void) rc2; // if put failed, I guess there is nothing I can do about it
		}
		if (rc != EOK) goto close_start;
	}
	assert(rc == EEMPTY);
	rc = EOK;
close_start:
close3:
	free(file_buffer2);
close2:
	free(file_buffer1);
close1:
	(void) 0; // Clangd is not happy about declaration right after a label, so here is a dummy statement.
	struct entry item = {};
	for (errno_t for_rc = stack_pop(&s, &item);
		for_rc != EEMPTY; // was pop successful?
		for_rc = stack_pop(&s, &item)){
			errno_t rc2 = vfs_put(item.handle1);
			(void) rc2; // if put failed, I guess there is nothing I can do about it
			rc2 = vfs_put(item.handle2);
			(void) rc2; // if put failed, I guess there is nothing I can do about it
	}

	stack_free(&s);
close_end:
	if (rc == EOK) *equal_out = equal;
	return rc;
}


static void print_usage(void)
{
	printf("Syntax: %s [<options>] <dir1> <dir2> \n", NAME);
	printf("Recursively compare two driectories for equality\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h Print help\n");
	printf("Return code:\n");
	printf("  0 <dir1> and <dir2> are equal\n");
	printf("  1 <dir1> and <dir2> differ\n");
	printf("  2 An error occurred. Details in stderr\n");
}

/** @}
 */
