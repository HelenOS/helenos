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

#include <dirent.h>
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
#include <io/logctl.h>

#define NAME  "cmpdirs"
/** When reading the content of a file for comparison, how large should one chunk be. */
#define CHUNK_SIZE 0x4000

// Macros to make handling errors less cumbersome
#define _check_ok(rc, action, ...) if ((rc) != EOK) {fprintf(stderr, __VA_ARGS__); log_msg(LOG_DEFAULT, LVL_ERROR, __VA_ARGS__ ); action; }
/** Print message (given by the variable arguments) to log and stderr if rc != EOK and then goto the specified label. */
#define check_ok(rc, label, ...) _check_ok(rc, goto label, __VA_ARGS__)
/** Print message (given by the variable arguments) to log and stderr if rc != EOK and then break. */
#define check_ok_break(rc, ...) _check_ok(rc, break, __VA_ARGS__)

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
			    "Unknown error while parsing command line options\n");
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
	logctl_set_log_level(NAME, LVL_DEBUG2);

	// lookup both directories
	int handle1;
	int handle2;
	rc = vfs_lookup(argv[0], 0, &handle1);
	check_ok(rc, close_end, "Failed to lookup \"%s\"\n", argv[0]);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "fd of \"%s\" is %d", argv[0], handle1);
	rc = vfs_lookup(argv[1], 0, &handle2);
	check_ok(rc, close1, "Failed to lookup \"%s\"\n", argv[1]);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "fd of \"%s\" is %d", argv[1], handle2);

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

/** Handles of 2 files or directories to compare */
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

/** Create a new growable stack. Don't forget to free it using stack_free when you are done using it. */
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

/** Returns the last entry in the stack and removes it from it. Does not free any memory. */
static errno_t stack_pop(struct stack *s, struct entry *entry_out) {
	if (s->size == 0) {
		return EEMPTY;
	}
	s->size--;
	(*entry_out) = s->buffer[s->size];
	return EOK;
}

/** Compares two vfs_stat_t structures in the sense that they represent the same data possibly across different filesystems. */
static bool stat_equal(vfs_stat_t a, vfs_stat_t b) {
	return a.is_file == b.is_file &&
		a.is_directory == b.is_directory &&
		a.size == b.size;
}

/** Compares the content of 2 files. It assumes:
 *  - both handles represent a file, not dir
 *  - both files have the same size
 *  - both buffers are the size CHUNK_SIZE
 *  - both handles are open for reading */
static errno_t content_equal(int handle1, vfs_stat_t stat1, char* buffer1, int handle2, vfs_stat_t stat2, char* buffer2, bool *equal_out) {
	errno_t rc = EOK;
	bool equal = true;
	aoff64_t pos = 0;

	while (pos < stat1.size && equal) {
		ssize_t read1, read2;
		rc = vfs_read_short(handle1, pos, buffer1, CHUNK_SIZE, &read1);
		check_ok_break(rc, "Failed to read handle %d\n", handle1);
		rc = vfs_read_short(handle2, pos, buffer2, CHUNK_SIZE, &read2);
		check_ok_break(rc, "Failed to read handle %d\n", handle2);
		const size_t read = min(read1, read2);
		equal = (0 == memcmp(buffer1, buffer2, read));
		pos += read;
		if (read1 == 0 && read2 == 0) {
			log_msg(LOG_DEFAULT, LVL_ERROR, "Reading of files ended before their end. Considering them equal, but it is not supposed to happen.");
			break; // this should not happen. Prevent infinite loop.
		}
		if (read == 0) {
			// something bad has happened - one has read 0 while other not and we are not at the end yet. That is not supposed to be possible
			log_msg(LOG_DEFAULT, LVL_ERROR, "Reading of one file ended sooner than the other.");
			equal = false;
			break;
		}
	}
	*equal_out = equal;
	return rc;
}

