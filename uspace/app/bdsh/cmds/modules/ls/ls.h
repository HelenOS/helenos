#ifndef LS_H
#define LS_H

#include <vfs/vfs.h>

/* Various values that can be returned by ls_scope() */
#define LS_BOGUS 0
#define LS_FILE  1
#define LS_DIR   2

typedef struct {
	/* Options set at runtime. */
	unsigned int recursive;
	unsigned int sort;

} ls_job_t;

/** Structure to represent a directory entry.
 *
 * Useful to keep together important information
 * for sorting directory entries.
 */
struct dir_elem_t {
	char *name;
	vfs_stat_t s;
};

#endif
