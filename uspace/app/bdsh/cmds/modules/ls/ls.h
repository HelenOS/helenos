#ifndef LS_H
#define LS_H

/* Various values that can be returned by ls_scope() */
#define LS_BOGUS 0
#define LS_FILE  1
#define LS_DIR   2

/* Protoypes for non entry points, intrinsic to ls. Stuff like ls_scope()
 * is also duplicated in rm, while rm sort of duplicates ls_scan_dir().
 * TODO make some more shared functions and don't expose the stuff below */
extern unsigned int ls_scope(const char *);
extern void ls_scan_dir(const char *, DIR *);
extern void ls_print_dir(const char *);
extern void ls_print_file(const char *);

#endif /* LS_H */

