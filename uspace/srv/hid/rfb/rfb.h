/*
 * Copyright (c) 2013 Martin Sucha
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

#ifndef RFB_H__
#define RFB_H__

#include <inet/tcp.h>
#include <io/pixelmap.h>
#include <fibril_synch.h>

#define RFB_SECURITY_NONE 1
#define RFB_SECURITY_HANDSHAKE_OK 0

#define RFB_CMSG_SET_PIXEL_FORMAT 0
#define RFB_CMSG_SET_ENCODINGS 2
#define RFB_CMSG_FRAMEBUFFER_UPDATE_REQUEST 3
#define RFB_CMSG_KEY_EVENT 4
#define RFB_CMSG_POINTER_EVENT 5
#define RFB_CMSG_CLIENT_CUT_TEXT 6

#define RFB_SMSG_FRAMEBUFFER_UPDATE 0
#define RFB_SMSG_SET_COLOR_MAP_ENTRIES 1
#define RFB_SMSG_BELL 2
#define RFB_SMSG_SERVER_CUT_TEXT 3

#define RFB_ENCODING_RAW 0
#define RFB_ENCODING_TRLE 15

#define RFB_TILE_ENCODING_RAW 0
#define RFB_TILE_ENCODING_SOLID 1

typedef struct {
	uint8_t bpp;
	uint8_t depth;
	uint8_t big_endian;
	uint8_t true_color;
	uint16_t r_max;
	uint16_t g_max;
	uint16_t b_max;
	uint8_t r_shift;
	uint8_t g_shift;
	uint8_t b_shift;
	uint8_t pad[3];
} __attribute__((packed)) rfb_pixel_format_t;

typedef struct {
	uint16_t width;
	uint16_t height;
	rfb_pixel_format_t pixel_format;
	uint32_t name_length;
	char name[0];
} __attribute__((packed)) rfb_server_init_t;

typedef struct {
	uint8_t message_type;
	uint8_t pad[3];
	rfb_pixel_format_t pixel_format;
} __attribute__((packed)) rfb_set_pixel_format_t;

typedef struct {
	uint8_t message_type;
	uint8_t pad;
	uint16_t count;
} __attribute__((packed)) rfb_set_encodings_t;

typedef struct {
	uint8_t message_type;
	uint8_t incremental;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
} __attribute__((packed)) rfb_framebuffer_update_request_t;

typedef struct {
	uint8_t message_type;
	uint8_t down_flag;
	uint8_t pad[2];
	uint32_t key;
} __attribute__((packed)) rfb_key_event_t;

typedef struct {
	uint8_t message_type;
	uint8_t button_mask;
	uint16_t x;
	uint16_t y;
} __attribute__((packed)) rfb_pointer_event_t;

typedef struct {
	uint8_t message_type;
	uint8_t pad;
	uint32_t length;
	char text[0];
} __attribute__((packed)) rfb_client_cut_text_t;

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	int32_t enctype;
	uint8_t data[0];
} __attribute__((packed)) rfb_rectangle_t;

typedef struct {
	uint8_t message_type;
	uint8_t pad;
	uint16_t rect_count;
} __attribute__((packed)) rfb_framebuffer_update_t;

typedef struct {
	uint8_t message_type;
	uint8_t pad;
	uint16_t first_color;
	uint16_t color_count;
} __attribute__((packed)) rfb_set_color_map_entries_t;

typedef struct {
	uint16_t red;
	uint16_t green;
	uint16_t blue;
} __attribute__((packed)) rfb_color_map_entry_t;

typedef struct {
	uint16_t width;
	uint16_t height;
	rfb_pixel_format_t pixel_format;
	const char *name;
	tcp_t *tcp;
	tcp_listener_t *lst;
	pixelmap_t framebuffer;
	rfb_rectangle_t damage_rect;
	bool damage_valid;
	fibril_mutex_t lock;
	pixel_t *palette;
	size_t palette_used;
	bool supports_trle;
} rfb_t;


extern errno_t rfb_init(rfb_t *, uint16_t, uint16_t, const char *);
extern errno_t rfb_set_size(rfb_t *, uint16_t, uint16_t);
extern errno_t rfb_listen(rfb_t *, uint16_t);

#endif
