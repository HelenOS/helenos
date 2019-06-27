// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2016, Linaro Limited
 */

/* Modified for HelenOS use by Jiří Zárevúcky. */

#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <mem.h>
#include <io/kio.h>

#define PRINTF(...) kio_printf(__VA_ARGS__)
#define ubsan_panic() abort()

struct source_location {
	const char *file_name;
	uint32_t line;
	uint32_t column;
};

struct type_descriptor {
	uint16_t type_kind;
	uint16_t type_info;
	char type_name[];
};

struct type_mismatch_data {
	struct source_location loc;
	struct type_descriptor *type;
	unsigned long alignment;
	unsigned char type_check_kind;
};

struct type_mismatch_data_v1 {
	struct source_location loc;
	struct type_descriptor *type;
	unsigned char log_alignment;
	unsigned char type_check_kind;
};

struct overflow_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct shift_out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *lhs_type;
	struct type_descriptor *rhs_type;
};

struct out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *array_type;
	struct type_descriptor *index_type;
};

struct unreachable_data {
	struct source_location loc;
};

struct vla_bound_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct invalid_value_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct nonnull_arg_data {
	struct source_location loc;
};

struct nonnull_return_data {
	struct source_location loc;
	struct source_location attr_loc;
};

struct pointer_overflow_data {
	struct source_location loc;
};

/*
 * When compiling with -fsanitize=undefined the compiler expects functions
 * with the following signatures. The functions are never called directly,
 * only when undefined behavior is detected in instrumented code.
 */
void __ubsan_handle_type_mismatch(struct type_mismatch_data *data, unsigned long ptr);
void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *data, unsigned long ptr);
void __ubsan_handle_add_overflow(struct overflow_data *data, unsigned long lhs, unsigned long rhs);
void __ubsan_handle_sub_overflow(struct overflow_data *data, unsigned long lhs, unsigned long rhs);
void __ubsan_handle_mul_overflow(struct overflow_data *data, unsigned long lhs, unsigned long rhs);
void __ubsan_handle_negate_overflow(struct overflow_data *data, unsigned long old_val);
void __ubsan_handle_divrem_overflow(struct overflow_data *data, unsigned long lhs, unsigned long rhs);
void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *data, unsigned long lhs, unsigned long rhs);
void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *data, unsigned long idx);
void __ubsan_handle_unreachable(struct unreachable_data *data);
void __ubsan_handle_missing_return(struct unreachable_data *data);
void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data, unsigned long bound);
void __ubsan_handle_load_invalid_value(struct invalid_value_data *data, unsigned long val);
#if __GCC_VERSION < 60000
void __ubsan_handle_nonnull_arg(struct nonnull_arg_data *data, size_t arg_no);
#else
void __ubsan_handle_nonnull_arg(struct nonnull_arg_data *data);
#endif
void __ubsan_handle_nonnull_return(struct nonnull_return_data *data);
void __ubsan_handle_builtin_unreachable(struct unreachable_data *data);
void __ubsan_handle_pointer_overflow(struct pointer_overflow_data *data,
    unsigned long base, unsigned long result);

static void print_loc(const char *func, struct source_location *loc)
{
	const char *f = func;
	const char func_prefix[] = "__ubsan_handle";

	if (!memcmp(f, func_prefix, sizeof(func_prefix) - 1))
		f += sizeof(func_prefix);

	PRINTF("####### Undefined behavior %s at %s:%" PRIu32 " col %" PRIu32 "\n",
	    f, loc->file_name, loc->line, loc->column);
}

void __ubsan_handle_type_mismatch(struct type_mismatch_data *data,
    unsigned long ptr)
{
	print_loc(__func__, &data->loc);
	PRINTF("Type: %s, alignment: %lu, type_check_kind: %hhu\n",
	    data->type->type_name, data->alignment, data->type_check_kind);
	ubsan_panic();
}

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *data,
    unsigned long ptr)
{
	print_loc(__func__, &data->loc);
	PRINTF("Type: %s, alignment: %hhu, type_check_kind: %hhu\n",
	    data->type->type_name, data->log_alignment, data->type_check_kind);
	ubsan_panic();
}

void __ubsan_handle_add_overflow(struct overflow_data *data,
    unsigned long lhs,
    unsigned long rhs)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_sub_overflow(struct overflow_data *data,
    unsigned long lhs,
    unsigned long rhs)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_mul_overflow(struct overflow_data *data,
    unsigned long lhs,
    unsigned long rhs)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_negate_overflow(struct overflow_data *data,
    unsigned long old_val)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_divrem_overflow(struct overflow_data *data,
    unsigned long lhs,
    unsigned long rhs)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *data,
    unsigned long lhs,
    unsigned long rhs)
{
	print_loc(__func__, &data->loc);
	PRINTF("LHS type: %s, value: %lu, RHS type: %s, value: %lu\n",
	    data->lhs_type->type_name, lhs, data->rhs_type->type_name, rhs);
	ubsan_panic();
}

void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *data,
    unsigned long idx)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_unreachable(struct unreachable_data *data)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_missing_return(struct unreachable_data *data)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data,
    unsigned long bound)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_load_invalid_value(struct invalid_value_data *data,
    unsigned long val)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

#if __GCC_VERSION < 60000
void __ubsan_handle_nonnull_arg(struct nonnull_arg_data *data, size_t arg_no)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}
#else
void __ubsan_handle_nonnull_arg(struct nonnull_arg_data *data)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}
#endif

void __ubsan_handle_nonnull_return(struct nonnull_return_data *data)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_builtin_unreachable(struct unreachable_data *data)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}

void __ubsan_handle_pointer_overflow(struct pointer_overflow_data *data,
    unsigned long base, unsigned long result)
{
	print_loc(__func__, &data->loc);
	ubsan_panic();
}
