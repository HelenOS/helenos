/*
 * Copyright (c) 2012 Matteo Facchinetti
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
/**
 * @file
 * @brief Texas Instruments AM335x CONTROL_MODULE: I/O multiplexing.
 */

#ifndef KERN_AM335X_IOMUX_H_
#define KERN_AM335X_IOMUX_H_

#include <typedefs.h>

/* Pad Control Register for each configurable pin p. 876 */
#define AM335X_IOMUX_SLEWRATE_SLOW_FLAG  (1 << 6)
#define AM335X_IOMUX_RX_ENABLE_FLAG  (1 << 5)
#define AM335X_IOMUX_PULLUP_FLAG  (1 << 4)
#define AM335X_IOMUX_PULLUPDOWN_ENABLE_FLAG  (1 << 3)
#define AM335X_IOMUX_MODE0  0
#define AM335X_IOMUX_MODE1  1
#define AM335X_IOMUX_MODE2  2
#define AM335X_IOMUX_MODE3  3
#define AM335X_IOMUX_MODE4  4
#define AM335X_IOMUX_MODE5  5
#define AM335X_IOMUX_MODE6  6
#define AM335X_IOMUX_MODE7  7

/* AM335X CONTROL_MODULE configurable I/O pin. Table 9-10 at p. 886 */
#define AM335X_IOMUX_CONF_BASE_ADDRESS 0x44E10800

