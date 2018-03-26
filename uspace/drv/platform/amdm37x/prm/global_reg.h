/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup amdm37xdrvprm
 * @{
 */
/** @file
 * @brief Clock Control Clock Management IO register structure.
 */
#ifndef AMDM37X_PRM_GLOBAL_REG_H
#define AMDM37X_PRM_GLOBAL_REG_H

#include <ddi.h>
#include <macros.h>

/* AM/DM37x TRM p.536 and p.615 */
#define GLOBAL_REG_PRM_BASE_ADDRESS  0x48307200
#define GLOBAL_REG_PRM_SIZE  65536

/** Global Reg PRM register map
 */
typedef struct {
	PADD32(8);
	struct {
		ioport32_t smps_sa;
#define GLOBAL_REG_PRM_VC_SMPS_SA_SA0_MASK   (0x7f << 0)
#define GLOBAL_REG_PRM_VC_SMPS_SA_SA0_CREATE(x)   (((x) & 0x7f) << 0)
#define GLOBAL_REG_PRM_VC_SMPS_SA_SA0_GET(r)   (r & 0x7f)
#define GLOBAL_REG_PRM_VC_SMPS_SA_SA1_MASK   (0x7f << 16)
#define GLOBAL_REG_PRM_VC_SMPS_SA_SA1_CREATE(x)   (((x) & 0x7f) << 16)
#define GLOBAL_REG_PRM_VC_SMPS_SA_SA1_GET(r)   (((r) >> 16 ) & 0x7f)

		ioport32_t smps_vol_ra;
#define GLOBAL_REG_PRM_VC_SMPS_VOL_RA_VOLRA0_MASK   (0xff << 0)
#define GLOBAL_REG_PRM_VC_SMPS_VOL_RA_VOLRA0_CREATE(x)   (((x) & 0xff) << 0)
#define GLOBAL_REG_PRM_VC_SMPS_VOL_RA_VOLRA0_GET(r)   (r & 0xff)
#define GLOBAL_REG_PRM_VC_SMPS_VOL_RA_VOLRA1_MASK   (0xff << 16)
#define GLOBAL_REG_PRM_VC_SMPS_VOL_RA_VOLRA1_CREATE(x)   (((x) & 0xff) << 16)
#define GLOBAL_REG_PRM_VC_SMPS_VOL_RA_VOLRA1_GET(r)   (((r) >> 16 ) & 0xff)

		ioport32_t smps_cmd_ra;
#define GLOBAL_REG_PRM_VC_SMPS_CMD_RA_CMDRA0_MASK   (0xff << 0)
#define GLOBAL_REG_PRM_VC_SMPS_CMD_RA_CMDRA0_CREATE(x)   (((x) & 0xff) << 0)
#define GLOBAL_REG_PRM_VC_SMPS_CMD_RA_CMDRA0_GET(r)   (r & 0xff)
#define GLOBAL_REG_PRM_VC_SMPS_CMD_RA_CMDRA1_MASK   (0xff << 16)
#define GLOBAL_REG_PRM_VC_SMPS_CMD_RA_CMDRA1_CREATE(x)   (((x) & 0xff) << 16)
#define GLOBAL_REG_PRM_VC_SMPS_CMD_RA_CMDRA1_GET(r)   (((r) >> 16 ) & 0xff)

		ioport32_t cmd_val_0;
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_ON_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_ON_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_ON_GET(r)   (((x) >> 24) & 0xff)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_ONLP_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_ONLP_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_ONLP_GET(r)   (((x) >> 24) & 0xff)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_RET_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_RET_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_RET_GET(r)   (((x) >> 24) & 0xff)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_OFF_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_OFF_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_0_OFF_GET(r)   (((x) >> 24) & 0xff)

		ioport32_t cmd_val_1;
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_ON_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_ON_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_ON_GET(r)   (((x) >> 24) & 0xff)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_ONLP_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_ONLP_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_ONLP_GET(r)   (((x) >> 24) & 0xff)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_RET_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_RET_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_RET_GET(r)   (((x) >> 24) & 0xff)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_OFF_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_OFF_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VC_CMD_VAL_1_OFF_GET(r)   (((x) >> 24) & 0xff)

		ioport32_t ch_conf;
#define GLOBAL_REG_PRM_VC_CH_CONF_CMD1_FLAG   (1 << 20)
#define GLOBAL_REG_PRM_VC_CH_CONF_RACEN1_FLAG   (1 << 19)
#define GLOBAL_REG_PRM_VC_CH_CONF_RAC1_FLAG   (1 << 18)
#define GLOBAL_REG_PRM_VC_CH_CONF_RAV1_FLAG   (1 << 17)
#define GLOBAL_REG_PRM_VC_CH_CONF_SA1_FLAG   (1 << 16)
#define GLOBAL_REG_PRM_VC_CH_CONF_CMD0_FLAG   (1 << 4)
#define GLOBAL_REG_PRM_VC_CH_CONF_RACEN0_FLAG   (1 << 3)
#define GLOBAL_REG_PRM_VC_CH_CONF_RAC0_FLAG   (1 << 2)
#define GLOBAL_REG_PRM_VC_CH_CONF_RAV0_FLAG   (1 << 1)
#define GLOBAL_REG_PRM_VC_CH_CONF_SA0_FLAG   (1 << 0)

		ioport32_t i2c_cfg;
#define GLOBAL_REG_PRM_VC_I2C_CFG_HSMASTER_FLAG   (1 << 5)
#define GLOBAL_REG_PRM_VC_I2C_CFG_SREN_FLAG   (1 << 4)
#define GLOBAL_REG_PRM_VC_I2C_CFG_HSEN_FLAG   (1 << 3)
#define GLOBAL_REG_PRM_VC_I2C_CFG_MCODE_MASK   (0x3 << 0)
#define GLOBAL_REG_PRM_VC_I2C_CFG_MCODE_CREATE(x)   ((x) & 0x3)
#define GLOBAL_REG_PRM_VC_I2C_CFG_MCODE_GET(r)   ((r) & 0x3)

		ioport32_t bypass_val;
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_VALID_FLAG   (1 << 24)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_DATA_MASK   (0xff << 16)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_DATA_CREATE(x)   (((x) & 0xff) << 16)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_DATA_GET(r)   (((r) >> 16) & 0xff)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_REGADDR_MASK   (0xff << 8)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_REGADDR_CREATE(x)   (((x) & 0xff) << 8)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_REGADDR_GET(r)   (((r) >> 8) & 0xff)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_SLAVEADDR_MASK   (0x7f << 0)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_SLAVEADDR_CREATE(x)   (((x) & 0x7f) << 0)
#define GLOBAL_REG_PRM_VC_BYPASS_VAL_SLAVEADDR_GET(r)   (((r) >> 0) & 0x7f)
	} vc;

	PADD32(4);
	ioport32_t rstctrl;
#define GLOBAL_REG_PRM_RSTCTRL_RST_DPLL3_FLAG   (1 << 2)
#define GLOBAL_REG_PRM_RSTCTRL_RST_GS_FLAG   (1 << 1)

	ioport32_t rsttime;
#define GLOBAL_REG_PRM_RSTTIME_RSTTIME2_MASK   (0x1f << 8)
#define GLOBAL_REG_PRM_RSTTIME_RSTTIME2_CREATE(x)   (((x) & 0x1f) << 8)
#define GLOBAL_REG_PRM_RSTTIME_RSTTIME2_GET(r)   (((r) >> 8) & 0x1f)
#define GLOBAL_REG_PRM_RSTTIME_RSTTIME1_MASK   (0xff << 0)
#define GLOBAL_REG_PRM_RSTTIME_RSTTIME1_CREATE(x)   (((x) & 0xff) << 0)
#define GLOBAL_REG_PRM_RSTTIME_RSTTIME1_GET(r)   (((r) >> 0) & 0xff)

	ioport32_t rstst;
#define GLOBAL_REG_PRM_RSTST_ICECRUSHER_RST_FLAG   (1 << 10)
#define GLOBAL_REG_PRM_RSTST_ICEPICK_RST_FLAG   (1 << 9)
#define GLOBAL_REG_PRM_RSTST_VDD2_VOLTAGE_MGR_RST_FLAG   (1 << 8)
#define GLOBAL_REG_PRM_RSTST_VDD1_VOLTAGE_MGR_RST_FLAG   (1 << 7)
#define GLOBAL_REG_PRM_RSTST_EXTERNAL_WARM_REST_FLAG   (1 << 6)
#define GLOBAL_REG_PRM_RSTST_MPU_WD_RST_FLAG   (1 << 4)
#define GLOBAL_REG_PRM_RSTST_GLOBAL_SW_RST_FLAG   (1 << 1)
#define GLOBAL_REG_PRM_RSTST_GLOABL_COLD_RST_FLAG   (1 << 0)

	PADD32(1);
	ioport32_t volctrl;
#define GLOBAL_REG_PRM_VOLCTRL_SEL_VMODE_FLAG   (1 << 4)
#define GLOBAL_REG_PRM_VOLCTRL_SEL_OFF_FLAG   (1 << 3)
#define GLOBAL_REG_PRM_VOLCTRL_AUTO_OFF_FLAG   (1 << 2)
#define GLOBAL_REG_PRM_VOLCTRL_AUTO_RET_FLAG   (1 << 1)
#define GLOBAL_REG_PRM_VOLCTRL_AUTO_SLEEP_FLAG   (1 << 0)

	ioport32_t sram_pcharge;
#define GLOBAL_REG_PRM_SRAM_PCHARGE_PCHARGE_TIME_MASK   (0xff)
#define GLOBAL_REG_PRM_SRAM_PCHARGE_PCHARGE_TIME_CREATE(x)   ((x) & 0xff)
#define GLOBAL_REG_PRM_SRAM_PCHARGE_PCHARGE_TIME_GET(r)   ((r) & 0xff)

	PADD32(2);
	ioport32_t clksrc_ctrl;
#define GLOBAL_REG_PRM_CLKSRC_CTRL_DPLL4_CLKINP_DIV_65_FLAG   (1 << 8)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKDIV_MASK   (0x3 << 6)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKDIV_1   (0x1 << 6)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKDIV_2   (0x2 << 6)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKDIV_GET(r)   (((r) >> 6) & 0x3)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_AUTOEXTCLKMODE_MASK   (0x3 << 3)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_AUTOEXTCLKMODE_ON   (0x0 << 3)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_AUTOEXTCLKMODE_SLEEP   (0x1 << 3)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_AUTOEXTCLKMODE_RET   (0x2 << 3)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_AUTOEXTCLKMODE_OFF   (0x3 << 3)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_AUTOEXTCLKMODE_GET(r)   (((r) >> 3) & 0x3)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKSEL_MASK   (0x3 << 0)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKSEL_BYPASS   (0x0 << 0)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKSEL_OSCILLATOR   (0x1 << 0)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKSEL_UNKNOWN   (0x3 << 0)
#define GLOBAL_REG_PRM_CLKSRC_CTRL_SYSCLKSEL_GET(r)   (((r) >> 0) & 0x3)

	PADD32(3);
	const ioport32_t obs;
#define GLOBAL_REG_PRM_OBS_OBS_BUS_MASK   (0x3ff)

	PADD32(3);
	ioport32_t voltsetup1;
#define GLOBAL_REG_PRM_VOLTSETUP1_SETUPTIME2_MASK   (0xff << 16)
#define GLOBAL_REG_PRM_VOLTSETUP1_SETUPTIME2_CREATE(x)   (((x) & 0xff) << 16)
#define GLOBAL_REG_PRM_VOLTSETUP1_SETUPTIME2_GET(r)   (((r) >> 16) & 0xff)
#define GLOBAL_REG_PRM_VOLTSETUP1_SETUPTIME1_MASK   (0xff << 0)
#define GLOBAL_REG_PRM_VOLTSETUP1_SETUPTIME1_CREATE(x)   (((x) & 0xff) << 0)
#define GLOBAL_REG_PRM_VOLTSETUP1_SETUPTIME1_GET(r)   (((r) >> 0) & 0xff)

	ioport32_t voltoffset;
#define GLOBAL_REG_PRM_VOLTOFFSET_OFFSET_TIME_MASK   (0xffff << 0)
#define GLOBAL_REG_PRM_VOLTOFFSET_OFFSET_TIME_CREATE(x)   (((x) & 0xffff) << 0)
#define GLOBAL_REG_PRM_VOLTOFFSET_OFFSET_TIME_GET(r)   (((r) >> 0) & 0xffff)

	ioport32_t clksetup;
#define GLOBAL_REG_PRM_CLKSETUP_SETUP_TIME_MASK   (0xffff << 0)
#define GLOBAL_REG_PRM_CLKSETUP_SETUP_TIME_CREATE(x)   (((x) & 0xffff) << 0)
#define GLOBAL_REG_PRM_CLKSETUP_SETUP_TIME_GET(r)   (((r) >> 0) & 0xffff)

	ioport32_t polctrl;
#define GLOBAL_REG_PRM_POLCTRL_OFFMODE_POL_FLAG   (1 << 3)
#define GLOBAL_REG_PRM_POLCTRL_CLKOUT_POL_FLAG   (1 << 2)
#define GLOBAL_REG_PRM_POLCTRL_CLKREG_POL_FLAG   (1 << 1)
#define GLOBAL_REG_PRM_POLCTRL_EXTVOL_POL_FLAG   (1 << 0)

	ioport32_t voltsetup2;
#define GLOBAL_REG_PRM_VOLTSETUP2_OFFMODESETUPTIME_MASK   (0xffff << 0)
#define GLOBAL_REG_PRM_VOLTSETUP2_OFFMODESETUPTIME_CREATE(x)   (((x) & 0xffff) << 0)
#define GLOBAL_REG_PRM_VOLTSETUP2_OFFMODESETUPTIME_GET(r)   (((r) >> 0) & 0xffff)

	PADD32(3);
	struct {
		ioport32_t config;
#define GLOBAL_REG_PRM_VP_CONFIG_ERROROFFSET_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VP_CONFIG_ERROROFFSET_CREATE(x)   (((x) & 0xff) << 24)
#define GLOBAL_REG_PRM_VP_CONFIG_ERROROFFSET_GET(r)   (((r) >> 0xff << 24)
#define GLOBAL_REG_PRM_VP_CONFIG_ERRORGAIN_MASK   (0xff << 16)
#define GLOBAL_REG_PRM_VP_CONFIG_ERRORGAIN_CREATE(x)   (((x) & 0xff) << 16)
#define GLOBAL_REG_PRM_VP_CONFIG_ERRORGAIN_GET(r)   (((r) >> 0xff << 16)
#define GLOBAL_REG_PRM_VP_CONFIG_INITVOLTAGE_MASK   (0xff << 8)
#define GLOBAL_REG_PRM_VP_CONFIG_INITVOLTAGE_CREATE(x)   (((x) & 0xff) << 8)
#define GLOBAL_REG_PRM_VP_CONFIG_INITVOLTAGE_GET(r)   (((r) >> 0xff << 8)
#define GLOBAL_REG_PRM_VP_CONFIG_TIMEOUTEN_FLAG    (1 << 3)
#define GLOBAL_REG_PRM_VP_CONFIG_INITVDD_FLAG   (1 << 2)
#define GLOBAL_REG_PRM_VP_CONFIG_FORCEUPDATE_FLAG   (1 << 1)
#define GLOBAL_REG_PRM_VP_CONFIG_VPENABLE_FLAG   (1 << 0)

		ioport32_t vstepmin;
#define GLOBAL_REG_PRM_VP_VSTEPMIN_SMPSWAITTIMEMIN_MASK   (0xffff << 8)
#define GLOBAL_REG_PRM_VP_VSTEPMIN_SMPSWAITTIMEMIN_CREATE(x)   (((x)0xffff << 8)
#define GLOBAL_REG_PRM_VP_VSTEPMIN_SMPSWAITTIMEMIN_GET(r)   (((r) >> 8) & 0xffff)
#define GLOBAL_REG_PRM_VP_VSTEPMIN_VSTEPMIN_MASK   (0xff << 0)
#define GLOBAL_REG_PRM_VP_VSTEPMIN_VSTEPMIN_CREATE(x)   (((x)0xff << 0)
#define GLOBAL_REG_PRM_VP_VSTEPMIN_VSTEPMIN_GET(r)   (((r) >> 0) & 0xff)

		ioport32_t vstepmax;
#define GLOBAL_REG_PRM_VP_VSTEPMAX_SMPSWAITTIMEMIN_MASK   (0xffff << 8)
#define GLOBAL_REG_PRM_VP_VSTEPMAX_SMPSWAITTIMEMIN_CREATE(x)   (((x)0xffff << 8)
#define GLOBAL_REG_PRM_VP_VSTEPMAX_SMPSWAITTIMEMIN_GET(r)   (((r) >> 8) & 0xffff)
#define GLOBAL_REG_PRM_VP_VSTEPMAX_VSTEPMIN_MASK   (0xff << 0)
#define GLOBAL_REG_PRM_VP_VSTEPMAX_VSTEPMIN_CREATE(x)   (((x)0xff << 0)
#define GLOBAL_REG_PRM_VP_VSTEPMAX_VSTEPMIN_GET(r)   (((r) >> 0) & 0xff)

		ioport32_t vlimitto;
#define GLOBAL_REG_PRM_VP_VLIMITTO_VDDMAX_MASK   (0xff << 24)
#define GLOBAL_REG_PRM_VP_VLIMITTO_VDDMAX_CREATE(x)   (((x)0xff << 24)
#define GLOBAL_REG_PRM_VP_VLIMITTO_VDDMAX_GET(r)   (((r) >> 24) & 0xff)
#define GLOBAL_REG_PRM_VP_VLIMITTO_VDDMIN_MASK   (0xff << 16)
#define GLOBAL_REG_PRM_VP_VLIMITTO_VDDMIN_CREATE(x)   (((x)0xff << 16)
#define GLOBAL_REG_PRM_VP_VLIMITTO_VDDMIN_GET(r)   (((r) >> 16) & 0xff)
#define GLOBAL_REG_PRM_VP_VLIMITTO_TIMEOUT_MASK   (0xffff << 0)
#define GLOBAL_REG_PRM_VP_VLIMITTO_TIMEOUT_CREATE(x)   (((x)0xffff << 0)
#define GLOBAL_REG_PRM_VP_VLIMITTO_TIMEOUT_GET(r)   (((r) >> 0) & 0xffff)

		const ioport32_t voltage;
#define GLOBAL_REG_PRM_VP_VOLTAGE_VPVOLTAGE_MASK   (0xff)
#define GLOBAL_REG_PRM_VP_VOLTAGE_VPVOLTAGE_GET(r)   ((r) & 0xff)

		const ioport32_t status;
#define GLOBAL_REG_PRM_VP_STATUS_VPINIDLE_FLAG   (1 << 0)

		PADD32(2);
	} vp[2];

	ioport32_t ldo_abb_setup;
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_SR2_IN_TRANSITION   (1 << 6)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_SR2_STATUS_MASK   (0x3 << 3)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_SR2_STATUS_BYPASS   (0x0 << 3)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_SR2_STATUS_FBB   (0x2 << 3)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_SR2_OPP_CHANGE_FLAG  (1 << 2)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_OPP_SEL_MASK   (0x3 << 0)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_OPP_SEL_DEFAULT   (0x0 << 0)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_OPP_SEL_FAST   (0x1 << 0)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_OPP_SEL_NOMINAL   (0x2 << 0)
#define GLOBAL_REG_PRM_LDO_ABB_SETUP_OPP_SEL_SLOW   (0x3 << 0)

	ioport32_t ldo_abb_ctrl;
#define GLOBAL_REG_PRM_LDO_ABB_CTRL_SR2_WTCNT_VALUE_MASK   (0xff << 8)
#define GLOBAL_REG_PRM_LDO_ABB_CTRL_SR2_WTCNT_VALUE_CREATE(x)   (((x) & 0xff) << 8)
#define GLOBAL_REG_PRM_LDO_ABB_CTRL_SR2_WTCNT_VALUE_GET(r)   (((r) >> 8) & 0xff)
#define GLOBAL_REG_PRM_LDO_ABB_CTRL_ACTIVE_FBB_SEL_FLAG   (1 << 2)
#define GLOBAL_REG_PRM_LDO_ABB_CTRL_SR2EN   (1 << 0)
} global_reg_prm_regs_t;

#endif
/**
 * @}
 */
