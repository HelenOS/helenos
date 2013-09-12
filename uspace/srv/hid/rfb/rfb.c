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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <inttypes.h>
#include <str.h>
#include <str_error.h>
#include <byteorder.h>
#include <macros.h>
#include <io/log.h>

#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>

#include "rfb.h"

/** Buffer for receiving the request. */
#define BUFFER_SIZE  1024

static char rbuf[BUFFER_SIZE];
static size_t rbuf_out;
static size_t rbuf_in;


/** Receive one character (with buffering) */
static int recv_char(int fd, char *c)
{
	if (rbuf_out == rbuf_in) {
		rbuf_out = 0;
		rbuf_in = 0;
		
		ssize_t rc = recv(fd, rbuf, BUFFER_SIZE, 0);
		if (rc <= 0)
			return rc;
		
		rbuf_in = rc;
	}
	
	*c = rbuf[rbuf_out++];
	return EOK;
}

/** Receive count characters (with buffering) */
static int recv_chars(int fd, char *c, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		int rc = recv_char(fd, c);
		if (rc != EOK)
			return rc;
		c++;
	}
	return EOK;
}

static int recv_skip_chars(int fd, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		char c;
		int rc = recv_char(fd, &c);
		if (rc != EOK)
			return rc;
	}
	return EOK;
}

static void rfb_pixel_format_to_be(rfb_pixel_format_t *src, rfb_pixel_format_t *dst)
{
	dst->r_max = host2uint16_t_be(src->r_max);
	dst->g_max = host2uint16_t_be(src->g_max);
	dst->b_max = host2uint16_t_be(src->b_max);
}

static void rfb_pixel_format_to_host(rfb_pixel_format_t *src, rfb_pixel_format_t *dst)
{
	dst->r_max = uint16_t_be2host(src->r_max);
	dst->g_max = uint16_t_be2host(src->g_max);
	dst->b_max = uint16_t_be2host(src->b_max);
}

static void rfb_server_init_to_be(rfb_server_init_t *src, rfb_server_init_t *dst)
{
	dst->width = host2uint16_t_be(src->width);
	dst->height = host2uint16_t_be(src->height);
	rfb_pixel_format_to_be(&src->pixel_format, &dst->pixel_format);
	dst->name_length = host2uint32_t_be(src->name_length);
}

static void rfb_set_encodings_to_host(rfb_set_encodings_t *src, rfb_set_encodings_t *dst)
{
	dst->count = uint16_t_be2host(src->count);
}

static void rfb_framebuffer_update_request_to_host(rfb_framebuffer_update_request_t *src,
    rfb_framebuffer_update_request_t *dst)
{
	dst->x = uint16_t_be2host(src->x);
	dst->y = uint16_t_be2host(src->x);
	dst->width = uint16_t_be2host(src->width);
	dst->height = uint16_t_be2host(src->height);
}

static void rfb_framebuffer_update_to_be(rfb_framebuffer_update_t *src,
    rfb_framebuffer_update_t *dst)
{
	dst->rect_count = host2uint16_t_be(src->rect_count);
}

static void rfb_rectangle_to_be(rfb_rectangle_t *src, rfb_rectangle_t *dst)
{
	dst->x = host2uint16_t_be(src->x);
	dst->y = host2uint16_t_be(src->y);
	dst->width = host2uint16_t_be(src->width);
	dst->height = host2uint16_t_be(src->height);
	dst->enctype = host2uint32_t_be(src->enctype);
}

static void rfb_key_event_to_host(rfb_key_event_t *src, rfb_key_event_t *dst)
{
	dst->key = uint32_t_be2host(src->key);
}

static void rfb_pointer_event_to_host(rfb_pointer_event_t *src, rfb_pointer_event_t *dst)
{
	dst->x = uint16_t_be2host(src->x);
	dst->y = uint16_t_be2host(src->y);
}

static void rfb_client_cut_text_to_host(rfb_client_cut_text_t *src,
    rfb_client_cut_text_t *dst)
{
	dst->length = uint32_t_be2host(src->length);
}

