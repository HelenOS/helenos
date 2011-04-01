#ifndef LS_H
#define LS_H

/* Various values that can be returned by ls_scope() */
#define LS_BOGUS 0
#define LS_FILE  1
#define LS_DIR   2

static void ls_scan_dir(const char *, DIR *, int);
static void ls_print(const char *, const char *);

/** Structure to represent a directory entry.
 *
 * Useful to keep together important informations
 * for sorting directory entries.
 */
struct dir_elem_t {
	char * name;
	int isdir;
};

#endif /* LS_H */

