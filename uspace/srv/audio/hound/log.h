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
