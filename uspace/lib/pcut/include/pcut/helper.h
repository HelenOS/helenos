/*
 * Copyright (c) 2014 Vojtech Horky
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
 * Helper macros.
 */
#ifndef PCUT_HELPER_H_GUARD
#define PCUT_HELPER_H_GUARD

/** @cond devel */

/** Join the two arguments on preprocessor level (inner call). */
#define PCUT_JOIN_IMPL(a, b) a##b

/** Join the two arguments on preprocessor level. */
#define PCUT_JOIN(a, b) PCUT_JOIN_IMPL(a, b)

/** Quote the parameter (inner call). */
#define PCUT_QUOTE_IMPL(x) #x

/** Quote the parameter. */
#define PCUT_QUOTE(x) PCUT_QUOTE_IMPL(x)

/** Get first argument from macro var-args. */
#define PCUT_VARG_GET_FIRST(x, ...) x

/** Get all but first arguments from macro var-args. */
#define PCUT_VARG_SKIP_FIRST(x, ...) __VA_ARGS__

#endif
