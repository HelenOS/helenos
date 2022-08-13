/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup audio
 * @brief HelenOS sound server
 * @{
 */
/** @file
 */

#ifndef LOG_H_
#define LOG_H_

#include <io/log.h>

#ifndef NAME
#define NAME "NONAME"
#endif

#include <stdio.h>

#define log_fatal(...) log_msg(LOG_DEFAULT, LVL_FATAL, ##__VA_ARGS__);
#define log_error(...) log_msg(LOG_DEFAULT, LVL_ERROR, ##__VA_ARGS__);
#define log_warning(...) log_msg(LOG_DEFAULT, LVL_WARN, ##__VA_ARGS__);
#define log_info(...) log_msg(LOG_DEFAULT, LVL_NOTE, ##__VA_ARGS__);
#define log_debug(...) log_msg(LOG_DEFAULT, LVL_DEBUG, ##__VA_ARGS__);
#define log_verbose(...) log_msg(LOG_DEFAULT, LVL_DEBUG2, ##__VA_ARGS__);

#endif

/**
 * @}
 */
