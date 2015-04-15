#include "conf/text_parse.h"

#include <assert.h>
#include <stdlib.h>

/** Constructor of parse structure */
void text_parse_init(text_parse_t *parse)
{
	assert(parse);
	list_initialize(&parse->errors);
	parse->has_error = false;
}

/** Destructor of parse structure
 *
 * It must be called before parse structure goes out of scope in order to avoid
 * memory leaks.
 */
void text_parse_deinit(text_parse_t *parse)
{
	assert(parse);
	list_foreach_safe(parse->errors, cur_link, next_link) {
		list_remove(cur_link);
		text_parse_error_t *error =
		    list_get_instance(cur_link, text_parse_error_t, link);
		free(error);
	}
}

/** Add an error to parse structure
 *
 * @param[in]  parse        the parse structure
 * @param[in]  lineno       line number where error originated (or zero)
 * @param[in]  parse_errno  user error code
 */
void text_parse_raise_error(text_parse_t *parse, size_t lineno,
    int parse_errno) {
	assert(parse);

	parse->has_error = true;

	text_parse_error_t *error = malloc(sizeof(text_parse_error_t));
	/* Silently ignore failed malloc, has_error flag is set anyway */
	if (!error) {
		return;
	}
	error->lineno = lineno;
	error->parse_errno = parse_errno;
	list_append(&error->link, &parse->errors);
}
