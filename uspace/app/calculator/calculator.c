/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup calculator
 * @{
 */
/** @file
 *
 * Inspired by the code released at https://github.com/osgroup/HelenOSProject
 *
 */

#include <clipboard.h>
#include <ctype.h>
#include <io/kbd_event.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/entry.h>
#include <ui/fixed.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <ui/pbutton.h>
#include <ui/ui.h>
#include <ui/window.h>

#define NAME  "calculator"

#define NULL_DISPLAY  "0"

#define SYNTAX_ERROR_DISPLAY   "Syntax error"
#define NUMERIC_ERROR_DISPLAY  "Numerical error"
#define UNKNOWN_ERROR_DISPLAY  "Unknown error"

#define EXPR_MAX_LEN 22

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

/** Dimensions. Most of this should not be needed with auto layout */
typedef struct {
	gfx_rect_t menubar_rect;
	gfx_rect_t entry_rect;
	gfx_coord2_t btn_orig;
	gfx_coord2_t btn_stride;
	gfx_coord2_t btn_dim;
} calc_geom_t;

typedef struct {
	ui_t *ui;
	ui_resource_t *ui_res;
	ui_pbutton_t *btn_eval;
	ui_pbutton_t *btn_clear;
	ui_pbutton_t *btn_add;
	ui_pbutton_t *btn_sub;
	ui_pbutton_t *btn_mul;
	ui_pbutton_t *btn_div;
	ui_pbutton_t *btn_0;
	ui_pbutton_t *btn_1;
	ui_pbutton_t *btn_2;
	ui_pbutton_t *btn_3;
	ui_pbutton_t *btn_4;
	ui_pbutton_t *btn_5;
	ui_pbutton_t *btn_6;
	ui_pbutton_t *btn_7;
	ui_pbutton_t *btn_8;
	ui_pbutton_t *btn_9;
	ui_menu_bar_t *menubar;
	calc_geom_t geom;
} calc_t;

static void display_update(void);

static void calc_file_exit(ui_menu_entry_t *, void *);
static void calc_edit_copy(ui_menu_entry_t *, void *);
static void calc_edit_paste(ui_menu_entry_t *, void *);

static void calc_pb_clicked(ui_pbutton_t *, void *);
static void calc_eval_clicked(ui_pbutton_t *, void *);
static void calc_clear_clicked(ui_pbutton_t *, void *);

static ui_pbutton_cb_t calc_pbutton_cb = {
	.clicked = calc_pb_clicked
};

static ui_pbutton_cb_t calc_clear_cb = {
	.clicked = calc_clear_clicked
};

static ui_pbutton_cb_t calc_eval_cb = {
	.clicked = calc_eval_clicked
};

static void wnd_close(ui_window_t *, void *);
static void wnd_kbd_event(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.kbd = wnd_kbd_event
};

static char *expr = NULL;
static ui_entry_t *display;

/** Window close request
 *
 * @param window Window
 * @param arg Argument (calc_t *)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	calc_t *calc = (calc_t *) arg;

	ui_quit(calc->ui);
}

/** Window keyboard event
 *
 * @param window Window
 * @param arg Argument (calc_t *)
 * @param event Keyboard event
 */
static void wnd_kbd_event(ui_window_t *window, void *arg, kbd_event_t *event)
{
	calc_t *calc = (calc_t *) arg;

	if (ui_window_def_kbd(window, event) == ui_claimed)
		return;

	if (event->type == KEY_PRESS && (event->mods & KM_CTRL) != 0) {
		switch (event->key) {
		case KC_C:
			calc_edit_copy(NULL, calc);
			break;
		case KC_V:
			calc_edit_paste(NULL, calc);
			break;
		default:
			break;
		}
	}

	switch (event->key) {
	case KC_ENTER:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_eval);
		else
			ui_pbutton_release(calc->btn_eval);
		break;
	case KC_BACKSPACE:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_clear);
		else
			ui_pbutton_release(calc->btn_clear);
		break;
	case KC_MINUS:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_sub);
		else
			ui_pbutton_release(calc->btn_sub);
		break;
	case KC_EQUALS:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_add);
		else
			ui_pbutton_release(calc->btn_add);
		break;
	case KC_SLASH:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_div);
		else
			ui_pbutton_release(calc->btn_div);
		break;
	case KC_0:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_0);
		else
			ui_pbutton_release(calc->btn_0);
		break;
	case KC_1:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_1);
		else
			ui_pbutton_release(calc->btn_1);
		break;
	case KC_2:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_2);
		else
			ui_pbutton_release(calc->btn_2);
		break;
	case KC_3:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_3);
		else
			ui_pbutton_release(calc->btn_3);
		break;
	case KC_4:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_4);
		else
			ui_pbutton_release(calc->btn_4);
		break;
	case KC_5:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_5);
		else
			ui_pbutton_release(calc->btn_5);
		break;
	case KC_6:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_6);
		else
			ui_pbutton_release(calc->btn_6);
		break;
	case KC_7:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_7);
		else
			ui_pbutton_release(calc->btn_7);
		break;
	case KC_8:
		if ((event->mods & KM_SHIFT) != 0) {
			if (event->type == KEY_PRESS)
				ui_pbutton_press(calc->btn_mul);
			else
				ui_pbutton_release(calc->btn_mul);
		} else {
			if (event->type == KEY_PRESS)
				ui_pbutton_press(calc->btn_8);
			else
				ui_pbutton_release(calc->btn_8);
		}
		break;
	case KC_9:
		if (event->type == KEY_PRESS)
			ui_pbutton_press(calc->btn_9);
		else
			ui_pbutton_release(calc->btn_9);
		break;
	default:
		break;
	}
}