static errno_t trees_equal(int root_handle1, int root_handle2, bool* equal_out) {
	// todo Check once again that all file handles are put at the end
	// Performs a depth-first search on handle1 and compares it to handle2.
	errno_t rc = EOK;
	bool equal = true;

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "Root handles are: %d, %d\n", root_handle1, root_handle2);

	// stack of handles to compare
	// All handles on the stack are NOT open yet, but must be put() in the end
	struct stack s;
	rc = stack_new(&s);
	check_ok(rc, close_end, "Failed to create stack\n");

	char *file_buffer1 = malloc(CHUNK_SIZE);
	if (file_buffer1 == NULL) rc = ENOMEM;
	check_ok(rc, close1, "No memory for file buffer #1 (%d bytes needed)\n", CHUNK_SIZE);

	char *file_buffer2 = malloc(CHUNK_SIZE);
	if (file_buffer2 == NULL) rc = ENOMEM;
	check_ok(rc, close2, "No memory for file buffer #2 (%d bytes needed)\n", CHUNK_SIZE);

	// While there is something in the stack
	for (struct entry item = {root_handle1, root_handle2}; // initial item
		rc != EEMPTY; // was pop successful?
		rc = stack_pop(&s, &item)
		) {
		if (rc != EOK) {
			// other error than EEMPTY
			goto close3;
		}

		// 1. compare metadata
		// 2. open handles for reading
		// 3. if files, compare contents
		// 4. if directories, check that they contain the same items and add all items to stack
		vfs_stat_t stat1;
		rc = vfs_stat(item.handle1, &stat1);
		check_ok(rc, close_inner, "Cannot stat handle %d\n", item.handle1);
		vfs_stat_t stat2;
		rc = vfs_stat(item.handle2, &stat2);
		check_ok(rc, close_inner, "Cannot stat handle %d\n", item.handle2);

		rc = vfs_open(item.handle1, MODE_READ);
		check_ok(rc, close_inner, "Cannot open handle %d\n", item.handle1);
		rc = vfs_open(item.handle2, MODE_READ);
		check_ok(rc, close_inner, "Cannot open handle %d\n", item.handle2);

		equal = equal && stat_equal(stat1, stat2);
		if (!equal) {
			goto close_inner;
		}
		if ((stat1.is_file && stat1.is_directory) || (!stat1.is_file && !stat1.is_directory)) { // sanity check
			// WTF
			rc = EINVAL;
			check_ok(rc, close_inner, "Invalid entry encountered: It either claims to be both a directory and file at the same time or it claims to be neither.");
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

			// Since the directories represented by handle1 and handle2 have equal size and all entries in handle1 are
			// also present in handle2, then all entries in handle2 must also be present in handle1. Therefore they are equal.

			struct dirent *dp;
			DIR *dirp = opendir_handle(item.handle1);
			if (!dirp) {
				rc = errno;
				assert(rc != EOK);
			}
			check_ok(rc, close_inner, "Failed to open directory of handle %d", item.handle1);

			while ((dp = readdir(dirp))) { // 1.
				// Calculate the real length of the entry name (excluding null-terminator)
				size_t name_buffer_size = sizeof(dp->d_name);
				size_t name_size = str_nsize(dp->d_name, name_buffer_size);
				// check that the string is null-terminated before the end of the buffer
				if (name_size >= name_buffer_size) {
					rc = ENAMETOOLONG;
				}
				check_ok_break(rc, "Name too long (limit is %"PRIdn" excluding null-terminator)", name_buffer_size);
				// add slash to the beginning of the name, because vfs_walk requires the path to be absolute (will be
				// resolved relatively to the starting directory)
				char name_with_slash[name_buffer_size+1];
				name_with_slash[0] = '/';
				str_cpy(name_with_slash + 1, name_size + 1, dp->d_name);
				// 2.
				int entry_handle1; // don't forget to put
				rc = vfs_walk(item.handle1, name_with_slash, 0, &entry_handle1);
				check_ok_break(rc, "(0) Failed to find entry \"%s\" in directory of handle %d\n", name_with_slash, item.handle1);
				// 3.
				int entry_handle2; // must be put
				rc= vfs_walk(item.handle2, name_with_slash, 0, &entry_handle2);
				if (rc != EOK) {
					errno_t rc2 = vfs_put(entry_handle1);
					(void) rc2; // if put failed, I guess there is nothing I can do about it
					if (rc == ENOENT) { // todo is ENOENT the correct one?
						rc = EOK;
						equal = false;
						break;
					}
					check_ok_break(rc, "(1) Failed to find entry \"%s\" in directory of handle %d\n", name_with_slash, item.handle2);
				}

				// 4.
				struct entry newitem = {entry_handle1, entry_handle2};
				rc = stack_append(&s, newitem);
				if (rc != EOK) {
					errno_t rc2 = vfs_put(entry_handle1);
					errno_t rc3 = vfs_put(entry_handle2);
					(void) rc2; // if put failed, I guess there is nothing I can do about it
					(void) rc3;
				}
				check_ok_break(rc, "Failed to append to stack\n");
			}
			closedir(dirp);
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
		if (!equal) break;
	}
	assert(rc == EEMPTY || !equal);
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
