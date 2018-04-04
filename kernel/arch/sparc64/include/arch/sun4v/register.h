/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_REGISTER_H_
#define KERN_sparc64_sun4v_REGISTER_H_

#include <arch/regdef.h>
#include <stdint.h>

/** Processor State Register. */
union pstate_reg {
	uint64_t value;
	struct {
		uint64_t : 54;
		unsigned cle : 1;	/**< Current Little Endian. */
		unsigned tle : 1;	/**< Trap Little Endian. */
		unsigned mm : 2;	/**< Memory Model. */
		unsigned : 1;		/**< RED state. */
		unsigned pef : 1;	/**< Enable floating-point. */
		unsigned am : 1;	/**< 32-bit Address Mask. */
		unsigned priv : 1;	/**< Privileged Mode. */
		unsigned ie : 1;	/**< Interrupt Enable. */
		unsigned : 1;
	} __attribute__((packed));
};
typedef union pstate_reg pstate_reg_t;

#endif

/** @}
 */