/** File / Exit menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (calc_t *)
 */
static void calc_file_exit(ui_menu_entry_t *mentry, void *arg)
{
	calc_t *calc = (calc_t *) arg;

	ui_quit(calc->ui);
}

/** Edit / Copy menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (calc_t *)
 */
static void calc_edit_copy(ui_menu_entry_t *mentry, void *arg)
{
	const char *str;

	(void) arg;
	str = (expr != NULL) ? expr : NULL_DISPLAY;

	(void) clipboard_put_str(str);
}

/** Edit / Paste menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (calc_t *)
 */
static void calc_edit_paste(ui_menu_entry_t *mentry, void *arg)
{
	char *str;
	char *old;
	char *cp;
	errno_t rc;

	(void) arg;

	rc = clipboard_get_str(&str);
	if (rc != EOK)
		return;

	/* Make sure string only contains allowed characters */
	cp = str;
	while (*cp != '\0') {
		if (!isdigit(*cp) && *cp != '+' && *cp != '-' &&
		    *cp != '*' && *cp != '/')
			return;
		++cp;
	}

	/* Update expression */
	old = expr;
	expr = str;
	free(old);

	display_update();
}

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
		(void) ui_entry_set_text(display, (void *) expr);
	else
		(void) ui_entry_set_text(display, (void *) NULL_DISPLAY);

	ui_entry_paint(display);
}

static void display_error(error_type_t error_type)
{
	if (expr != NULL) {
		free(expr);
		expr = NULL;
	}

	switch (error_type) {
	case ERROR_SYNTAX:
		(void) ui_entry_set_text(display,
		    (void *) SYNTAX_ERROR_DISPLAY);
		break;
	case ERROR_NUMERIC:
		(void) ui_entry_set_text(display,
		    (void *) NUMERIC_ERROR_DISPLAY);
		break;
	default:
		(void) ui_entry_set_text(display,
		    (void *) UNKNOWN_ERROR_DISPLAY);
		break;
	}

	ui_entry_paint(display);
}

static void calc_pb_clicked(ui_pbutton_t *pbutton, void *arg)
{
	const char *subexpr = (const char *) arg;

	if (expr != NULL) {
		char *new_expr;

		if (str_length(expr) < EXPR_MAX_LEN) {
			asprintf(&new_expr, "%s%s", expr, subexpr);
			free(expr);
			expr = new_expr;
		}
	} else {
		expr = str_dup(subexpr);
	}

	display_update();
}

static void calc_clear_clicked(ui_pbutton_t *pbutton, void *arg)
{
	if (expr != NULL) {
		free(expr);
		expr = NULL;
	}

	display_update();
}

static void calc_eval_clicked(ui_pbutton_t *pbutton, void *arg)
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

static errno_t calc_button_create(calc_t *calc, ui_fixed_t *fixed,
    int x, int y, const char *text, ui_pbutton_cb_t *cb, void *arg,
    ui_pbutton_t **rbutton)
{
	ui_pbutton_t *pb;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_pbutton_create(calc->ui_res, text, &pb);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(pb, cb, arg);

	rect.p0.x = calc->geom.btn_orig.x + calc->geom.btn_stride.x * x;
	rect.p0.y = calc->geom.btn_orig.y + calc->geom.btn_stride.y * y;
	rect.p1.x = rect.p0.x + calc->geom.btn_dim.x;
	rect.p1.y = rect.p0.y + calc->geom.btn_dim.y;
	ui_pbutton_set_rect(pb, &rect);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(pb));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	if (rbutton != NULL)
		*rbutton = pb;
	return EOK;
}

