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

#ifndef __SCREENBUFFER_H__
#define __SCREENBUFFER_H__


#define DEFAULT_FOREGROUND 0xffff00	/**< default console foreground color */
#define DEFAULT_BACKGROUND 0x000080	/**< default console background color */

typedef struct {
	unsigned int bg_color;		/**< background color */
	unsigned int fg_color;		/**< foreground color */
} style_t;

/** One field on screen. It contain one character and its attributes. */
typedef struct {
	char character;			/**< Character itself */
	style_t style;			/**< Character`s attributes */
} keyfield_t;

/** Structure for buffering state of one virtual console.
 */
typedef struct {
	keyfield_t *buffer;			/**< Screen content - characters and its style. Used as cyclyc buffer. */
	unsigned int size_x, size_y;		/**< Number of columns and rows */
	unsigned int position_x, position_y;	/**< Coordinates of last printed character for determining cursor position */
	style_t style;				/**< Current style */
	unsigned int top_line;			/**< Points to buffer[][] line that will be printed at screen as the first line */
} screenbuffer_t;

/** Returns keyfield for position on screen. Screenbuffer->buffer is cyclic buffer so we must couted in index of the topmost line.
 * @param scr	screenbuffer
 * @oaram x	position on screen
 * @param y	position on screen
 * @return	keyfield structure with character and its attributes on x,y
 */
static inline keyfield_t *get_field_at(screenbuffer_t *scr, unsigned int x, unsigned int y) 
{
	return scr->buffer + x + ((y + scr->top_line) % scr->size_y) * scr->size_x;
}

/** Compares two styles.
 * @param s1 first style
 * @param s2 second style
 * @return nonzero on equality
 */
static inline int style_same(style_t s1, style_t s2)
{
	return s1.fg_color == s2.fg_color && s1.bg_color == s2.bg_color;
}


void screenbuffer_putchar(screenbuffer_t *scr, char c);
screenbuffer_t *screenbuffer_init(screenbuffer_t *scr, int size_x, int size_y);

void screenbuffer_clear(screenbuffer_t *scr);
void screenbuffer_clear_line(screenbuffer_t *scr, unsigned int line);
void screenbuffer_copy_buffer(screenbuffer_t *scr, keyfield_t *dest);
void screenbuffer_goto(screenbuffer_t *scr, unsigned int x, unsigned int y);
void screenbuffer_set_style(screenbuffer_t *scr, unsigned int fg_color, unsigned int bg_color);

#endif

