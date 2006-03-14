/*
 * Copyright (C) 2006 Martin Decky
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

#include <arch/drivers/cuda.h>
#include <arch/asm.h>

#define CUDA_PACKET 0x01
#define CUDA_POWERDOWN 0x0a

#define RS 0x200
#define B (0 * RS)
#define A (1 * RS)
#define SR (10 * RS)
#define ACR (11 * RS)

#define SR_OUT 0x10
#define TACK 0x10
#define TIP 0x20


static volatile __u8 *cuda = (__u8 *) 0xf2000000;


static void cuda_packet(const __u8 data)
{
	cuda[B] = cuda[B] | TIP;
	cuda[ACR] = cuda[ACR] | SR_OUT;
	cuda[SR] = CUDA_PACKET;
	cuda[B] = cuda[B] & ~TIP;
	
	cuda[ACR] = cuda[ACR] | SR_OUT;
	cuda[SR] = data;
	cuda[B] = cuda[B] | TACK;
	
	cuda[B] = cuda[B] | TIP;
}


void cpu_halt(void) {
	cuda_packet(CUDA_POWERDOWN);
	
	cpu_sleep();
}