int rfb_init(rfb_t *rfb, uint16_t width, uint16_t height, const char *name)
{
	memset(rfb, 0, sizeof(rfb_t));
	fibril_mutex_initialize(&rfb->lock);
	
	rfb_pixel_format_t *pf = &rfb->pixel_format;
	pf->bpp = 32;
	pf->depth = 24;
	pf->big_endian = 1;
	pf->true_color = 1;
	pf->r_max = 255;
	pf->g_max = 255;
	pf->b_max = 255;
	pf->r_shift = 0;
	pf->g_shift = 8;
	pf->b_shift = 16;
	
	rfb->name = str_dup(name);
	rfb->supports_trle = false;
	
	return rfb_set_size(rfb, width, height);
}

int rfb_set_size(rfb_t *rfb, uint16_t width, uint16_t height)
{
	size_t new_size = width * height * sizeof(pixel_t);
	void *pixbuf = malloc(new_size);
	if (pixbuf == NULL)
		return ENOMEM;

	free(rfb->framebuffer.data);
	rfb->framebuffer.data = pixbuf;
	rfb->framebuffer.width = width;
	rfb->framebuffer.height = height;
	rfb->width = width;
	rfb->height = height;
	
	/* Fill with white */
	memset(rfb->framebuffer.data, 255, new_size);
	
	return EOK;
}

static int recv_message(int conn_sd, char type, void *buf, size_t size)
{
	memcpy(buf, &type, 1);
	return recv_chars(conn_sd, ((char *) buf) + 1, size -1);
}

static uint32_t rfb_scale_channel(uint8_t val, uint32_t max)
{
	return val * max / 255;
}

static void rfb_encode_index(rfb_t *rfb, uint8_t *buf, pixel_t pixel)
{
	int first_free_index = -1;
	for (size_t i = 0; i < 256; i++) {
		bool free = ALPHA(rfb->palette[i]) == 0;
		if (free && first_free_index == -1) {
			first_free_index = i;
		}
		else if (!free && RED(rfb->palette[i]) == RED(pixel) &&
		    GREEN(rfb->palette[i]) == GREEN(pixel) &&
		    BLUE(rfb->palette[i]) == BLUE(pixel)) {
			*buf = i;
			return;
		}
	}
	
	if (first_free_index != -1) {
		rfb->palette[first_free_index] = PIXEL(255, RED(pixel), GREEN(pixel),
		    BLUE(pixel));
		rfb->palette_used = max(rfb->palette_used, (unsigned) first_free_index + 1);
		*buf = first_free_index;
		return;
	}
	
	/* TODO find nearest color index. We are lazy so return index 0 for now */
	*buf = 0;
}

static void rfb_encode_true_color(rfb_pixel_format_t *pf, void *buf,
    pixel_t pixel)
{
	uint32_t pix = 0;
	pix |= rfb_scale_channel(RED(pixel), pf->r_max) << pf->r_shift;
	pix |= rfb_scale_channel(GREEN(pixel), pf->g_max) << pf->g_shift;
	pix |= rfb_scale_channel(BLUE(pixel), pf->b_max) << pf->b_shift;
	
	if (pf->bpp == 8) {
		uint8_t pix8 = pix;
		memcpy(buf, &pix8, 1);
	}
	else if (pf->bpp == 16) {
		uint16_t pix16 = pix;
		if (pf->big_endian) {
			pix16 = host2uint16_t_be(pix16);
		}
		else {
			pix16 = host2uint16_t_le(pix16);
		}
		memcpy(buf, &pix16, 2);
	}
	else if (pf->bpp == 32) {
		if (pf->big_endian) {
			pix = host2uint32_t_be(pix);
		}
		else {
			pix = host2uint32_t_le(pix);
		}
		memcpy(buf, &pix, 4);
	}
}

static void rfb_encode_pixel(rfb_t *rfb, void *buf, pixel_t pixel)
{
	if (rfb->pixel_format.true_color) {
		rfb_encode_true_color(&rfb->pixel_format, buf, pixel);
	}
	else {
		rfb_encode_index(rfb, buf, pixel);
	}
}

static void rfb_set_color_map_entries_to_be(rfb_set_color_map_entries_t *src,
    rfb_set_color_map_entries_t *dst)
{
	dst->first_color = host2uint16_t_be(src->first_color);
	dst->color_count = host2uint16_t_be(src->color_count);
}

