/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * @brief SB16 main structure combining all functionality
 */
#ifndef DRV_AUDIO_SB16_REGISTERS_H
#define DRV_AUDIO_SB16_REGISTERS_H

#include <ddi.h>

typedef struct sb16_regs {
	ioport8_t fm_address_status;
	ioport8_t fm_data;
	ioport8_t afm_address_status;
	ioport8_t afm_data;
	ioport8_t mixer_address;
#define MIXER_RESET_ADDRESS 0x00
#define MIXER_PNP_IRQ_ADDRESS 0x80
#define MIXER_PNP_DMA_ADDRESS 0x81
#define MIXER_IRQ_STATUS_ADDRESS 0x82 /* The Interrupt Status register,
                                       * addressed as register 82h on the
                                       * Mixer register map p.27 */
	ioport8_t mixer_data;
	ioport8_t dsp_reset;
	ioport8_t __reserved1; /* 0x7 */
	ioport8_t fm_address_status2;
	ioport8_t fm_data2;
	ioport8_t dsp_data_read;
	ioport8_t __reserved2; /* 0xb */
	ioport8_t dsp_write; /* Both command and data, bit 7 is write status */
#define DSP_WRITE_BUSY (1 << 7)
	ioport8_t __reserved3; /* 0xd */
	ioport8_t dsp_read_status; /* Bit 7 */
#define DSP_READ_READY (1 << 7)
	ioport8_t dma16_ack; /* 0xf */
	ioport8_t cd_command_data;
	ioport8_t cd_status;
	ioport8_t cd_reset;
	ioport8_t cd_enable;
} sb16_regs_t;

typedef struct mpu_regs {
	ioport8_t data;
#define MPU_CMD_ACK (0xfe)

	ioport8_t status_command;
#define MPU_STATUS_OUTPUT_BUSY (1 << 6)
#define MPU_STATUS_INPUT_BUSY (1 << 7)

#define MPU_CMD_RESET (0xff)
#define MPU_CMD_ENTER_UART (0x3f)
} mpu_regs_t;

#endif
/**
 * @}
 */
