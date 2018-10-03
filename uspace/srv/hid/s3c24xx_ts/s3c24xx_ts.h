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

/** @addtogroup uspace_srv_s3c24xx_ts
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip ADC and touch-screen interface driver.
 */

#ifndef S3C24XX_TS_H_
#define S3C24XX_TS_H_

#include <stdint.h>
#include <async.h>

/** S3C24xx ADC and touch-screen I/O */
typedef struct {
	uint32_t con;
	uint32_t tsc;
	uint32_t dly;
	uint32_t dat0;
	uint32_t dat1;
	uint32_t updn;
} s3c24xx_adc_io_t;

/* Fields in ADCCON register */
#define ADCCON_ECFLG		0x8000
#define ADCCON_PRSCEN		0x4000

#define ADCCON_PRSCVL(val)	(((val) & 0xff) << 6)

#define ADCCON_SEL_MUX(smux)	(((smux) & 7) << 3)

#define ADCCON_STDBM		0x0004
#define ADCCON_READ_START	0x0002
#define ADCCON_ENABLE_START	0x0001

/* Values for ADCCON_SEL_MUX */
#define SMUX_AIN0		0
#define SMUX_AIN1		1
#define SMUX_AIN2		2
#define SMUX_AIN3		3
#define SMUX_YM			4
#define SMUX_YP			5
#define SMUX_XM			6
#define SMUX_XP			7

/* Fields in ADCTSC register */
#define ADCTSC_DSUD_UP		0x0100
#define ADCTSC_YM_ENABLE	0x0080
#define ADCTSC_YP_DISABLE	0x0040
#define ADCTSC_XM_ENABLE	0x0020
#define ADCTSC_XP_DISABLE	0x0010
#define ADCTSC_PULLUP_DISABLE	0x0008
#define ADCTSC_AUTO_PST		0x0004

#define ADCTSC_XY_PST_NOOP	0x0000
#define ADCTSC_XY_PST_X		0x0001
#define ADCTSC_XY_PST_Y		0x0002
#define ADCTSC_XY_PST_WAITINT	0x0003
#define ADCTSC_XY_PST_MASK	0x0003

/* Fields in ADCDAT0, ADCDAT1 registers */
#define ADCDAT_UPDOWN		0x8000
#define ADCDAT_AUTO_PST		0x4000

/* Fields in ADCUPDN register */
#define ADCUPDN_TSC_UP		0x0002
#define ADCUPDN_TSC_DN		0x0001

/** Touchscreen interrupt number */
#define S3C24XX_TS_INR		31

/** Touchscreen I/O address */
#define S3C24XX_TS_ADDR		0x58000000

typedef enum {
	ts_wait_pendown,
	ts_sample_pos,
	ts_wait_penup
} ts_state_t;

typedef enum {
	updn_up,
	updn_down
} ts_updn_t;

/** S3C24xx touchscreen driver instance */
typedef struct {
	/** Physical device address */
	uintptr_t paddr;

	/** Device I/O structure */
	s3c24xx_adc_io_t *io;

	/** Callback session to the client */
	async_sess_t *client_sess;

	/** Service ID */
	service_id_t service_id;

	/** Device/driver state */
	ts_state_t state;

	/** Previous position reported to client. */
	int last_x;
	int last_y;
} s3c24xx_ts_t;

#endif

/** @}
 */
