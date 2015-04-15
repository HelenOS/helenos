#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#include "util.h"

/** Compose path to file inside a directory.
 *
 * @return	pointer to newly allocated string
 * @return	NULL on error
 */
char *compose_path(const char *dirname, const char *filename)
{
	size_t size = str_size(dirname) + str_size(filename) + 2;
	char *result = malloc(sizeof(char) * size);

	if (result == NULL) {
		return NULL;
	}
	if (snprintf(result, size, "%s/%s", dirname, filename) < 0) {
		return NULL;
	}
	return result;
}