static void print_syntax(void)
{
	printf("Syntax: %s [-d <display-spec>]\n", NAME);
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_ANY_DEFAULT;
	ui_t *ui;
	ui_resource_t *ui_res;
	ui_fixed_t *fixed;
	ui_wnd_params_t params;
	ui_window_t *window;
	ui_menu_t *mfile;
	ui_menu_entry_t *mexit;
	ui_menu_t *medit;
	ui_menu_entry_t *mcopy;
	ui_menu_entry_t *mpaste;
	calc_t calc;
	errno_t rc;
	int i;

	i = 1;
	while (i < argc) {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return rc;
	}

	ui_wnd_params_init(&params);
	params.caption = "Calculator";
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;

	if (ui_is_textmode(ui)) {
		params.rect.p1.x = 38;
		params.rect.p1.y = 18;

		calc.geom.menubar_rect.p0.x = 1;
		calc.geom.menubar_rect.p0.y = 1;
		calc.geom.menubar_rect.p1.x = params.rect.p1.x - 1;
		calc.geom.menubar_rect.p1.y = 2;
		calc.geom.entry_rect.p0.x = 4;
		calc.geom.entry_rect.p0.y = 3;
		calc.geom.entry_rect.p1.x = 34;
		calc.geom.entry_rect.p1.y = 4;
		calc.geom.btn_orig.x = 4;
		calc.geom.btn_orig.y = 5;
		calc.geom.btn_dim.x = 6;
		calc.geom.btn_dim.y = 2;
		calc.geom.btn_stride.x = 8;
		calc.geom.btn_stride.y = 3;
	} else {
		params.rect.p1.x = 250;
		params.rect.p1.y = 270;

		calc.geom.menubar_rect.p0.x = 4;
		calc.geom.menubar_rect.p0.y = 30;
		calc.geom.menubar_rect.p1.x = params.rect.p1.x - 4;
		calc.geom.menubar_rect.p1.y = 52;
		calc.geom.entry_rect.p0.x = 10;
		calc.geom.entry_rect.p0.y = 51;
		calc.geom.entry_rect.p1.x = 240;
		calc.geom.entry_rect.p1.y = 76;
		calc.geom.btn_orig.x = 10;
		calc.geom.btn_orig.y = 90;
		calc.geom.btn_dim.x = 50;
		calc.geom.btn_dim.y = 35;
		calc.geom.btn_stride.x = 60;
		calc.geom.btn_stride.y = 45;
	}

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	ui_window_set_cb(window, &window_cb, (void *) &calc);
	calc.ui = ui;

	ui_res = ui_window_get_res(window);
	calc.ui_res = ui_res;

	rc = ui_fixed_create(&fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_menu_bar_create(ui, window, &calc.menubar);
	if (rc != EOK) {
		printf("Error creating menu bar.\n");
		return rc;
	}

	rc = ui_menu_dd_create(calc.menubar, "~F~ile", NULL, &mfile);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(mfile, "E~x~it", "Alt-F4", &mexit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mexit, calc_file_exit, (void *) &calc);

	rc = ui_menu_dd_create(calc.menubar, "~E~dit", NULL, &medit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(medit, "~C~opy", "Ctrl-C", &mcopy);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mcopy, calc_edit_copy, (void *) &calc);

	rc = ui_menu_entry_create(medit, "~P~aste", "Ctrl-V", &mpaste);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mpaste, calc_edit_paste, (void *) &calc);

	ui_menu_bar_set_rect(calc.menubar, &calc.geom.menubar_rect);

	rc = ui_fixed_add(fixed, ui_menu_bar_ctl(calc.menubar));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_entry_create(window, NULL_DISPLAY, &display);
	if (rc != EOK) {
		printf("Error creating text lentry.\n");
		return rc;
	}

	ui_entry_set_rect(display, &calc.geom.entry_rect);
	ui_entry_set_halign(display, gfx_halign_right);
	ui_entry_set_read_only(display, true);

	rc = ui_fixed_add(fixed, ui_entry_ctl(display));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = calc_button_create(&calc, fixed, 0, 0, "7", &calc_pbutton_cb,
	    (void *) "7", &calc.btn_7);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 1, 0, "8", &calc_pbutton_cb,
	    (void *) "8", &calc.btn_8);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 2, 0, "9", &calc_pbutton_cb,
	    (void *) "9", &calc.btn_9);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 3, 0, "/", &calc_pbutton_cb,
	    (void *) "/", &calc.btn_div);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 0, 1, "4", &calc_pbutton_cb,
	    (void *) "4", &calc.btn_4);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 1, 1, "5", &calc_pbutton_cb,
	    (void *) "5", &calc.btn_5);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 2, 1, "6", &calc_pbutton_cb,
	    (void *) "6", &calc.btn_6);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 3, 1, "*", &calc_pbutton_cb,
	    (void *) "*", &calc.btn_mul);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 0, 2, "1", &calc_pbutton_cb,
	    (void *) "1", &calc.btn_1);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 1, 2, "2", &calc_pbutton_cb,
	    (void *) "2", &calc.btn_2);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 2, 2, "3", &calc_pbutton_cb,
	    (void *) "3", &calc.btn_3);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 3, 2, "-", &calc_pbutton_cb,
	    (void *) "-", &calc.btn_sub);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 0, 3, "0", &calc_pbutton_cb,
	    (void *) "0", &calc.btn_0);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 1, 3, "C", &calc_clear_cb,
	    (void *) "C", &calc.btn_clear);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 2, 3, "=", &calc_eval_cb,
	    (void *) "=", &calc.btn_eval);
	if (rc != EOK)
		return rc;

	rc = calc_button_create(&calc, fixed, 3, 3, "+", &calc_pbutton_cb,
	    (void *) "+", &calc.btn_add);
	if (rc != EOK)
		return rc;

	ui_pbutton_set_default(calc.btn_eval, true);

	ui_window_add(window, ui_fixed_ctl(fixed));

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}

	ui_run(ui);
	ui_window_destroy(window);
	ui_destroy(ui);

	return 0;
}
