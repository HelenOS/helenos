/*	$NetBSD: redir.c,v 1.22 2000/05/22 10:18:47 elric Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)redir.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: redir.c,v 1.22 2000/05/22 10:18:47 elric Exp $");
#endif
#endif /* not lint */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>	/* PIPE_BUF */
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Code for dealing with input/output redirection.
 */

#include "shell.h"
#include "nodes.h"
#include "jobs.h"
#include "expand.h"
#include "redir.h"
#include "output.h"
#include "memalloc.h"
#include "error.h"
#include "options.h"


#define EMPTY -2		/* marks an unused slot in redirtab */
#ifndef PIPE_BUF
# define PIPESIZE 4096		/* amount of buffering in a pipe */
#else
# define PIPESIZE PIPE_BUF
#endif


MKINIT
struct redirtab {
	struct redirtab *next;
	short renamed[10];
};


MKINIT struct redirtab *redirlist;

/*
 * We keep track of whether or not fd0 has been redirected.  This is for
 * background commands, where we want to redirect fd0 to /dev/null only
 * if it hasn't already been redirected.
*/
int fd0_redirected = 0;

/*
 * We also keep track of where fd2 goes.
 */
int fd2 = 2;

STATIC int openredirect (union node *);
STATIC void dupredirect (union node *, int, char[10 ]);
STATIC int openhere (union node *);
STATIC int noclobberopen (const char *);


/*
 * Process a list of redirection commands.  If the REDIR_PUSH flag is set,
 * old file descriptors are stashed away so that the redirection can be
 * undone by calling popredir.  If the REDIR_BACKQ flag is set, then the
 * standard output, and the standard error if it becomes a duplicate of
 * stdout, is saved in memory.
 */

void
redirect(redir, flags)
	union node *redir;
	int flags;
	{
	union node *n;
	struct redirtab *sv = NULL;
	int i;
	int fd;
	int newfd;
	int try;
	char memory[10];	/* file descriptors to write to memory */

	for (i = 10 ; --i >= 0 ; )
		memory[i] = 0;
	memory[1] = flags & REDIR_BACKQ;
	if (flags & REDIR_PUSH) {
		sv = ckmalloc(sizeof (struct redirtab));
		for (i = 0 ; i < 10 ; i++)
			sv->renamed[i] = EMPTY;
		sv->next = redirlist;
		redirlist = sv;
	}
	for (n = redir ; n ; n = n->nfile.next) {
		fd = n->nfile.fd;
		try = 0;
		if ((n->nfile.type == NTOFD || n->nfile.type == NFROMFD) &&
		    n->ndup.dupfd == fd)
			continue; /* redirect from/to same file descriptor */

		INTOFF;
		newfd = openredirect(n);
		if (((flags & REDIR_PUSH) && sv->renamed[fd] == EMPTY) ||
		    (fd == fd2)) {
			if (newfd == fd) {
				try++;
			} else if ((i = fcntl(fd, F_DUPFD, 10)) == -1) {
				switch (errno) {
				case EBADF:
					if (!try) {
						dupredirect(n, newfd, memory);
						try++;
						break;
					}
					/* FALLTHROUGH*/
				default:
					if (newfd >= 0) {
						close(newfd);
					}
					INTON;
					error("%d: %s", fd, strerror(errno));
					/* NOTREACHED */
				}
			}
			if (!try) {
				close(fd);
				if (flags & REDIR_PUSH) {
					sv->renamed[fd] = i;
				}
				if (fd == fd2) {
					fd2 = i;
				}
			}
		} else if (fd != newfd) {
			close(fd);
		}
                if (fd == 0)
                        fd0_redirected++;
		if (!try)
			dupredirect(n, newfd, memory);
		INTON;
	}
	if (memory[1])
		out1 = &memout;
	if (memory[2])
		out2 = &memout;
}


STATIC int
openredirect(redir)
	union node *redir;
	{
	char *fname;
	int f;

	switch (redir->nfile.type) {
	case NFROM:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_RDONLY)) < 0)
			goto eopen;
		break;
	case NFROMTO:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)
			goto ecreate;
		break;
	case NTO:
		/* Take care of noclobber mode. */
		if (Cflag) {
			fname = redir->nfile.expfname;
			if ((f = noclobberopen(fname)) < 0)
				goto ecreate;
			break;
		}
	case NTOOV:
		fname = redir->nfile.expfname;
#ifdef O_CREAT
		if ((f = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
			goto ecreate;
#else
		if ((f = creat(fname, 0666)) < 0)
			goto ecreate;
#endif
		break;
	case NAPPEND:
		fname = redir->nfile.expfname;
#ifdef O_APPEND
		if ((f = open(fname, O_WRONLY|O_CREAT|O_APPEND, 0666)) < 0)
			goto ecreate;
#else
		if ((f = open(fname, O_WRONLY)) < 0
		 && (f = creat(fname, 0666)) < 0)
			goto ecreate;
		lseek(f, (off_t)0, 2);
#endif
		break;
	case NTOFD:
	case NFROMFD:
		f = -1;
		break;
	case NHERE:
	case NXHERE:
		f = openhere(redir);
		break;
	default:
		abort();
	}

	return f;
ecreate:
	error("cannot create %s: %s", fname, errmsg(errno, E_CREAT));
eopen:
	error("cannot open %s: %s", fname, errmsg(errno, E_OPEN));
}


