#ifndef _CONF_TEXT_PARSE_H
#define _CONF_TEXT_PARSE_H

#include <adt/list.h>
#include <stdbool.h>

/** Container of (recoverable) errors that could occur while parsing */
typedef struct {
	/** List of text_parse_error_t */
	list_t errors;

	/** Boolean flag to indicate errors (in case list couldn't be filled) */
	bool has_error;
} text_parse_t;

/** An error of parsing */
typedef struct {
	link_t link;

	/** Line where the error originated */
	size_t lineno;

	/** Code of parse error (name is chosen not to clash with errno) */
	int parse_errno;
} text_parse_error_t;


extern void text_parse_init(text_parse_t *);
extern void text_parse_deinit(text_parse_t *);

extern void text_parse_raise_error(text_parse_t *, size_t, int);

#endif