static void rfb_color_map_entry_to_be(rfb_color_map_entry_t *src,
    rfb_color_map_entry_t *dst)
{
	dst->red = host2uint16_t_be(src->red);
	dst->green = host2uint16_t_be(src->green);
	dst->blue = host2uint16_t_be(src->blue);
}

static void *rfb_send_palette_message(rfb_t *rfb, size_t *psize)
{
	size_t size = sizeof(rfb_set_color_map_entries_t) +
	    rfb->palette_used * sizeof(rfb_color_map_entry_t);
	
	void *buf = malloc(size);
	if (buf == NULL)
		return NULL;
	
	void *pos = buf;
	
	rfb_set_color_map_entries_t *scme = pos;
	scme->message_type = RFB_SMSG_SET_COLOR_MAP_ENTRIES;
	scme->first_color = 0;
	scme->color_count = rfb->palette_used;
	rfb_set_color_map_entries_to_be(scme, scme);
	pos += sizeof(rfb_set_color_map_entries_t);
	
	rfb_color_map_entry_t *entries = pos;
	for (unsigned i = 0; i < rfb->palette_used; i++) {
		entries[i].red = 65535 * RED(rfb->palette[i]) / 255;
		entries[i].green = 65535 * GREEN(rfb->palette[i]) / 255;
		entries[i].blue = 65535 * BLUE(rfb->palette[i]) / 255;
		rfb_color_map_entry_to_be(&entries[i], &entries[i]);
	}
	
	*psize = size;
	return buf;
}

static size_t rfb_rect_encode_raw(rfb_t *rfb, rfb_rectangle_t *rect, void *buf)
{
	size_t pixel_size = rfb->pixel_format.bpp / 8;
	size_t size = (rect->width * rect->height * pixel_size);
	
	if (buf == NULL)
		return size;
	
	for (uint16_t y = 0; y < rect->height; y++) {
		for (uint16_t x = 0; x < rect->width; x++) {
			pixel_t pixel = pixelmap_get_pixel(&rfb->framebuffer,
			    x + rect->x, y + rect->y);
			rfb_encode_pixel(rfb, buf, pixel);
			buf += pixel_size;
		}
	}
	
	return size;
}

typedef enum {
	COMP_NONE, COMP_SKIP_START, COMP_SKIP_END
} cpixel_compress_type_t;

typedef struct {
	size_t size;
	cpixel_compress_type_t compress_type;
} cpixel_ctx_t;

static void cpixel_context_init(cpixel_ctx_t *ctx, rfb_pixel_format_t *pixel_format)
{
	ctx->size = pixel_format->bpp / 8;
	ctx->compress_type = COMP_NONE;
	
	if (pixel_format->bpp == 32 && pixel_format->depth <= 24) {
		uint32_t mask = 0;
		mask |= pixel_format->r_max << pixel_format->r_shift;
		mask |= pixel_format->g_max << pixel_format->g_shift;
		mask |= pixel_format->b_max << pixel_format->b_shift;
		
		if (pixel_format->big_endian) {
			mask = host2uint32_t_be(mask);
		}
		else {
			mask = host2uint32_t_le(mask);
		}
		
		uint8_t *mask_data = (uint8_t *) &mask;
		if (mask_data[0] == 0) {
			ctx->compress_type = COMP_SKIP_START;
			ctx->size = 3;
		}
		else if (mask_data[3] == 0) {
			ctx->compress_type = COMP_SKIP_END;
			ctx->size = 3;
		}
	}
}

static void cpixel_encode(rfb_t *rfb, cpixel_ctx_t *cpixel, void *buf,
    pixel_t pixel)
{
	uint8_t data[4];
	rfb_encode_pixel(rfb, data, pixel);
	
	switch (cpixel->compress_type) {
	case COMP_NONE:
	case COMP_SKIP_END:
		memcpy(buf, data, cpixel->size);
		break;
	case COMP_SKIP_START:
		memcpy(buf, data + 1, cpixel->size);
	}
}

static ssize_t rfb_tile_encode_raw(rfb_t *rfb, cpixel_ctx_t *cpixel,
    rfb_rectangle_t *tile, void *buf)
{
	ssize_t size = tile->width * tile->height * cpixel->size;
	if (buf == NULL)
		return size;
	
	for (uint16_t y = tile->y; y < tile->y + tile->height; y++) {
		for (uint16_t x = tile->x; x < tile->x + tile->width; x++) {
			pixel_t pixel = pixelmap_get_pixel(&rfb->framebuffer, x, y);
			cpixel_encode(rfb, cpixel, buf, pixel);
		}
	}
	
	return size;
}