STATIC void
dupredirect(redir, f, memory)
	union node *redir;
	int f;
	char memory[10];
	{
	int fd = redir->nfile.fd;

	memory[fd] = 0;
	if (redir->nfile.type == NTOFD || redir->nfile.type == NFROMFD) {
		if (redir->ndup.dupfd >= 0) {	/* if not ">&-" */
			if (memory[redir->ndup.dupfd])
				memory[fd] = 1;
			else
				copyfd(redir->ndup.dupfd, fd);
		}
		return;
	}

	if (f != fd) {
		copyfd(f, fd);
		close(f);
	}
	return;
}


/*
 * Handle here documents.  Normally we fork off a process to write the
 * data to a pipe.  If the document is short, we can stuff the data in
 * the pipe without forking.
 */

STATIC int
openhere(redir)
	union node *redir;
	{
	int pip[2];
	int len = 0;

	if (pipe(pip) < 0)
		error("Pipe call failed");
	if (redir->type == NHERE) {
		len = strlen(redir->nhere.doc->narg.text);
		if (len <= PIPESIZE) {
			xwrite(pip[1], redir->nhere.doc->narg.text, len);
			goto out;
		}
	}
	if (forkshell((struct job *)NULL, (union node *)NULL, FORK_NOJOB) == 0) {
		close(pip[0]);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
#ifdef SIGTSTP
		signal(SIGTSTP, SIG_IGN);
#endif
		signal(SIGPIPE, SIG_DFL);
		if (redir->type == NHERE)
			xwrite(pip[1], redir->nhere.doc->narg.text, len);
		else
			expandhere(redir->nhere.doc, pip[1]);
		_exit(0);
	}
out:
	close(pip[1]);
	return pip[0];
}



/*
 * Undo the effects of the last redirection.
 */

void
popredir() {
	struct redirtab *rp = redirlist;
	int i;

	INTOFF;
	for (i = 0 ; i < 10 ; i++) {
		if (rp->renamed[i] != EMPTY) {
                        if (i == 0)
                                fd0_redirected--;
			close(i);
			if (rp->renamed[i] >= 0) {
				copyfd(rp->renamed[i], i);
				close(rp->renamed[i]);
			}
			if (rp->renamed[i] == fd2) {
				fd2 = i;
			}
		}
	}
	redirlist = rp->next;
	ckfree(rp);
	INTON;
}

/*
 * Undo all redirections.  Called on error or interrupt.
 */

#ifdef mkinit

INCLUDE "redir.h"

RESET {
	while (redirlist)
		popredir();
}

SHELLPROC {
	clearredir();
}

#endif

/* Return true if fd 0 has already been redirected at least once.  */
int
fd0_redirected_p () {
        return fd0_redirected != 0;
}

/*
 * Discard all saved file descriptors.
 */

void
clearredir() {
	struct redirtab *rp;
	int i;

	for (rp = redirlist ; rp ; rp = rp->next) {
		for (i = 0 ; i < 10 ; i++) {
			if (rp->renamed[i] >= 0) {
				close(rp->renamed[i]);
				if (rp->renamed[i] == fd2) {
					fd2 = -1;
				}
			}
			rp->renamed[i] = EMPTY;
		}
	}
}



/*
 * Copy a file descriptor to be >= to.  Returns -1
 * if the source file descriptor is closed, EMPTY if there are no unused
 * file descriptors left.
 */

int
copyfd(from, to)
	int from;
	int to;
{
	int newfd;

	newfd = fcntl(from, F_DUPFD, to);
	if (newfd < 0) {
		if (errno == EMFILE)
			return EMPTY;
		else
			error("%d: %s", from, strerror(errno));
	}
	return newfd;
}

/*
 * Open a file in noclobber mode.
 * The code was copied from bash.
 */
int
noclobberopen(fname)
	const char *fname;
{
	int r, fd;
	struct stat finfo, finfo2;

	/*
	 * If the file exists and is a regular file, return an error
	 * immediately.
	 */
	r = stat(fname, &finfo);
	if (r == 0 && S_ISREG(finfo.st_mode)) {
		errno = EEXIST;
		return -1;
	}

	/*
	 * If the file was not present (r != 0), make sure we open it
	 * exclusively so that if it is created before we open it, our open
	 * will fail.  Make sure that we do not truncate an existing file.
	 * Note that we don't turn on O_EXCL unless the stat failed -- if the
	 * file was not a regular file, we leave O_EXCL off.
	 */
	if (r != 0)
		return open(fname, O_WRONLY|O_CREAT|O_EXCL, 0666);
	fd = open(fname, O_WRONLY|O_CREAT, 0666);

	/* If the open failed, return the file descriptor right away. */
	if (fd < 0)
		return fd;

	/*
	 * OK, the open succeeded, but the file may have been changed from a
	 * non-regular file to a regular file between the stat and the open.
	 * We are assuming that the O_EXCL open handles the case where FILENAME
	 * did not exist and is symlinked to an existing file between the stat
	 * and open.
	 */

	/*
	 * If we can open it and fstat the file descriptor, and neither check
	 * revealed that it was a regular file, and the file has not been
	 * replaced, return the file descriptor.
	 */
	 if (fstat(fd, &finfo2) == 0 && !S_ISREG(finfo2.st_mode) &&
	     finfo.st_dev == finfo2.st_dev && finfo.st_ino == finfo2.st_ino)
	 	return fd;

	/* The file has been replaced.  badness. */
	close(fd);
	errno = EEXIST;
	return -1;
}
