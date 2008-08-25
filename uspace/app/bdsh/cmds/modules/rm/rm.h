#ifndef RM_H
#define RM_H

/* Return values for rm_scope() */
#define RM_BOGUS 0
#define RM_FILE  1
#define RM_DIR   2

/* Flags for rm_update() */
#define _RM_ENTRY   0
#define _RM_ADVANCE 1
#define _RM_REWIND  2
#define _RM_EXIT    3

/* A simple job structure */
typedef struct {
	/* Options set at run time */
	unsigned int force;      /* -f option */
	unsigned int recursive;  /* -r option */
	unsigned int safe;       /* -s option */

	/* Keeps track of the job in progress */
	int advance; /* How far deep we've gone since entering */
	DIR *entry;  /* Entry point to the tree being removed */
	char *owd;   /* Where we were when we invoked rm */
	char *cwd;   /* Current directory being transversed */
	char *nwd;   /* Next directory to be transversed */

	/* Counters */
	int f_removed; /* Number of files unlinked */
	int d_removed; /* Number of directories unlinked */
} rm_job_t;


/* Prototypes for the rm command, excluding entry points */
static unsigned int rm_start(rm_job_t *);
static void rm_end(rm_job_t *rm);
static unsigned int rm_recursive(const char *);
static unsigned int rm_single(const char *);
static unsigned int rm_scope(const char *);

#endif /* RM_H */

