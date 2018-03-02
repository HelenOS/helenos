/*
 * Copyright (c) 2016 Martin Decky
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

/** @addtogroup vcalc
 * @{
 */
/** @file
 *
 * Inspired by the code released at https://github.com/osgroup/HelenOSProject
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <io/pixel.h>
#include <task.h>
#include <window.h>
#include <grid.h>
#include <button.h>
#include <label.h>
#include <ctype.h>
#include <stdlib.h>
#include <str.h>

#define NAME  "vcalc"

#define NULL_DISPLAY  "."

#define SYNTAX_ERROR_DISPLAY   "syntax error"
#define NUMERIC_ERROR_DISPLAY  "numerical error"
#define UNKNOWN_ERROR_DISPLAY  "unknown error"

typedef enum {
	STATE_INITIAL = 0,
	STATE_FINISH,
	STATE_ERROR,
	STATE_DIGIT,
	STATE_VALUE
} parser_state_t;

typedef enum {
	ERROR_SYNTAX = 0,
	ERROR_NUMERIC
} error_type_t;

typedef enum {
	OPERATOR_NONE = 0,
	OPERATOR_ADD,
	OPERATOR_SUB,
	OPERATOR_MUL,
	OPERATOR_DIV
} operator_t;

typedef enum {
	ITEM_VALUE = 0,
	ITEM_OPERATOR
} stack_item_type_t;

typedef struct {
	link_t link;

	stack_item_type_t type;
	union {
		int64_t value;
		operator_t operator;
	} data;
} stack_item_t;

static char *expr = NULL;
static label_t *display;

static bool is_digit(char c)
{
	return ((c >= '0') && (c <= '9'));
}

static int get_digit(char c)
{
	assert(is_digit(c));

	return (c - '0');
}

static bool is_plus(char c)
{
	return (c == '+');
}

static bool is_minus(char c)
{
	return (c == '-');
}

static bool is_finish(char c)
{
	return (c == 0);
}

static operator_t get_operator(char c)
{
	switch (c) {
	case '+':
		return OPERATOR_ADD;
	case '-':
		return OPERATOR_SUB;
	case '*':
		return OPERATOR_MUL;
	case '/':
		return OPERATOR_DIV;
	default:
		return OPERATOR_NONE;
	}
}

static bool is_operator(char c)
{
	return (get_operator(c) != OPERATOR_NONE);
}

static bool stack_push_value(list_t *stack, int64_t value, bool value_neg)
{
	stack_item_t *item = malloc(sizeof(stack_item_t));
	if (!item)
		return false;

	link_initialize(&item->link);
	item->type = ITEM_VALUE;

	if (value_neg)
		item->data.value = -value;
	else
		item->data.value = value;

	list_prepend(&item->link, stack);

	return true;
}

static bool stack_push_operator(list_t *stack, operator_t operator)
{
	stack_item_t *item = malloc(sizeof(stack_item_t));
	if (!item)
		return false;

	link_initialize(&item->link);
	item->type = ITEM_OPERATOR;
	item->data.operator = operator;
	list_prepend(&item->link, stack);

	return true;
}

static bool stack_pop_value(list_t *stack, int64_t *value)
{
	link_t *link = list_first(stack);
	if (!link)
		return false;

	stack_item_t *item = list_get_instance(link, stack_item_t, link);
	if (item->type != ITEM_VALUE)
		return false;

	*value = item->data.value;

	list_remove(link);
	free(item);

	return true;
}

static bool stack_pop_operator(list_t *stack, operator_t *operator)
{
	link_t *link = list_first(stack);
	if (!link)
		return false;

	stack_item_t *item = list_get_instance(link, stack_item_t, link);
	if (item->type != ITEM_OPERATOR)
		return false;

	*operator = item->data.operator;

	list_remove(link);
	free(item);

	return true;
}

static void stack_cleanup(list_t *stack)
{
	while (!list_empty(stack)) {
		link_t *link = list_first(stack);
		if (link) {
			stack_item_t *item = list_get_instance(link, stack_item_t,
			    link);

			list_remove(link);
			free(item);
		}
	}
}

static bool compute(int64_t a, operator_t operator, int64_t b, int64_t *value)
{
	switch (operator) {
	case OPERATOR_ADD:
		*value = a + b;
		break;
	case OPERATOR_SUB:
		*value = a - b;
		break;
	case OPERATOR_MUL:
		*value = a * b;
		break;
	case OPERATOR_DIV:
		if (b == 0)
			return false;

		*value = a / b;
		break;
	default:
		return false;
	}

	return true;
}

static unsigned int get_priority(operator_t operator)
{
	switch (operator) {
	case OPERATOR_ADD:
		return 0;
	case OPERATOR_SUB:
		return 0;
	case OPERATOR_MUL:
		return 1;
	case OPERATOR_DIV:
		return 1;
	default:
		return 0;
	}
}

static void evaluate(list_t *stack, int64_t *value, parser_state_t *state,
    error_type_t *error_type)
{
	while (!list_empty(stack)) {
		if (!stack_pop_value(stack, value)) {
			*state = STATE_ERROR;
			*error_type = ERROR_SYNTAX;
			break;
		}

		if (!list_empty(stack)) {
			operator_t operator;
			if (!stack_pop_operator(stack, &operator)) {
				*state = STATE_ERROR;
				*error_type = ERROR_SYNTAX;
				break;
			}

			int64_t value_a;
			if (!stack_pop_value(stack, &value_a)) {
				*state = STATE_ERROR;
				*error_type = ERROR_SYNTAX;
				break;
			}

			if (!compute(value_a, operator, *value, value)) {
				*state = STATE_ERROR;
				*error_type = ERROR_NUMERIC;
				break;
			}

			if (!stack_push_value(stack, *value, false)) {
				*state = STATE_ERROR;
				*error_type = ERROR_SYNTAX;
				break;
			}
		}
	}
}

static void display_update(void)
{
	if (expr != NULL)
		display->rewrite(&display->widget, (void *) expr);
	else
		display->rewrite(&display->widget, (void *) NULL_DISPLAY);
}

static void display_error(error_type_t error_type)
{
	if (expr != NULL) {
		free(expr);
		expr = NULL;
	}

	switch (error_type) {
	case ERROR_SYNTAX:
		display->rewrite(&display->widget, (void *) SYNTAX_ERROR_DISPLAY);
		break;
	case ERROR_NUMERIC:
		display->rewrite(&display->widget, (void *) NUMERIC_ERROR_DISPLAY);
		break;
	default:
		display->rewrite(&display->widget, (void *) UNKNOWN_ERROR_DISPLAY);
	}
}

static void on_btn_click(widget_t *widget, void *data)
{
	const char *subexpr = (const char *) widget_get_data(widget);

	if (expr != NULL) {
		char *new_expr;

		asprintf(&new_expr, "%s%s", expr, subexpr);
		free(expr);
		expr = new_expr;
	} else
		expr = str_dup(subexpr);

	display_update();
}

static void on_c_click(widget_t *widget, void *data)
{
	if (expr != NULL) {
		free(expr);
		expr = NULL;
	}

	display_update();
}

static void on_eval_click(widget_t *widget, void *data)
{
	if (expr == NULL)
		return;

	list_t stack;
	list_initialize(&stack);

	error_type_t error_type = ERROR_SYNTAX;
	size_t i = 0;
	parser_state_t state = STATE_INITIAL;
	int64_t value = 0;
	bool value_neg = false;
	operator_t last_operator = OPERATOR_NONE;

	while ((state != STATE_FINISH) && (state != STATE_ERROR)) {
		switch (state) {
		case STATE_INITIAL:
			if (is_digit(expr[i])) {
				value = get_digit(expr[i]);
				i++;
				state = STATE_VALUE;
			} else if (is_plus(expr[i])) {
				i++;
				value_neg = false;
				state = STATE_DIGIT;
			} else if (is_minus(expr[i])) {
				i++;
				value_neg = true;
				state = STATE_DIGIT;
			} else
				state = STATE_ERROR;
			break;

		case STATE_DIGIT:
			if (is_digit(expr[i])) {
				value = get_digit(expr[i]);
				i++;
				state = STATE_VALUE;
			} else
				state = STATE_ERROR;
			break;

		case STATE_VALUE:
			if (is_digit(expr[i])) {
				value *= 10;
				value += get_digit(expr[i]);
				i++;
			} else if (is_operator(expr[i])) {
				if (!stack_push_value(&stack, value, value_neg)) {
					state = STATE_ERROR;
					break;
				}

				value = 0;
				value_neg = false;

				operator_t operator = get_operator(expr[i]);

				if (get_priority(operator) <= get_priority(last_operator)) {
					evaluate(&stack, &value, &state, &error_type);
					if (state == STATE_ERROR)
						break;

					if (!stack_push_value(&stack, value, value_neg)) {
						state = STATE_ERROR;
						break;
					}
				}

				if (!stack_push_operator(&stack, operator)) {
					state = STATE_ERROR;
					break;
				}

				last_operator = operator;
				i++;
				state = STATE_DIGIT;
			} else if (is_finish(expr[i])) {
				if (!stack_push_value(&stack, value, value_neg)) {
					state = STATE_ERROR;
					break;
				}

				state = STATE_FINISH;
			} else
				state = STATE_ERROR;
			break;

		default:
			state = STATE_ERROR;
		}
	}

	evaluate(&stack, &value, &state, &error_type);
	stack_cleanup(&stack);

	if (state == STATE_ERROR) {
		display_error(error_type);
		return;
	}

	free(expr);
	asprintf(&expr, "%" PRId64, value);
	display_update();
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("%s: Compositor server not specified.\n", NAME);
		return 1;
	}

	window_t *main_window = window_open(argv[1], NULL,
	    WINDOW_MAIN | WINDOW_DECORATED | WINDOW_RESIZEABLE, NAME);
	if (!main_window) {
		printf("%s: Cannot open main window.\n", NAME);
		return 2;
	}

	pixel_t grd_bg = PIXEL(255, 240, 240, 240);

	pixel_t btn_bg = PIXEL(255, 0, 0, 0);
	pixel_t btn_fg = PIXEL(200, 200, 200, 200);

	pixel_t lbl_bg = PIXEL(255, 240, 240, 240);
	pixel_t lbl_fg = PIXEL(255, 0, 0, 0);

	grid_t *grid = create_grid(window_root(main_window), NULL, 4, 5, grd_bg);

	display = create_label(NULL, NULL, NULL_DISPLAY, 16, lbl_bg, lbl_fg);

	button_t *btn_1 = create_button(NULL, "1", "1", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_2 = create_button(NULL, "2", "2", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_3 = create_button(NULL, "3", "3", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_4 = create_button(NULL, "4", "4", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_5 = create_button(NULL, "5", "5", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_6 = create_button(NULL, "6", "6", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_7 = create_button(NULL, "7", "7", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_8 = create_button(NULL, "8", "8", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_9 = create_button(NULL, "9", "9", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_0 = create_button(NULL, "0", "0", 16, btn_bg, btn_fg,
	    lbl_fg);

	button_t *btn_add = create_button(NULL, "+", "+", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_sub = create_button(NULL, "-", "-", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_mul = create_button(NULL, "*", "*", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_div = create_button(NULL, "/", "/", 16, btn_bg, btn_fg,
	    lbl_fg);

	button_t *btn_eval = create_button(NULL, NULL, "=", 16, btn_bg, btn_fg,
	    lbl_fg);
	button_t *btn_c = create_button(NULL, NULL, "C", 16, btn_bg, btn_fg,
	    lbl_fg);

	if ((!grid) || (!display) || (!btn_1) || (!btn_2) || (!btn_3) ||
	    (!btn_4) || (!btn_5) || (!btn_6) || (!btn_7) || (!btn_8) ||
	    (!btn_9) || (!btn_0) || (!btn_add) || (!btn_sub) || (!btn_mul) ||
	    (!btn_div) || (!btn_eval) || (!btn_c)) {
		window_close(main_window);
		printf("%s: Cannot create widgets.\n", NAME);
		return 3;
	}

	sig_connect(&btn_1->clicked, &btn_1->widget, on_btn_click);
	sig_connect(&btn_2->clicked, &btn_2->widget, on_btn_click);
	sig_connect(&btn_3->clicked, &btn_3->widget, on_btn_click);
	sig_connect(&btn_4->clicked, &btn_4->widget, on_btn_click);
	sig_connect(&btn_5->clicked, &btn_5->widget, on_btn_click);
	sig_connect(&btn_6->clicked, &btn_6->widget, on_btn_click);
	sig_connect(&btn_7->clicked, &btn_7->widget, on_btn_click);
	sig_connect(&btn_8->clicked, &btn_8->widget, on_btn_click);
	sig_connect(&btn_9->clicked, &btn_9->widget, on_btn_click);
	sig_connect(&btn_0->clicked, &btn_0->widget, on_btn_click);

	sig_connect(&btn_add->clicked, &btn_add->widget, on_btn_click);
	sig_connect(&btn_sub->clicked, &btn_sub->widget, on_btn_click);
	sig_connect(&btn_div->clicked, &btn_div->widget, on_btn_click);
	sig_connect(&btn_mul->clicked, &btn_mul->widget, on_btn_click);

	sig_connect(&btn_eval->clicked, &btn_eval->widget, on_eval_click);
	sig_connect(&btn_c->clicked, &btn_c->widget, on_c_click);

	grid->add(grid, &display->widget, 0, 0, 4, 1);

	grid->add(grid, &btn_1->widget, 0, 1, 1, 1);
	grid->add(grid, &btn_2->widget, 1, 1, 1, 1);
	grid->add(grid, &btn_3->widget, 2, 1, 1, 1);
	grid->add(grid, &btn_add->widget, 3, 1, 1, 1);

	grid->add(grid, &btn_4->widget, 0, 2, 1, 1);
	grid->add(grid, &btn_5->widget, 1, 2, 1, 1);
	grid->add(grid, &btn_6->widget, 2, 2, 1, 1);
	grid->add(grid, &btn_sub->widget, 3, 2, 1, 1);

	grid->add(grid, &btn_7->widget, 0, 3, 1, 1);
	grid->add(grid, &btn_8->widget, 1, 3, 1, 1);
	grid->add(grid, &btn_9->widget, 2, 3, 1, 1);
	grid->add(grid, &btn_mul->widget, 3, 3, 1, 1);

	grid->add(grid, &btn_c->widget, 0, 4, 1, 1);
	grid->add(grid, &btn_0->widget, 1, 4, 1, 1);
	grid->add(grid, &btn_eval->widget, 2, 4, 1, 1);
	grid->add(grid, &btn_div->widget, 3, 4, 1, 1);

	window_resize(main_window, 0, 0, 400, 400, WINDOW_PLACEMENT_ANY);
	window_exec(main_window);

	task_retval(0);
	async_manager();

	return 0;
}
