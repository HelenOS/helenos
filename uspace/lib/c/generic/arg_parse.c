/*
 * Copyright (c) 2009 Lukas Mejdrech
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @file
 * Command-line arguments parsing functions
 */

#include <arg_parse.h>
#include <errno.h>
#include <str.h>

int arg_parse_short_long(const char *arg, const char *ashort, const char *along)
{
	if (str_cmp(arg, ashort) == 0)
		return 0;

	if (str_lcmp(arg, along, str_length(along)) == 0)
		return str_length(along);

	return -1;
}

/** Parse the next argument as an integer.
 *
 * The actual argument is pointed by the index.
 * Parse the offseted argument value if the offset is set
 * or the next one if not.
 *
 * @param[in]     argc   The total number of arguments.
 * @param[in]     argv   The arguments.
 * @param[in,out] index  The actual argument index. The index is incremented
 *                       by the number of processed arguments.
 * @param[out]    value  The parsed argument value.
 * @param[in]     offset The value offset in the actual argument. If not set,
 *                       the next argument is parsed instead.
 *
 * @return EOK on success.
 * @return ENOENT if the argument is missing.
 * @return EINVAL if the argument is in wrong format.
 *
 */
errno_t arg_parse_int(int argc, char *argv[], int *index, int *value,
    int offset)
{
	char *rest;

	if (offset)
		*value = strtol(argv[*index] + offset, &rest, 10);
	else if ((*index) + 1 < argc) {
		(*index)++;
		*value = strtol(argv[*index], &rest, 10);
	} else
		return ENOENT;

	if ((rest) && (*rest))
		return EINVAL;

	return EOK;
}

/** Parse the next named argument as an integral number.
 *
 * The actual argument is pointed by the index.
 * Parse the offseted actual argument if the offset is set or the next
 * one if not. Translate the argument using the parse_value function.
 * Increment the actual index by the number of processed arguments.
 *
 * @param[in]     argc        The total number of arguments.
 * @param[in]     argv        The arguments.
 * @param[in,out] index       The actual argument index. The index is
 *                            incremented by the number of processed arguments.
 * @param[out]    value       The parsed argument value.
 * @param[in]     offset      The value offset in the actual argument. If not
 *                            set, the next argument is parsed instead.
 * @param[in]     parse_value The translation function to parse the named value.
 *
 * @return EOK on success.
 * @return ENOENT if the argument is missing.
 * @return EINVAL if the argument name has not been found.
 *
 */
errno_t arg_parse_name_int(int argc, char *argv[], int *index, int *value,
    int offset, arg_parser parser)
{
	char *arg;

	errno_t ret = arg_parse_string(argc, argv, index, &arg, offset);
	if (ret != EOK)
		return ret;

	return parser(arg, value);
}

/** Parse the next argument as a character string.
 *
 * The actual argument is pointed by the index.
 * Parse the offseted actual argument value if the offset is set or the next
 * one if not. Increment the actual index by the number of processed arguments.
 *
 * @param[in]     argc   The total number of arguments.
 * @param[in]     argv   The arguments.
 * @param[in,out] index  The actual argument index. The index is
 *                       incremented by the number of processed arguments.
 * @param[out]    value  The parsed argument value.
 * @param[in]     offset The value offset in the actual argument. If not set,
 *                       the next argument is parsed instead.
 *
 * @return EOK on success.
 * @return ENOENT if the parameter is missing.
 *
 */
errno_t arg_parse_string(int argc, char **argv, int *index, char **value,
    int offset)
{
	if (offset)
		*value = argv[*index] + offset;
	else if ((*index) + 1 < argc) {
		(*index)++;
		*value = argv[*index];
	} else
		return ENOENT;

	return EOK;
}

/** @}
 */
