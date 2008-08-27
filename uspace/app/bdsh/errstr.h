#ifndef ERRSTR_H
#define ERRSTR_H

/* Simple array to translate error codes to meaningful strings */

static char *cl_errors[] = {
	"Success",
	"Failure",
	"Busy",
	"No Such Entry",
	"Not Enough Memory",
	"Permission Denied",
	"Method Not Supported",
	"Bad command or file name",
	"Entry already exists",
	"Object too large",
	NULL
};

static char *err2str(int);

#endif

