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
		if (rc <= 0) {
			fprintf(stderr, "recv() failed (%zd)\n", rc);
			return rc;
		}
		
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

static void rfb_encode_pixel(void *buf, rfb_pixel_format_t *pf, uint8_t r, uint8_t g,
    uint8_t b)
{
	uint32_t pix = 0;
	pix |= rfb_scale_channel(r, pf->r_max) << pf->r_shift;
	pix |= rfb_scale_channel(g, pf->g_max) << pf->g_shift;
	pix |= rfb_scale_channel(b, pf->b_max) << pf->b_shift;
	
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

static int rfb_send_framebuffer_update(rfb_t *rfb, int conn_sd, bool incremental)
{
	if (!incremental || !rfb->damage_valid) {
		rfb->damage_rect.x = 0;
		rfb->damage_rect.y = 0;
		rfb->damage_rect.width = rfb->width;
		rfb->damage_rect.height = rfb->height;
	}
	

	/* We send only single raw rectangle right now */
	size_t buf_size = sizeof(rfb_framebuffer_update_t) +
	    sizeof(rfb_rectangle_t) * 1 +
	    (rfb->damage_rect.width * rfb->damage_rect.height * (rfb->pixel_format.bpp/8));
	
	void *buf = malloc(buf_size);
	if (buf == NULL)
		return ENOMEM;
	memset(buf, 0, buf_size);
	
	void *pos = buf;
	rfb_framebuffer_update_t *fbu = buf;
	fbu->message_type = RFB_SMSG_FRAMEBUFFER_UPDATE;
	fbu->rect_count = 1;
	rfb_framebuffer_update_to_be(fbu, fbu);
	pos += sizeof(rfb_framebuffer_update_t);
	
	rfb_rectangle_t *rect = pos;
	*rect = rfb->damage_rect;
	rect->enctype = RFB_ENCODING_RAW;
	rfb_rectangle_to_be(rect, rect);
	pos += sizeof(rfb_rectangle_t);
	
	size_t pixel_size = rfb->pixel_format.bpp / 8;
	
	for (uint16_t y = 0; y < rfb->damage_rect.height; y++) {
		for (uint16_t x = 0; x < rfb->damage_rect.width; x++) {
			pixel_t pixel = pixelmap_get_pixel(&rfb->framebuffer,
			    x + rfb->damage_rect.x, y + rfb->damage_rect.y);
			rfb_encode_pixel(pos, &rfb->pixel_format, RED(pixel), GREEN(pixel), BLUE(pixel));
			pos += pixel_size;
		}
	}
	
	int rc = send(conn_sd, buf, buf_size, 0);
	free(buf);
	
	rfb->damage_valid = false;
	return rc;
}

static void rfb_socket_connection(rfb_t *rfb, int conn_sd)
{
	/* Version handshake */
	int rc = send(conn_sd, "RFB 003.008\n", 12, 0);
	if (rc != EOK)
		return;
	
	char client_version[12];
	rc = recv_chars(conn_sd, client_version, 12);
	if (rc != EOK)
		return;
	
	if (memcmp(client_version, "RFB 003.008\n", 12) != 0)
		return;
	
	/* Security handshake
	 * 1 security type supported, which is 1 - None
	 */
	char sec_types[2];
	sec_types[0] = 1; /* length */
	sec_types[1] = RFB_SECURITY_NONE;
	rc = send(conn_sd, sec_types, 2, 0);
	if (rc != EOK)
		return;
	
	char selected_sec_type = 0;
	rc = recv_char(conn_sd, &selected_sec_type);
	if (rc != EOK)
		return;
	if (selected_sec_type != RFB_SECURITY_NONE)
		return;
	uint32_t security_result = RFB_SECURITY_HANDSHAKE_OK;
	rc = send(conn_sd, &security_result, sizeof(uint32_t), 0);
	if (rc != EOK)
		return;
	
	/* Client init */
	char shared_flag;
	rc = recv_char(conn_sd, &shared_flag);
	if (rc != EOK)
		return;
	
	/* Server init */
	fibril_mutex_lock(&rfb->lock);
	size_t name_length = str_length(rfb->name);
	size_t msg_length = sizeof(rfb_server_init_t) + name_length;
	rfb_server_init_t *server_init = malloc(msg_length);
	if (server_init == NULL) {
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
	if (rc != EOK)
		return;
	
	while (true) {
		char message_type = 0;
		rc = recv_char(conn_sd, &message_type);
		if (rc != EOK)
			return;
		
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
			fibril_mutex_lock(&rfb->lock);
			rfb->pixel_format = spf.pixel_format;
			fibril_mutex_unlock(&rfb->lock);
			printf("set pixel format\n");
			break;
		case RFB_CMSG_SET_ENCODINGS:
			recv_message(conn_sd, message_type, &se, sizeof(se));
			rfb_set_encodings_to_host(&se, &se);
			for (uint16_t i = 0; i < se.count; i++) {
				int32_t encoding = 0;
				rc = recv_chars(conn_sd, (char *) &encoding, sizeof(int32_t));
				if (rc != EOK)
					return;
				encoding = uint32_t_be2host(encoding);
			}
			printf("set encodings\n");
			break;
		case RFB_CMSG_FRAMEBUFFER_UPDATE_REQUEST:
			recv_message(conn_sd, message_type, &fbur, sizeof(fbur));
			rfb_framebuffer_update_request_to_host(&fbur, &fbur);
			printf("framebuffer update request\n");
			fibril_mutex_lock(&rfb->lock);
			rfb_send_framebuffer_update(rfb, conn_sd, fbur.incremental);
			fibril_mutex_unlock(&rfb->lock);
			break;
		case RFB_CMSG_KEY_EVENT:
			recv_message(conn_sd, message_type, &ke, sizeof(ke));
			rfb_key_event_to_host(&ke, &ke);
			break;
		case RFB_CMSG_POINTER_EVENT:
			recv_message(conn_sd, message_type, &pe, sizeof(pe));
			rfb_pointer_event_to_host(&pe, &pe);
			break;
		case RFB_CMSG_CLIENT_CUT_TEXT:
			recv_message(conn_sd, message_type, &cct, sizeof(cct));
			rfb_client_cut_text_to_host(&cct, &cct);
			recv_skip_chars(conn_sd, cct.length);
			break;
		default:
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
		fprintf(stderr, "Error parsing network address (%s)\n",
		    str_error(rc));
		return rc;
	}

	int listen_sd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_sd < 0) {
		fprintf(stderr, "Error creating listening socket (%s)\n",
		    str_error(listen_sd));
		return rc;
	}
	
	rc = bind(listen_sd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc != EOK) {
		fprintf(stderr, "Error binding socket (%s)\n",
		    str_error(rc));
		return rc;
	}
	
	rc = listen(listen_sd, 2);
	if (rc != EOK) {
		fprintf(stderr, "listen() failed (%s)\n", str_error(rc));
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
			fprintf(stderr, "accept() failed (%s)\n", str_error(conn_sd));
			continue;
		}
		
		rbuf_out = 0;
		rbuf_in = 0;
		
		rfb_socket_connection(rfb, conn_sd);
		closesocket(conn_sd);
	}
}
