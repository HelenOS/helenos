/*
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup fs
 * @{
 */ 


#ifndef _MFS_CONST_H_
#define _MFS_CONST_H_

#include <sys/types.h>
#include <bool.h>

#define MFS_MAX_BLOCK_SIZE	4096
#define MFS_MIN_BLOCK_SIZE	1024

#define MFS_ROOT_INO		1
#define MFS_SUPER_BLOCK		0
#define MFS_SUPER_BLOCK_SIZE	1024

#define V2_NR_DIRECT_ZONES	7
#define V2_NR_INDIRECT_ZONES	3

#define V1_NR_DIRECT_ZONES	7
#define V1_NR_INDIRECT_ZONES	2

#define V1_MAX_NAME_LEN		14
#define V1L_MAX_NAME_LEN	30
#define V2_MAX_NAME_LEN		14
#define V2L_MAX_NAME_LEN	30
#define V3_MAX_NAME_LEN		60

#endif


/**
 * @}
 */ 

