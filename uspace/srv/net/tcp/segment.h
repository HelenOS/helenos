/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup tcp
 * @{
 */
/** @file Segment processing
 */

#ifndef SEGMENT_H
#define SEGMENT_H

#include <stddef.h>
#include <stdint.h>
#include "tcp_type.h"

extern void tcp_segment_delete(tcp_segment_t *);
extern tcp_segment_t *tcp_segment_dup(tcp_segment_t *);
extern tcp_segment_t *tcp_segment_make_ctrl(tcp_control_t);
extern tcp_segment_t *tcp_segment_make_rst(tcp_segment_t *);
extern tcp_segment_t *tcp_segment_make_data(tcp_control_t, void *, size_t);
extern void tcp_segment_trim(tcp_segment_t *, uint32_t, uint32_t);
extern void tcp_segment_text_copy(tcp_segment_t *, void *, size_t);
extern size_t tcp_segment_text_size(tcp_segment_t *);
extern void tcp_segment_dump(tcp_segment_t *);

#endif

/** @}
 */