static ssize_t rfb_tile_encode_solid(rfb_t *rfb, cpixel_ctx_t *cpixel,
    rfb_rectangle_t *tile, void *buf)
{
	/* Check if it is single color */
	pixel_t the_color = pixelmap_get_pixel(&rfb->framebuffer, tile->x, tile->y);
	for (uint16_t y = tile->y; y < tile->y + tile->height; y++) {
		for (uint16_t x = tile->x; x < tile->x + tile->width; x++) {
			if (pixelmap_get_pixel(&rfb->framebuffer, x, y) != the_color)
				return -1;
		}
	}
	
	/* OK, encode it */
	if (buf)
		cpixel_encode(rfb, cpixel, buf, the_color);
	return cpixel->size;
}

static size_t rfb_rect_encode_trle(rfb_t *rfb, rfb_rectangle_t *rect, void *buf)
{
	cpixel_ctx_t cpixel;
	cpixel_context_init(&cpixel, &rfb->pixel_format);
	
	size_t size = 0;
	for (uint16_t y = 0; y < rect->height; y += 16) {
		for (uint16_t x = 0; x < rect->width; x += 16) {
			rfb_rectangle_t tile = {
				.x = x,
				.y = y,
				.width = (x + 16 <= rect->width ? 16 : rect->width - x),
				.height = (y + 16 <= rect->height ? 16 : rect->height - y)
			};
			
			size += 1;
			uint8_t *tile_enctype_ptr = buf;
			if (buf)
				buf +=  1;
			
			uint8_t tile_enctype = RFB_TILE_ENCODING_SOLID;
			ssize_t tile_size = rfb_tile_encode_solid(rfb, &cpixel, &tile, buf);
			if (tile_size < 0) {
				tile_size = rfb_tile_encode_raw(rfb, &cpixel, &tile, buf);
				tile_enctype = RFB_TILE_ENCODING_RAW;
			}
			
			if (buf) {
				*tile_enctype_ptr = tile_enctype;
				buf += tile_size;
			}
		}
	}
	return size;
}

static int rfb_send_framebuffer_update(rfb_t *rfb, int conn_sd, bool incremental)
{
	fibril_mutex_lock(&rfb->lock);
	if (!incremental || !rfb->damage_valid) {
		rfb->damage_rect.x = 0;
		rfb->damage_rect.y = 0;
		rfb->damage_rect.width = rfb->width;
		rfb->damage_rect.height = rfb->height;
	}
	

	/* We send only single raw rectangle right now */
	size_t buf_size = sizeof(rfb_framebuffer_update_t) +
	    sizeof(rfb_rectangle_t) * 1 +
	    rfb_rect_encode_raw(rfb, &rfb->damage_rect, NULL)
	    ;
	
	void *buf = malloc(buf_size);
	if (buf == NULL) {
		fibril_mutex_unlock(&rfb->lock);
		return ENOMEM;
	}
	memset(buf, 0, buf_size);
	
	void *pos = buf;
	rfb_framebuffer_update_t *fbu = buf;
	fbu->message_type = RFB_SMSG_FRAMEBUFFER_UPDATE;
	fbu->rect_count = 1;
	rfb_framebuffer_update_to_be(fbu, fbu);
	pos += sizeof(rfb_framebuffer_update_t);
	
	rfb_rectangle_t *rect = pos;
	pos += sizeof(rfb_rectangle_t);
	
	*rect = rfb->damage_rect;
	
	if (rfb->supports_trle) {
		rect->enctype = RFB_ENCODING_TRLE;
		pos += rfb_rect_encode_trle(rfb, rect, pos);
	}
	else {
		rect->enctype = RFB_ENCODING_RAW;
		pos += rfb_rect_encode_raw(rfb, rect, pos);
	}
	rfb_rectangle_to_be(rect, rect);
	
	rfb->damage_valid = false;
	
	size_t send_palette_size = 0;
	void *send_palette = NULL;
	
	if (!rfb->pixel_format.true_color) {
		send_palette = rfb_send_palette_message(rfb, &send_palette_size);
		if (send_palette == NULL) {
			free(buf);
			fibril_mutex_unlock(&rfb->lock);
			return ENOMEM;
		}
	}
	
	fibril_mutex_unlock(&rfb->lock);
	
	if (!rfb->pixel_format.true_color) {
		int rc = send(conn_sd, send_palette, send_palette_size, 0);
		if (rc != EOK) {
			free(buf);
			return rc;
		}
	}
	
	int rc = send(conn_sd, buf, buf_size, 0);
	free(buf);
	
	return rc;
}

