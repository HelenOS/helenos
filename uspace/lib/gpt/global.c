/*
 * Copyright (c) 2011-2013 Dominik Taborsky
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

/** @addtogroup libgpt
 * @{
 */
/** @file
 */

#include "libgpt.h"

/** GPT header signature ("EFI PART" in ASCII) */
const uint8_t efi_signature[8] = {
	0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54
};

const uint8_t revision[4] = {
	00, 00, 01, 00
};

const partition_type_t gpt_ptypes[] = {
	{ "unused entry",                 "00000000" "0000" "0000" "0000000000000000" }, /* 0 */
	{ "HelenOS System",               "3dc61fa0" "cf7a" "3ad8" "ac57615029d81a6b" }, /* "HelenOS System" encoded as RFC 4122 UUID, version 3 (MD5 name-based) */
	{ "MBR partition scheme",         "024dee41" "33e7" "11d3" "9d690008c781f39f" },
	{ "EFI System",                   "c12a7328" "f81f" "11d2" "ba4b00a0c93ec93b" },
	{ "BIOS Boot",                    "21686148" "6449" "6e6f" "744e656564454649" },
	{ "Windows Reserved",             "e3c9e316" "0b5c" "4db8" "817df92df00215ae" },
	{ "Windows Basic data",           "ebd0a0a2" "b9e5" "4433" "87c068b6b72699c7" },
	{ "Windows LDM metadata",         "5808c8aa" "7e8f" "42e0" "85d2e1e90434cfb3" },
	{ "Windows LDM data",             "af9b60a0" "1431" "4f62" "bc683311714a69ad" },
	{ "Windows Recovery Environment", "de94bba4" "06d1" "4d40" "a16abfd50179d6ac" },
	{ "Windows IBM GPFS",             "37affc90" "ef7d" "4e96" "91c32d7ae055b174" }, /* 10 */
	{ "Windows Cluster metadata",     "db97dba9" "0840" "4bae" "97f0ffb9a327c7e1" },
	{ "HP-UX Data",                   "75894c1e" "3aeb" "11d3" "b7c17b03a0000000" },
	{ "HP-UX Service",                "e2a1e728" "32e3" "11d6" "a6827b03a0000000" },
	{ "Linux filesystem data",        "0fc63daf" "8483" "4772" "8e793d69d8477de4" },
	{ "Linux RAID",                   "a19d880f" "05fc" "4d3b" "a006743f0f84911e" },
	{ "Linux Swap",                   "0657fd6d" "a4ab" "43c4" "84e50933c84b4f4f" },
	{ "Linux LVM",                    "e6d6d379" "f507" "44c2" "a23c238f2a3df928" },
	{ "Linux filesystem data",        "933ac7e1" "2eb4" "4f13" "b8440e14e2aef915" },
	{ "Linux Reserved",               "8da63339" "0007" "60c0" "c436083ac8230908" },
	{ "FreeBSD Boot",                 "83bd6b9d" "7f41" "11dc" "be0b001560b84f0f" }, /* 20 */
	{ "FreeBSD Data",                 "516e7cb4" "6ecf" "11d6" "8ff800022d09712b" },
	{ "FreeBSD Swap",                 "516e7cb5" "6ecf" "11d6" "8ff800022d09712b" },
	{ "FreeBSD UFS",                  "516e7cb6" "6ecf" "11d6" "8ff800022d09712b" },
	{ "FreeBSD Vinum VM",             "516e7cb8" "6ecf" "11d6" "8ff800022d09712b" },
	{ "FreeBSD ZFS",                  "516e7cba" "6ecf" "11d6" "8ff800022d09712b" },
	{ "Mac OS X HFS+",                "48465300" "0000" "11aa" "aa1100306543ecac" },
	{ "Mac OS X UFS",                 "55465300" "0000" "11aa" "aa1100306543ecac" },
	{ "Mac OS X ZFS",                 "6a898cc3" "1dd2" "11b2" "99a6080020736631" },
	{ "Mac OS X RAID",                "52414944" "0000" "11aa" "aa1100306543ecac" },
	{ "Mac OS X RAID, offline",       "52414944" "5f4f" "11aa" "aa1100306543ecac" }, /* 30 */
	{ "Mac OS X Boot",                "426f6f74" "0000" "11aa" "aa1100306543ecac" },
	{ "Mac OS X Label",               "4c616265" "6c00" "11aa" "aa1100306543ecac" },
	{ "Mac OS X TV Recovery",         "5265636f" "7665" "11aa" "aa1100306543ecac" },
	{ "Mac OS X Core Storage",        "53746f72" "6167" "11aa" "aa1100306543ecac" },
	{ "Solaris Boot",                 "6a82cb45" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Root",                 "6a85cf4d" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Swap",                 "6a87c46f" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Backup",               "6a8b642b" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris /usr",                 "6a898cc3" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris /var",                 "6a8ef2e9" "1dd2" "11b2" "99a6080020736631" }, /* 40 */
	{ "Solaris /home",                "6a90ba39" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Alternate sector",     "6a9283a5" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Reserved",             "6a945a3b" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Reserved",             "6a9630d1" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Reserved",             "6a980767" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Reserved",             "6a96237f" "1dd2" "11b2" "99a6080020736631" },
	{ "Solaris Reserved",             "6a8d2ac7" "1dd2" "11b2" "99a6080020736631" },
	{ "NetBSD Swap",                  "49f48d32" "b10e" "11dc" "b99b0019d1879648" },
	{ "NetBSD FFS",                   "49f48d5a" "b10e" "11dc" "b99b0019d1879648" },
	{ "NetBSD LFS",                   "49f48d82" "b10e" "11dc" "b99b0019d1879648" }, /* 50 */
	{ "NetBSD RAID",                  "49f48daa" "b10e" "11dc" "b99b0019d1879648" },
	{ "NetBSD Concatenated",          "2db519c4" "b10f" "11dc" "b99b0019d1879648" },
	{ "NetBSD Encrypted",             "2db519ec" "b10f" "11dc" "b99b0019d1879648" },
	{ "ChromeOS ChromeOS kernel",     "fe3a2a5d" "4f32" "41a7" "b725accc3285a309" },
	{ "ChromeOS rootfs",              "3cb8e202" "3b7e" "47dd" "8a3c7ff2a13cfcec" },
	{ "ChromeOS future use",          "2e0a753d" "9e48" "43b0" "8337b15192cb1b5e" },
	{ "MidnightBSD Boot",             "85d5e45e" "237c" "11e1" "b4b3e89a8f7fc3a7" },
	{ "MidnightBSD Data",             "85d5e45a" "237c" "11e1" "b4b3e89a8f7fc3a7" },
	{ "MidnightBSD Swap",             "85d5e45b" "237c" "11e1" "b4b3e89a8f7fc3a7" },
	{ "MidnightBSD UFS",              "0394ef8b" "237e" "11e1" "b4b3e89a8f7fc3a7" }, /* 60 */
	{ "MidnightBSD Vinum VM",         "85d5e45c" "237c" "11e1" "b4b3e89a8f7fc3a7" },
	{ "MidnightBSD ZFS",              "85d5e45d" "237c" "11e1" "b4b3e89a8f7fc3a7" },
	{ "unknown entry",                NULL } /* Keep this as the last entry */
};
