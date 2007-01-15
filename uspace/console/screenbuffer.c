/*
 * Copyright (C) 2006 Josef Cejka
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

/** @addtogroup console
 * @{ 
 */
/** @file
 */

#include <screenbuffer.h>
#include <malloc.h>
#include <unistd.h>

/** Store one character to screenbuffer. Its position is determined by
 * scr->position_x and scr->position_y.
 *
 * @param scr	screenbuffer
 * @param c	stored character
 */
void screenbuffer_putchar(screenbuffer_t *scr, char c) 
{
	keyfield_t *field;
	
	field = get_field_at(scr, scr->position_x, scr->position_y);

	field->character = c;
	field->style = scr->style;
}

/** Initilize screenbuffer. Allocate space for screen content in accordance to given size.
 * @param scr		initialized screenbuffer
 * @param size_x	width in characters		
 * @param size_y	height in characters
 * @return pointer to screenbuffer (same as scr parameter) or NULL
 */
screenbuffer_t *screenbuffer_init(screenbuffer_t *scr, int size_x, int size_y) 
{
	scr->buffer = (keyfield_t *) malloc(sizeof(keyfield_t) * size_x * size_y);
	if (!scr->buffer) {
		return NULL;
	}
	
	scr->size_x = size_x;
	scr->size_y = size_y;
	scr->style.fg_color = DEFAULT_FOREGROUND;
	scr->style.bg_color = DEFAULT_BACKGROUND;
	scr->is_cursor_visible = 1;
	
	screenbuffer_clear(scr);
	
	return scr;
}

/** Clear screenbuffer. 
 * @param scr screenbuffer
 */
void screenbuffer_clear(screenbuffer_t *scr)
{
	unsigned int i;
	
	for (i = 0; i < (scr->size_x * scr->size_y); i++) {
		scr->buffer[i].character = ' ';
		scr->buffer[i].style = scr->style;
	}

	scr->top_line = 0;
	scr->position_y = 0;
	scr->position_x = 0;
}

/** Clear one buffer line.
 * @param scr
 * @param line One buffer line (not a screen line!)
 */
void screenbuffer_clear_line(screenbuffer_t *scr, unsigned int line)
{
	unsigned int i;
	
	for (i = 0; i < scr->size_x; i++) {
		scr->buffer[i + line * scr->size_x].character = ' ';
		scr->buffer[i + line * scr->size_x].style = scr->style;
	}
}

/** Copy content buffer from screenbuffer to given memory.
 * @param scr	source screenbuffer
 * @param dest  destination
 */
void screenbuffer_copy_buffer(screenbuffer_t *scr, keyfield_t *dest) 
{
	unsigned int i;
	
	for (i = 0; i < scr->size_x * scr->size_y; i++) {
		dest[i] = scr->buffer[i];
	}
}

/** Set new cursor position in screenbuffer.
 * @param scr
 * @param x
 * @param y
 */
void screenbuffer_goto(screenbuffer_t *scr, unsigned int x, unsigned int y)
{
	scr->position_x = x % scr->size_x;
	scr->position_y = y % scr->size_y;
}

/** Set new style.
 * @param scr
 * @param fg_color
 * @param bg_color
 */
void screenbuffer_set_style(screenbuffer_t *scr, unsigned int fg_color, unsigned int bg_color)
{
	scr->style.fg_color = fg_color;
	scr->style.bg_color = bg_color;
}

 
/** @}
 */
