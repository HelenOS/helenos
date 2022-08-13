/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STYPE_T_H_
#define STYPE_T_H_

/** Block visit record
 *
 * One block VR is created for each block that we enter. A variable declaration
 * statement inserts the variable declaration here. Upon leaving the block we
 * pop from the stack, thus all the variable declarations from that block
 * are forgotten.
 */
typedef struct run_block_vr {
	/** Variable declarations in this block */
	intmap_t vdecls; /* of stree_vdecl_t */
} stype_block_vr_t;

/** Procedure visit record
 *
 * A procedure can be a member function or a property getter or setter. A
 * procedure visit record is created whenever @c stype (the static typing
 * pass) enters a procedure.
 */
typedef struct run_proc_vr {
	/** Definition of function or property being invoked */
	struct stree_proc *proc;

	/** Block activation records */
	list_t block_vr; /* of run_block_ar_t */

	/** Number of active breakable statements (for break checking). */
	int bstat_cnt;
} stype_proc_vr_t;

/** Conversion class */
typedef enum {
	/** Implicit conversion */
	convc_implicit,
	/** 'as' conversion */
	convc_as
} stype_conv_class_t;

/** Static typer state object */
typedef struct stype {
	/** Code of the program being typed */
	struct stree_program *program;

	/**
	 * CSI context in which we are currently typing. We keep an implicit
	 * stack of these (in instances of local variable
	 * @c stype_csi::prev_ctx.)
	 */
	struct stree_csi *current_csi;

	/** Procedure VR for the current procedure. */
	stype_proc_vr_t *proc_vr;

	/** @c b_true if an error occured. */
	bool_t error;
} stype_t;

#endif
