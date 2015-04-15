#ifndef _CONF_CONFIGURATION_H
#define _CONF_CONFIGURATION_H

#include <conf/ini.h>
#include <conf/text_parse.h>
#include <stdbool.h>

/** Structure represents a declaration of a configuration value */
typedef struct {
	/** Value name */
	const char *name;
	
	/** Parse and store given string
	 *
	 * @param[in]  string  string to parse (it's whitespace-trimmed)
	 * @param[out] dst     pointer where to store parsed result (must be
	 *                     preallocated)
	 * @param[out] parse   parse struct to store parsing errors
	 * @param[in]  lineno  line number of the original string
	 *
	 * @return  true on success
	 * @return  false on error (e.g. format error, low memory)
	 */
	bool (*parse)(const char *, void *, text_parse_t *, size_t);

	/** Offset in structure where parsed value ought be stored */
	size_t offset;

	/** String representation of default value
	 *
	 * Format is same as in input. NULL value denotes required
	 * configuration value.
	 */
	const char *default_value;
} config_item_t;

/** Code of configuration processing error */
typedef enum {
	CONFIGURATION_EMISSING_ITEM = -1
} config_error_t;

#define CONFIGURATION_ITEM_SENTINEL {NULL}

extern int config_load_ini_section(config_item_t *, ini_section_t *, void *,
    text_parse_t *);

extern bool config_parse_string(const char *, void *, text_parse_t *, size_t);

#endif
