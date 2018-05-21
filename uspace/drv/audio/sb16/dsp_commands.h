/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * @brief SB16 DSP Command constants
 */
#ifndef DRV_AUDIO_SB16_DSP_COMMANDS_H
#define DRV_AUDIO_SB16_DSP_COMMANDS_H

/** See Sound Blaster Series HW programming Guide Chapter 6. */
typedef enum dsp_command {
	/*
	 * Followed by unsigned byte of digital data,
	 * software controls sampling rate
	 */
	DIRECT_8B_OUTPUT = 0x10,
	/* Same as DIRECT_8B_OUTPUT but for input */
	DIRECT_8B_INPUT = 0x20,

	/*
	 * Followed by time constant.
	 * TC = 65536 - (256 000 000 /
	 *   (channels * sampling rate))
	 * Send only high byte
	 */
	TRANSFER_TIME_CONSTANT = 0x40,

	/*
	 * Followed by length.high and length.low
	 * starts single-cycle DMA, length is -1
	 */
	SINGLE_DMA_8B_OUTPUT = 0x14,
	/*
	 * Same as SINGLE_DMA_8B_OUTPUT, but for
	 * input
	 */
	SINGLE_DMA_8B_INPUT = 0x24,
	/*
	 * Starts single-cycle DMA using
	 * Creative ADPSM 8->2 bit compressed
	 * data, Followed by length.low
	 * and length.high. Length is -1
	 */
	SINGLE_DMA_8B_ADPCM_2B_OUT = 0x16,
	/*
	 * Starts single-cycle DMA using
	 * DPSM 8->2 bit compressed data
	 * with reference byte.
	 * Followed by length.low and
	 * length.high. Length is -1
	 */
	SINGLE_DMA_8B_ADPCM_2B_OUT_REF = 0x17,
	/*
	 * Same as
	 * SINGLE_DMA_8B_ADPCM_2B_OUT
	 */
	SINGLE_DMA_8B_ADPCM_4B_OUT = 0x74,
	/*
	 * Same as
	 * SINGLE_DMA_8B_ADPCM_2B_OUT_REF
	 */
	SINGLE_DMA_8B_ADPCM_4B_OUT_REF = 0x75,
	/*
	 * Same as
	 * SINGLE_DMA_8B_ADPCM_2B_OUT
	 */
	SINGLE_DMA_8B_ADPCM_3B_OUT = 0x76,
	/*
	 * Same as
	 * SINGLE_DMA_8B_ADPCM_2B_OUT_REF
	 */
	SINGLE_DMA_8B_ADPCM_3B_OUT_REF = 0x77,
	/*
	 * Stop sending DMA request,
	 * works for SINGLE and AUTO
	 */
	DMA_8B_PAUSE = 0xd0,
	/* Resume transfers paused by DMA_8B_PAUSE */
	DMA_8B_CONTINUE = 0xd4,

	/*
	 * Connect speaker via internal amplifier,
	 * has no effect on 4.xx
	 */
	SPEAKER_ON = 0xd1,
	/*
	 * Disconnect output from the amplifier,
	 * has no effect on 4.xx
	 */
	SPEAKER_OFF = 0xd3,

	/* Read DSP for MIDI data */
	MIDI_POLLING = 0x30,
	/*
	 * Start interrupt mode, interrupt will be
	 * generated when there is in-bound data.
	 * To exit send again
	 */
	MIDI_INTERRUPT = 0x31,
	/* Followed by midi_data */
	MIDI_OUTPUT = 0x38,

	/*
	 * Followed by duration.low, duration.high. Duration is -1
	 * In the units of sampling period. Generates interrupt
	 * at the end of period
	 */
	PAUSE = 0x80,
	/* Read 2 bytes, major and minor number */
	DSP_VERSION = 0xe1,

	/*
	 * Starts auto-init DMA mode using 8-bit
	 * Interrupt after every block.
	 * To terminate, switch to single or use
	 * EXIT command
	 */
	AUTO_DMA_8B_OUTPUT = 0x1c,
	/* Same as AUTO_DMA_8B_OUTPUT, but for input */
	AUTO_DMA_8B_INPUT = 0x2c,
	/*
	 * Same as AUTO_DMA_8B_OUTPUT, but use
	 * 8->2bit ADPCM audio format
	 */
	AUTO_DMA_8B_ADPCM_2B_REF = 0x1f,
	/* Same as AUTO_DMA_8B_ADPCM_2B_REF */
	AUTO_DMA_8B_ADPCM_4B_REF = 0x7d,
	/* Same as AUTO_DMA_8B_ADPCM_2B_REF */
	AUTO_DMA_8B_ADPCM_3B_REF = 0x7f,

	/* Ends DMA transfer and terminates I/O process */
	DMA_8B_EXIT = 0xda,

	/*
	 * Followed by size.low, size.high
	 * Used with HIGH_SPEED AUTO_DMA
	 */
	BLOCK_TRANSFER_SIZE = 0x48,
	/*
	 * Start UART MIDI polling mode, read and
	 * write from/to DSP is interpreted as
	 * read/write from/to MIDI.
	 * To exit use reset signal. Note that reset
	 * will restore previous state and won't do
	 * complete reset
	 */
	UART_MIDI_POLLING = 0x34,
	/*
	 * Same as UART_MIDI_POLLING, but use
	 * interrupts instead of polling.
	 */
	UART_MIDI_INTERRUPT = 0x35,
	/*
	 * Add time stamp to inbound data, the
	 * order is time.low time.mid time.high
	 * data
	 */
	UART_MIDI_POLLING_TS = 0x36,
	/*
	 * Same as UART_MIDI_POLLING_TS, but use
	 * interrupts instead of polling
	 */
	UART_MIDI_INTERRUPT_TS = 0x37,

	/* 0xff means amp is on, 0x00 means it's off */
	SPEAKER_STATUS = 0xd8,

	/*
	 * DSP will generate interrupt after
	 * every block. No other commands are
	 * accepted in this mode. To exit
	 * the mode send RESET command.
	 * Note that reset will restore
	 * previous state.
	 */
	AUTO_DMA_8B_HIGH_OUTPUT = 0x90,
	/* Same as AUTO_DMA_8B_HIGH_OUTPUT */
	AUTO_DMA_8B_HIGH_INPUT = 0x98,
	/*
	 * Transfer one block and exit,
	 * generates interrupt
	 */
	SINGLE_DMA_8B_HIGH_OUTPUT = 0x91,
	/* Same as SINGLE_DMA_8B_HIGH_OUTPUT */
	SINGLE_DMA_8B_HIGH_INPUT = 0x99,

	/* Mono mode is the default, only on 3.xx */
	SET_MONO_INPUT = 0xa0,
	/* Switch to stereo recording, only on 3.xx */
	SET_STEREO_INPUT = 0xa8,

	/*
	 * Followed by sapling rate
	 * 5000 to 45000 Hz, inclusive
	 */
	SET_SAMPLING_RATE_OUTPUT = 0x41,
	/* Same as SET_SAMPLING_RATE_OUTPUT */
	SET_SAMPLING_RATE_INPUT = 0x42,

	/*
	 * Followed by mode, size.low, size.high
	 * mode format is:
	 *    0x00 - unsigned mono
	 *    0x10 - signed mono
	 *    0x20 - unsigned stereo
	 *    0x30 - signed stereo
	 * Size is -1. Terminate AUTO_DMA by EXIT
	 * or switch to SINGLE_DMA
	 */
	SINGLE_DMA_16B_DA = 0xb0,
	SINGLE_DMA_16B_DA_FIFO = 0xb2,
	AUTO_DMA_16B_DA = 0xb4,
	AUTO_DMA_16B_DA_FIFO = 0xb6,
	SINGLE_DMA_16B_AD = 0xb8,
	SINGLE_DMA_16B_AD_FIFO = 0xba,
	AUTO_DMA_16B_AD = 0xbc,
	AUTO_DMA_16B_AD_FIFO = 0xbe,

	/*
	 * Followed by mode, size.low, size.high
	 * mode format is:
	 *    0x00 - unsigned mono
	 *    0x10 - signed mono
	 *    0x20 - unsigned stereo
	 *    0x30 - signed stereo
	 * Size is -1. Terminate AUTO_DMA by EXIT
	 * or switch to SINGLE_DMA
	 */
	SINGLE_DMA_8B_DA = 0xc0,
	SINGLE_DMA_8B_DA_FIFO = 0xc2,
	AUTO_DMA_8B_DA = 0xc4,
	AUTO_DMA_8B_DA_FIFO = 0xc6,
	SINGLE_DMA_8B_AD = 0xc8,
	SINGLE_DMA_8B_AD_FIFO = 0xca,
	AUTO_DMA_8B_AD = 0xcc,
	AUTO_DMA_8B_AD_FIFO = 0xce,

	/* Stop sending DMA request, both SINGLE and AUTO */
	DMA_16B_PAUSE = 0xd5,
	/* Resume requests paused by DMA_16B_PAUSE */
	DMA_16B_CONTINUE = 0xd6,
	/* Ends DMA transfer and terminates I/O process */
	DMA_16B_EXIT = 0xd9,
} dsp_command_t;

#define DSP_MODE_SIGNED 0x10
#define DSP_MODE_STEREO 0x20

static inline const char *mode_to_str(uint8_t mode)
{
	if (mode & 0xcf)
		return "unknown";
	static const char *names[] = {
		"unsigned mono (8bit)",
		"signed mono (16bit)",
		"unsigned stereo (8bit)",
		"signed stereo (16bit)",
	};
	return names[mode >> 4];
}

#endif
/**
 * @}
 */
