/*
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 * @brief ARM Generic Interrupt Controller v3 TFA adaptation layer, 64-bit.
 */

#ifndef KERN_TFA_ADAPT_H_
#define KERN_TFA_ADAPT_H_

#include <arch/asm.h>
#include <arch/regutils.h>
#include <assert.h>
#include <panic.h>

# define  U_(_x)	(_x##U)
# define  U(_x)	    U_(_x)
# define  UL_(_x)	(_x##UL)
# define  UL(_x)	UL_(_x)
# define  ULL_(_x)	(_x##ULL)
# define  ULL(_x)	ULL_(_x)
# define  L_(_x)	(_x##L)
# define  L(_x)	    L_(_x)
# define  LL_(_x)	(_x##LL)
# define  LL(_x)	LL_(_x)

#define BIT_32(nr)			(U(1) << (nr))
#define BIT_64(nr)			(ULL(1) << (nr))

#define dsb()			dsbsy()

typedef unsigned long u_register_t;

SPECIAL_REG_GEN_WRITE(ICC_SRE_EL1);
SPECIAL_REG_GEN_WRITE(ICC_PMR_EL1);
SPECIAL_REG_GEN_WRITE(ICC_EOIR1_EL1);
SPECIAL_REG_GEN_WRITE(ICC_IGRPEN1_EL1);
SPECIAL_REG_GEN_READ(ICC_IAR1_EL1);
SPECIAL_REG_GEN_WRITE(ICC_SGI1R_EL1);

static inline void write_icc_sre_el1(uint64_t val)
{
	ICC_SRE_EL1_write(val);
}

static inline void write_icc_pmr_el1(uint64_t val)
{
	ICC_PMR_EL1_write(val);
}

static inline void write_icc_eoir1_el1(uint64_t val)
{
	ICC_EOIR1_EL1_write(val);
}

static inline void write_icc_igrpen1_el1(uint64_t val)
{
	ICC_IGRPEN1_EL1_write(val);
}

static inline uint64_t read_icc_iar1_el1(void)
{
	return ICC_IAR1_EL1_read();
}

static inline void write_icc_sgi1r(uint64_t val)
{
	ICC_SGI1R_EL1_write(val);
}

static inline void dsbishst(void)
{
	asm volatile ("dsb ishst" ::: "memory");
}
static inline void dsbsy(void)
{
	asm volatile ("dsb sy" ::: "memory");
}
static inline void isb(void)
{
	asm volatile ("isb" ::: "memory");
}

static inline void mmio_write_32(uintptr_t addr, uint32_t value)
{
	pio_write_32((ioport32_t *)addr, value);
}

static inline uint32_t mmio_read_32(uintptr_t addr)
{
	return pio_read_32((ioport32_t *)addr);
}

static inline void mmio_write_64(uintptr_t addr, uint64_t value)
{
	pio_write_64((ioport64_t *)addr, value);
}

static inline uint64_t mmio_read_64(uintptr_t addr)
{
	return pio_read_64((ioport64_t *)addr);
}

#endif /* KERN_TFA_ADAPT_H_ */

/** @}
 */
