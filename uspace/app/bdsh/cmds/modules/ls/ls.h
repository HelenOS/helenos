#ifndef LS_H
#define LS_H

#include <vfs/vfs.h>

/* Various values that can be returned by ls_scope() */
#define LS_BOGUS 0
#define LS_FILE  1
#define LS_DIR   2

/** Structure to represent a directory entry.
 *
 * Useful to keep together important information
 * for sorting directory entries.
 */
struct dir_elem_t {
	char *name;
	vfs_stat_t s;
};

typedef struct {
	/* Options set at runtime. */
	unsigned int recursive;
	unsigned int sort;

	bool single_column;
	bool exact_size;

	errno_t (*printer)(struct dir_elem_t *);
} ls_job_t;

#endif
