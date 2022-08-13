/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * @brief DDF log helper macros
 */
#ifndef DRV_AUDIO_SB16_DDF_LOG_H
#define DRV_AUDIO_SB16_DDF_LOG_H

#include <ddf/log.h>

#define ddf_log_fatal(msg...) ddf_msg(LVL_FATAL, msg)
#define ddf_log_error(msg...) ddf_msg(LVL_ERROR, msg)
#define ddf_log_warning(msg...) ddf_msg(LVL_WARN, msg)
#define ddf_log_note(msg...) ddf_msg(LVL_NOTE, msg)
#define ddf_log_debug(msg...) ddf_msg(LVL_DEBUG, msg)
#define ddf_log_verbose(msg...) ddf_msg(LVL_DEBUG2, msg)

#endif
/**
 * @}
 */
