/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup remcons
 * @{
 */
/** @file
 */

#ifndef TELNET_H_
#define TELNET_H_

#include <inttypes.h>

typedef uint8_t telnet_cmd_t;

/*
 * Telnet commands.
 */

#define TELNET_IAC 255

#define TELNET_WILL 251
#define TELNET_WONT 252
#define TELNET_DO 253
#define TELNET_DONT 254

#define TELNET_IS_OPTION_CODE(code) (((code) >= 251) && ((code) <= 254))

#define TELNET_ECHO 1
#define TELNET_SUPPRESS_GO_AHEAD 3
#define TELNET_LINEMODE 34

#endif

/**
 * @}
 */
