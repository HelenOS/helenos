/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip PWM timer driver.
 */

#ifndef KERN_S3C24XX_TIMER_H_
#define KERN_S3C24XX_TIMER_H_

#include <typedefs.h>

/** Physical address where S3C24XX on-chip PWM timer is mapped */
#define S3C24XX_TIMER_ADDRESS	0x51000000

/** S3C24xx on-chip PWM timer registers */
typedef struct {
	ioport32_t tcfg0;	/**< Timer configuration register 0 */
	ioport32_t tcfg1;	/**< Timer configuration register 1 */
	ioport32_t tcon;	/**< Timer control register */

	struct {
		ioport32_t cntb;	/**< Count buffer register */
		ioport32_t cmpb;	/**< Compare buffer register */
		ioport32_t cnto;	/**< Count observation register */
	} timer[5];
} s3c24xx_timer_t;

/** Bits in the S3C24xx PWM timer TCON register. */
enum s3c24xx_tcon_bits {
	TCON_T0_START		= (1 << 0),	/**< Timer 0 start */
	TCON_T0_MUPDATE		= (1 << 1),	/**< Timer 0 manual update */
	TCON_T0_INVERT		= (1 << 2),	/**< Timer 0 inverter on */
	TCON_T0_AUTO_RLD	= (1 << 3),	/**< Timer 0 auto reload */

	TCON_DEAD_ZONE		= (1 << 4),	/**< Dead zone enable */

	TCON_T1_START		= (1 << 8),	/**< Timer 1 start */
	TCON_T1_MUPDATE		= (1 << 9),	/**< Timer 1 manual update */
	TCON_T1_INVERT		= (1 << 10),	/**< Timer 1 inverter on */
	TCON_T1_AUTO_RLD	= (1 << 11),	/**< Timer 1 auto reload */

	TCON_T2_START		= (1 << 12),	/**< Timer 2 start */
	TCON_T2_MUPDATE		= (1 << 13),	/**< Timer 2 manual update */
	TCON_T2_INVERT		= (1 << 14),	/**< Timer 2 inverter on */
	TCON_T2_AUTO_RLD	= (1 << 15),	/**< Timer 2 auto reload */

	TCON_T3_START		= (1 << 16),	/**< Timer 3 start */
	TCON_T3_MUPDATE		= (1 << 17),	/**< Timer 3 manual update */
	TCON_T3_INVERT		= (1 << 18),	/**< Timer 3 inverter on */
	TCON_T3_AUTO_RLD	= (1 << 19),	/**< Timer 3 auto reload */

	TCON_T4_START		= (1 << 20),	/**< Timer 4 start */
	TCON_T4_MUPDATE		= (1 << 21),	/**< Timer 4 manual update */
	TCON_T4_AUTO_RLD	= (1 << 22)	/**< Timer 4 auto reload */
};

#endif

/** @}
 */
