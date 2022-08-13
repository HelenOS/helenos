/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma warning(push, 0)
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#pragma warning(pop)


#define MAX_IDENTIFIER_LENGTH 256

static int counter = 0;

static void print_numbered_identifier(int value, FILE *output) {
	fprintf(output, "pcut_item_%d", value);
}

static void print_numbered_identifier2(int value, FILE *output) {
	fprintf(output, "pcut_item2_%d", value);
}

static void print_numbered_identifier3(int value, FILE *output) {
	fprintf(output, "pcut_item3_%d", value);
}

typedef struct {
	char name[MAX_IDENTIFIER_LENGTH];
	size_t length;
} identifier_t;

static void identifier_init(identifier_t *identifier) {
	identifier->name[0] = 0;
	identifier->length = 0;
}

static void identifier_add_char(identifier_t *identifier, char c) {
	if (identifier->length + 1 >= MAX_IDENTIFIER_LENGTH) {
		fprintf(stderr, "Identifier %s is too long, aborting!\n", identifier->name);
		exit(1);
	}

	identifier->name[identifier->length] = c;
	identifier->length++;
	identifier->name[identifier->length] = 0;
}

static void identifier_print_or_expand(identifier_t *identifier, FILE *output) {
	const char *name = identifier->name;
	if (strcmp(name, "PCUT_ITEM_NAME") == 0) {
		print_numbered_identifier(counter, output);
	} else if (strcmp(name, "PCUT_ITEM_NAME_PREV") == 0) {
		print_numbered_identifier(counter - 1, output);
	} else if (strcmp(name, "PCUT_ITEM_COUNTER_INCREMENT") == 0) {
		counter++;
	} else if (strcmp(name, "PCUT_ITEM2_NAME") == 0) {
		print_numbered_identifier2(counter, output);
	} else if (strcmp(name, "PCUT_ITEM3_NAME") == 0) {
		print_numbered_identifier3(counter, output);
	} else {
		fprintf(output, "%s", name);
	}
}

static int is_identifier_char(int c, int inside_identifier) {
	return isalpha(c) || (c == '_')
			|| (inside_identifier && isdigit(c));
}

int main(int argc, char *argv[]) {
	FILE *input = stdin;
	FILE *output = stdout;

	int last_char_was_identifier = 0;
	identifier_t last_identifier;

	/* Unused parameters. */
	(void) argc;
	(void) argv;

	while (1) {
		int current_char_denotes_identifier;

		int current_char = fgetc(input);
		if (current_char == EOF) {
			break;
		}

		current_char_denotes_identifier = is_identifier_char(current_char, last_char_was_identifier);
		if (current_char_denotes_identifier) {
			if (!last_char_was_identifier) {
				identifier_init(&last_identifier);
			}
			identifier_add_char(&last_identifier, current_char);
			last_char_was_identifier = 1;
		} else {
			if (last_char_was_identifier) {
				identifier_print_or_expand(&last_identifier, output);
			}
			fprintf(stdout, "%c", current_char);
			last_char_was_identifier = 0;
		}
	}

	if (last_char_was_identifier) {
		identifier_print_or_expand(&last_identifier, output);
	}
	fprintf(output, "\n");

	return 0;
}
