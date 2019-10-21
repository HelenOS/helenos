/*
 * Copyright (c) 2019 Jiri Svoboda
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

#include <gfx/coord.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(coord);

extern void gfx_coord2_add(gfx_coord2_t *, gfx_coord2_t *, gfx_coord2_t *);
extern void gfx_coord2_subtract(gfx_coord2_t *, gfx_coord2_t *, gfx_coord2_t *);
extern void gfx_rect_translate(gfx_coord2_t *, gfx_rect_t *, gfx_rect_t *);

/** gfx_coord2_add should add two coordinate vectors */
PCUT_TEST(coord2_add)
{
	gfx_coord2_t a, b;
	gfx_coord2_t d;

	a.x = 10;
	a.y = 11;
	b.x = 20;
	b.y = 22;

	gfx_coord2_add(&a, &b, &d);

	PCUT_ASSERT_EQUALS(a.x + b.x, d.x);
	PCUT_ASSERT_EQUALS(a.y + b.y, d.y);
}

/** gfx_coord2_subtract should subtract two coordinate vectors */
PCUT_TEST(coord2_subtract)
{
	gfx_coord2_t a, b;
	gfx_coord2_t d;

	a.x = 10;
	a.y = 11;
	b.x = 20;
	b.y = 22;

	gfx_coord2_subtract(&a, &b, &d);

	PCUT_ASSERT_EQUALS(a.x - b.x, d.x);
	PCUT_ASSERT_EQUALS(a.y - b.y, d.y);
}

/** gfx_rect_translate should translate rectangle */
PCUT_TEST(rect_translate)
{
	gfx_coord2_t offs;
	gfx_rect_t srect;
	gfx_rect_t drect;

	offs.x = 5;
	offs.y = 6;

	srect.p0.x = 10;
	srect.p0.y = 11;
	srect.p1.x = 20;
	srect.p1.y = 22;

	gfx_rect_translate(&offs, &srect, &drect);

	PCUT_ASSERT_EQUALS(offs.x + srect.p0.x, drect.p0.x);
	PCUT_ASSERT_EQUALS(offs.y + srect.p0.y, drect.p0.y);
	PCUT_ASSERT_EQUALS(offs.x + srect.p1.x, drect.p1.x);
	PCUT_ASSERT_EQUALS(offs.y + srect.p1.y, drect.p1.y);
}

PCUT_EXPORT(coord);
