/*
 * Copyright (c) 2010 Jiri Svoboda
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