typedef struct {
	ioport32_t gpmc_ad1;
	ioport32_t gpmc_ad2;
	ioport32_t gpmc_ad3;
	ioport32_t gpmc_ad4;
	ioport32_t gpmc_ad5;
	ioport32_t gpmc_ad6;
	ioport32_t gpmc_ad7;
	ioport32_t gpmc_ad8;
	ioport32_t gpmc_ad9;
	ioport32_t gpmc_ad10;
	ioport32_t gpmc_ad11;
	ioport32_t gpmc_ad12;
	ioport32_t gpmc_ad13;
	ioport32_t gpmc_ad14;
	ioport32_t gpmc_ad15;
	ioport32_t gpmc_a0;
	ioport32_t gpmc_a1;
	ioport32_t gpmc_a2;
	ioport32_t gpmc_a3;
	ioport32_t gpmc_a4;
	ioport32_t gpmc_a5;
	ioport32_t gpmc_a6;
	ioport32_t gpmc_a7;
	ioport32_t gpmc_a8;
	ioport32_t gpmc_a9;
	ioport32_t gpmc_a10;
	ioport32_t gpmc_a11;
	ioport32_t gpmc_wait0;
	ioport32_t gpmc_wpn;
	ioport32_t gpmc_be1n;
	ioport32_t gpmc_csn0;
	ioport32_t gpmc_csn1;
	ioport32_t gpmc_csn2;
	ioport32_t gpmc_csn3;
	ioport32_t gpmc_clk;
	ioport32_t gpmc_advn_ale;
	ioport32_t gpmc_oen_ren;
	ioport32_t gpmc_wen;
	ioport32_t gpmc_be0n_cle;
	ioport32_t lcd_data0;
	ioport32_t lcd_data1;
	ioport32_t lcd_data2;
	ioport32_t lcd_data3;
	ioport32_t lcd_data4;
	ioport32_t lcd_data5;
	ioport32_t lcd_data6;
	ioport32_t lcd_data7;
	ioport32_t lcd_data8;
	ioport32_t lcd_data9;
	ioport32_t lcd_data10;
	ioport32_t lcd_data11;
	ioport32_t lcd_data12;
	ioport32_t lcd_data13;
	ioport32_t lcd_data14;
	ioport32_t lcd_data15;
	ioport32_t lcd_vsync;
	ioport32_t lcd_hsync;
	ioport32_t lcd_pclk;
	ioport32_t lcd_ac_bias_en;
	ioport32_t mmc0_dat3;
	ioport32_t mmc0_dat2;
	ioport32_t mmc0_dat1;
	ioport32_t mmc0_dat0;
	ioport32_t mmc0_clk;
	ioport32_t mmc0_cmd;
	ioport32_t mii1_col;
	ioport32_t mii1_crs;
	ioport32_t mii1_rxerr;
	ioport32_t mii1_txen;
	ioport32_t mii1_rxdv;
	ioport32_t mii1_txd3;
	ioport32_t mii1_txd2;
	ioport32_t mii1_txd1;
	ioport32_t mii1_txd0;
	ioport32_t mii1_txclk;
	ioport32_t mii1_rxclk;
	ioport32_t mii1_rxd3;
	ioport32_t mii1_rxd2;
	ioport32_t mii1_rxd1;
	ioport32_t mii1_rxd0;
	ioport32_t rmii1_refclk;
	ioport32_t mdio_data;
	ioport32_t mdio_clk;
	ioport32_t spi0_sclk;
	ioport32_t spi0_d0;
	ioport32_t spi0_d1;
	ioport32_t spi0_cs0;
	ioport32_t spi0_cs1;
	ioport32_t ecap0_in_pwm0_out;
	ioport32_t uart0_ctsn;
	ioport32_t uart0_rtsn;
	ioport32_t uart0_rxd;
	ioport32_t uart0_txd;
	ioport32_t uart1_ctsn;
	ioport32_t uart1_rtsn;
	ioport32_t uart1_rxd;
	ioport32_t uart1_txd;
	ioport32_t i2c0_sda;
	ioport32_t i2c0_scl;
	ioport32_t mcasp0_aclkx;
	ioport32_t mcasp0_fsx;
	ioport32_t mcasp0_axr0;
	ioport32_t mcasp0_ahclkr;
	ioport32_t mcasp0_aclkr;
	ioport32_t mcasp0_fsr;
	ioport32_t mcasp0_axr1;
	ioport32_t mcasp0_ahclkx;
	ioport32_t xdma_event_intr0;
	ioport32_t xdma_event_intr1;
	ioport32_t nresetin_out;
	ioport32_t porz;
	ioport32_t nnmi;
	ioport32_t osc0_in;
	ioport32_t osc0_out;
	ioport32_t osc0_vss;
	ioport32_t tms;
	ioport32_t tdi;
	ioport32_t tdo;
	ioport32_t tck;
	ioport32_t ntrst;
	ioport32_t emu0;
	ioport32_t emu1;
	ioport32_t osc1_in;
	ioport32_t osc1_out;
	ioport32_t osc1_vss;
	ioport32_t rtc_porz;
	ioport32_t pmic_power_en;
	ioport32_t ext_wakeup;
	ioport32_t enz_kaldo_1p8v;
	ioport32_t usb0_dm;
	ioport32_t usb0_dp;
	ioport32_t usb0_ce;
	ioport32_t usb0_id;
	ioport32_t usb0_vbus;
	ioport32_t usb0_drvvbus;
	ioport32_t usb1_dm;
	ioport32_t usb1_dp;
	ioport32_t usb1_ce;
	ioport32_t usb1_id;
	ioport32_t usb1_vbus;
	ioport32_t usb1_drvvbus;
	ioport32_t ddr_resetn;
	ioport32_t ddr_csn0;
	ioport32_t ddr_cke;
	ioport32_t ddr_ck;
	ioport32_t ddr_nck;
	ioport32_t ddr_casn;
	ioport32_t ddr_rasn;
	ioport32_t ddr_wen;
	ioport32_t ddr_ba0;
	ioport32_t ddr_ba1;
	ioport32_t ddr_ba2;
	ioport32_t ddr_a0;
	ioport32_t ddr_a1;
	ioport32_t ddr_a2;
	ioport32_t ddr_a3;
	ioport32_t ddr_a4;
	ioport32_t ddr_a5;
	ioport32_t ddr_a6;
	ioport32_t ddr_a7;
	ioport32_t ddr_a8;
	ioport32_t ddr_a9;
	ioport32_t ddr_a10;
	ioport32_t ddr_a11;
	ioport32_t ddr_a12;
	ioport32_t ddr_a13;
	ioport32_t ddr_a14;
	ioport32_t ddr_a15;
	ioport32_t ddr_odt;
	ioport32_t ddr_d0;
	ioport32_t ddr_d1;
	ioport32_t ddr_d2;
	ioport32_t ddr_d3;
	ioport32_t ddr_d4;
	ioport32_t ddr_d5;
	ioport32_t ddr_d6;
	ioport32_t ddr_d7;
	ioport32_t ddr_d8;
	ioport32_t ddr_d9;
	ioport32_t ddr_d10;
	ioport32_t ddr_d11;
	ioport32_t ddr_d12;
	ioport32_t ddr_d13;
	ioport32_t ddr_d14;
	ioport32_t ddr_d15;
	ioport32_t ddr_dqm0;
	ioport32_t ddr_dqm1;
	ioport32_t ddr_dqs0;
	ioport32_t ddr_dqsn0;
	ioport32_t ddr_dqs1;
	ioport32_t ddr_dqsn1;
	ioport32_t ddr_vref;
	ioport32_t ddr_vtp;
	ioport32_t ain7;
	ioport32_t ain6;
	ioport32_t ain5;
	ioport32_t ain4;
	ioport32_t ain3;
	ioport32_t ain2;
	ioport32_t ain1;
	ioport32_t ain0;
	ioport32_t vrefp;
	ioport32_t vrefn;
	ioport32_t avdd;
	ioport32_t avss;
	ioport32_t iforce;
	ioport32_t vsense;
	ioport32_t testout;
} am335x_iomux_conf_regs_t;
#endif /* KERN_AM335X_IOMUX_H_ */