static int rfb_set_pixel_format(rfb_t *rfb, rfb_pixel_format_t *pixel_format)
{
	rfb->pixel_format = *pixel_format;
	if (rfb->pixel_format.true_color) {
		free(rfb->palette);
		rfb->palette = NULL;
		rfb->palette_used = 0;
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "changed pixel format to %d-bit true color (%x<<%d, %x<<%d, %x<<%d)",
		    pixel_format->depth, pixel_format->r_max, pixel_format->r_shift,
		    pixel_format->g_max, pixel_format->g_shift, pixel_format->b_max,
		    pixel_format->b_shift);
	}
	else {
		if (rfb->palette == NULL) {
			rfb->palette = malloc(sizeof(pixel_t) * 256);
			if (rfb->palette == NULL)
				return ENOMEM;
			memset(rfb->palette, 0, sizeof(pixel_t) * 256);
			rfb->palette_used = 0;
		}
		log_msg(LOG_DEFAULT, LVL_DEBUG, "changed pixel format to %d-bit palette",
		    pixel_format->depth);
	}
	return EOK;
}

static void rfb_socket_connection(rfb_t *rfb, int conn_sd)
{
	/* Version handshake */
	int rc = send(conn_sd, "RFB 003.008\n", 12, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Failed sending server version %d", rc);
		return;
	}
	
	char client_version[12];
	rc = recv_chars(conn_sd, client_version, 12);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Failed receiving client version: %d", rc);
		return;
	}
	
	if (memcmp(client_version, "RFB 003.008\n", 12) != 0) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Client version is not RFB 3.8");
		return;
	}
	
	/* Security handshake
	 * 1 security type supported, which is 1 - None
	 */
	char sec_types[2];
	sec_types[0] = 1; /* length */
	sec_types[1] = RFB_SECURITY_NONE;
	rc = send(conn_sd, sec_types, 2, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN,
		    "Failed sending security handshake: %d", rc);
		return;
	}
	
	char selected_sec_type = 0;
	rc = recv_char(conn_sd, &selected_sec_type);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Failed receiving security type: %d", rc);
		return;
	}
	if (selected_sec_type != RFB_SECURITY_NONE) {
		log_msg(LOG_DEFAULT, LVL_WARN,
		    "Client selected security type other than none");
		return;
	}
	uint32_t security_result = RFB_SECURITY_HANDSHAKE_OK;
	rc = send(conn_sd, &security_result, sizeof(uint32_t), 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Failed sending security result: %d", rc);
		return;
	}
	
	/* Client init */
	char shared_flag;
	rc = recv_char(conn_sd, &shared_flag);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Failed receiving client init: %d", rc);
		return;
	}
	
	/* Server init */
	fibril_mutex_lock(&rfb->lock);
	size_t name_length = str_length(rfb->name);
	size_t msg_length = sizeof(rfb_server_init_t) + name_length;
	rfb_server_init_t *server_init = malloc(msg_length);
	if (server_init == NULL) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Cannot allocate memory for server init");
		fibril_mutex_unlock(&rfb->lock);
		return;
	}
	server_init->width = rfb->width;
	server_init->height = rfb->height;
	server_init->pixel_format = rfb->pixel_format,
	server_init->name_length = name_length;
	rfb_server_init_to_be(server_init, server_init);
	memcpy(server_init->name, rfb->name, name_length);
	fibril_mutex_unlock(&rfb->lock);
	rc = send(conn_sd, server_init, msg_length, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "Failed sending server init: %d", rc);
		return;
	}
	
	while (true) {
		char message_type = 0;
		rc = recv_char(conn_sd, &message_type);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_WARN,
			    "Failed receiving client message type");
			return;
		}
		
		rfb_set_pixel_format_t spf;
		rfb_set_encodings_t se;
		rfb_framebuffer_update_request_t fbur;
		rfb_key_event_t ke;
		rfb_pointer_event_t pe;
		rfb_client_cut_text_t cct;
		switch (message_type) {
		case RFB_CMSG_SET_PIXEL_FORMAT:
			recv_message(conn_sd, message_type, &spf, sizeof(spf));
			rfb_pixel_format_to_host(&spf.pixel_format, &spf.pixel_format);
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Received SetPixelFormat message");
			fibril_mutex_lock(&rfb->lock);
			rc = rfb_set_pixel_format(rfb, &spf.pixel_format);
			fibril_mutex_unlock(&rfb->lock);
			if (rc != EOK)
				return;
			break;
		case RFB_CMSG_SET_ENCODINGS:
			recv_message(conn_sd, message_type, &se, sizeof(se));
			rfb_set_encodings_to_host(&se, &se);
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Received SetEncodings message");
			for (uint16_t i = 0; i < se.count; i++) {
				int32_t encoding = 0;
				rc = recv_chars(conn_sd, (char *) &encoding, sizeof(int32_t));
				if (rc != EOK)
					return;
				encoding = uint32_t_be2host(encoding);
				if (encoding == RFB_ENCODING_TRLE) {
					log_msg(LOG_DEFAULT, LVL_DEBUG,
					    "Client supports TRLE encoding");
					rfb->supports_trle = true;
				}
			}
			break;
		case RFB_CMSG_FRAMEBUFFER_UPDATE_REQUEST:
			recv_message(conn_sd, message_type, &fbur, sizeof(fbur));
			rfb_framebuffer_update_request_to_host(&fbur, &fbur);
			log_msg(LOG_DEFAULT, LVL_DEBUG2,
			    "Received FramebufferUpdateRequest message");
			rfb_send_framebuffer_update(rfb, conn_sd, fbur.incremental);
			break;
		case RFB_CMSG_KEY_EVENT:
			recv_message(conn_sd, message_type, &ke, sizeof(ke));
			rfb_key_event_to_host(&ke, &ke);
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Received KeyEvent message");
			break;
		case RFB_CMSG_POINTER_EVENT:
			recv_message(conn_sd, message_type, &pe, sizeof(pe));
			rfb_pointer_event_to_host(&pe, &pe);
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Received PointerEvent message");
			break;
		case RFB_CMSG_CLIENT_CUT_TEXT:
			recv_message(conn_sd, message_type, &cct, sizeof(cct));
			rfb_client_cut_text_to_host(&cct, &cct);
			log_msg(LOG_DEFAULT, LVL_DEBUG2, "Received ClientCutText message");
			recv_skip_chars(conn_sd, cct.length);
			break;
		default:
			log_msg(LOG_DEFAULT, LVL_WARN,
			    "Invalid client message type encountered");
			return;
		}
	}
}

