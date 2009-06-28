#ifndef LS_H
#define LS_H

/* Various values that can be returned by ls_scope() */
#define LS_BOGUS 0
#define LS_FILE  1
#define LS_DIR   2


static unsigned int ls_scope(const char *);
static void ls_scan_dir(const char *, DIR *);
static void ls_print(const char *, const char *);

#endif /* LS_H */