int rfb_listen(rfb_t *rfb, uint16_t port) {
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	int rc = inet_pton(AF_INET, "127.0.0.1", (void *)
	    &addr.sin_addr.s_addr);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error parsing network address (%s)",
		    str_error(rc));
		return rc;
	}

	int listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error creating listening socket (%s)",
		    str_error(listen_sd));
		return rc;
	}
	
	rc = bind(listen_sd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error binding socket (%s)",
		    str_error(rc));
		return rc;
	}
	
	rc = listen(listen_sd, 2);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "listen() failed (%s)", str_error(rc));
		return rc;
	}
	
	rfb->listen_sd = listen_sd;
	
	return EOK;
}

void rfb_accept(rfb_t *rfb)
{
	while (true) {
		struct sockaddr_in raddr;
		socklen_t raddr_len = sizeof(raddr);
		int conn_sd = accept(rfb->listen_sd, (struct sockaddr *) &raddr,
		    &raddr_len);
		
		if (conn_sd < 0) {
			log_msg(LOG_DEFAULT, LVL_WARN, "accept() failed (%s)",
			    str_error(conn_sd));
			continue;
		}
		
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Connection accepted");
		
		rbuf_out = 0;
		rbuf_in = 0;
		
		rfb_socket_connection(rfb, conn_sd);
		closesocket(conn_sd);
	}
}
