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

/** gfx_coord_div_rneg rounds towards negative numbers */
PCUT_TEST(coord_div_rneg)
{
	PCUT_ASSERT_INT_EQUALS(-3, gfx_coord_div_rneg(-7, 3));
	PCUT_ASSERT_INT_EQUALS(-2, gfx_coord_div_rneg(-6, 3));
	PCUT_ASSERT_INT_EQUALS(-2, gfx_coord_div_rneg(-5, 3));
	PCUT_ASSERT_INT_EQUALS(-2, gfx_coord_div_rneg(-4, 3));
	PCUT_ASSERT_INT_EQUALS(-1, gfx_coord_div_rneg(-3, 3));
	PCUT_ASSERT_INT_EQUALS(-1, gfx_coord_div_rneg(-2, 3));
	PCUT_ASSERT_INT_EQUALS(-1, gfx_coord_div_rneg(-1, 3));
	PCUT_ASSERT_INT_EQUALS(0, gfx_coord_div_rneg(0, 3));
	PCUT_ASSERT_INT_EQUALS(0, gfx_coord_div_rneg(1, 3));
	PCUT_ASSERT_INT_EQUALS(0, gfx_coord_div_rneg(2, 3));
	PCUT_ASSERT_INT_EQUALS(1, gfx_coord_div_rneg(3, 3));
	PCUT_ASSERT_INT_EQUALS(1, gfx_coord_div_rneg(4, 3));
	PCUT_ASSERT_INT_EQUALS(1, gfx_coord_div_rneg(5, 3));
	PCUT_ASSERT_INT_EQUALS(2, gfx_coord_div_rneg(6, 3));
}

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

	PCUT_ASSERT_INT_EQUALS(a.x + b.x, d.x);
	PCUT_ASSERT_INT_EQUALS(a.y + b.y, d.y);
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

	PCUT_ASSERT_INT_EQUALS(a.x - b.x, d.x);
	PCUT_ASSERT_INT_EQUALS(a.y - b.y, d.y);
}

/** gfx_coord2_clip with point to lower-left of clipping rectangle */
PCUT_TEST(coord2_clip_ll)
{
	gfx_coord2_t p;
	gfx_coord2_t cp;
	gfx_rect_t rect;

	p.x = 1;
	p.y = 2;

	rect.p0.x = 3;
	rect.p0.y = 4;
	rect.p1.x = 5;
	rect.p1.y = 6;

	gfx_coord2_clip(&p, &rect, &cp);

	PCUT_ASSERT_INT_EQUALS(3, cp.x);
	PCUT_ASSERT_INT_EQUALS(4, cp.y);
}

/** gfx_coord2_clip with point inside the clipping rectangle */
PCUT_TEST(coord2_clip_mm)
{
	gfx_coord2_t p;
	gfx_coord2_t cp;
	gfx_rect_t rect;

	p.x = 2;
	p.y = 3;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	gfx_coord2_clip(&p, &rect, &cp);

	PCUT_ASSERT_INT_EQUALS(2, cp.x);
	PCUT_ASSERT_INT_EQUALS(3, cp.y);
}

/** gfx_coord2_clip with point to upper-right of clipping rectangle */
PCUT_TEST(coord2_clip_hh)
{
	gfx_coord2_t p;
	gfx_coord2_t cp;
	gfx_rect_t rect;

	p.x = 5;
	p.y = 6;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	gfx_coord2_clip(&p, &rect, &cp);

	PCUT_ASSERT_INT_EQUALS(2, cp.x);
	PCUT_ASSERT_INT_EQUALS(3, cp.y);
}

/** gfx_coord2_project projects pixel from one rectangle to another  */
PCUT_TEST(coord2_project)
{
	gfx_coord2_t a, d;
	gfx_rect_t srect, drect;

	srect.p0.x = 10;
	srect.p0.y = 10;
	srect.p1.x = 20 + 1;
	srect.p1.y = 20 + 1;

	drect.p0.x = 100;
	drect.p0.y = 100;
	drect.p1.x = 200 + 1;
	drect.p1.y = 200 + 1;

	a.x = 10;
	a.y = 10;
	gfx_coord2_project(&a, &srect, &drect, &d);
	PCUT_ASSERT_INT_EQUALS(100, d.x);
	PCUT_ASSERT_INT_EQUALS(100, d.y);

	a.x = 15;
	a.y = 15;
	gfx_coord2_project(&a, &srect, &drect, &d);
	PCUT_ASSERT_INT_EQUALS(150, d.x);
	PCUT_ASSERT_INT_EQUALS(150, d.y);

	a.x = 12;
	a.y = 16;
	gfx_coord2_project(&a, &srect, &drect, &d);
	PCUT_ASSERT_INT_EQUALS(120, d.x);
	PCUT_ASSERT_INT_EQUALS(160, d.y);

	a.x = 20;
	a.y = 20;
	gfx_coord2_project(&a, &srect, &drect, &d);
	PCUT_ASSERT_INT_EQUALS(200, d.x);
	PCUT_ASSERT_INT_EQUALS(200, d.y);
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

	PCUT_ASSERT_INT_EQUALS(offs.x + srect.p0.x, drect.p0.x);
	PCUT_ASSERT_INT_EQUALS(offs.y + srect.p0.y, drect.p0.y);
	PCUT_ASSERT_INT_EQUALS(offs.x + srect.p1.x, drect.p1.x);
	PCUT_ASSERT_INT_EQUALS(offs.y + srect.p1.y, drect.p1.y);
}

/** gfx_rect_rtranslate should reverse-translate rectangle */
PCUT_TEST(rect_rtranslate)
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

	gfx_rect_rtranslate(&offs, &srect, &drect);

	PCUT_ASSERT_INT_EQUALS(srect.p0.x - offs.x, drect.p0.x);
	PCUT_ASSERT_INT_EQUALS(srect.p0.y - offs.y, drect.p0.y);
	PCUT_ASSERT_INT_EQUALS(srect.p1.x - offs.x, drect.p1.x);
	PCUT_ASSERT_INT_EQUALS(srect.p1.y - offs.y, drect.p1.y);
}

/** Sorting span with lower start and higher end point results in the same span. */
PCUT_TEST(span_points_sort_asc)
{
	gfx_coord_t a, b;

	gfx_span_points_sort(1, 2, &a, &b);
	PCUT_ASSERT_INT_EQUALS(1, a);
	PCUT_ASSERT_INT_EQUALS(2, b);
}

/** Sorting span with same start and end point results in the same span. */
PCUT_TEST(span_points_sort_equal)
{
	gfx_coord_t a, b;

	gfx_span_points_sort(1, 1, &a, &b);
	PCUT_ASSERT_INT_EQUALS(1, a);
	PCUT_ASSERT_INT_EQUALS(1, b);
}

/** Sorting span with hight start and lower end point results in transposed span. */
PCUT_TEST(span_points_sort_desc)
{
	gfx_coord_t a, b;

	gfx_span_points_sort(1, 0, &a, &b);
	PCUT_ASSERT_INT_EQUALS(1, a);
	PCUT_ASSERT_INT_EQUALS(2, b);
}

/** Rectangle envelope with first rectangle empty should return the second rectangle. */
PCUT_TEST(rect_envelope_a_empty)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t e;

	a.p0.x = 0;
	a.p0.y = 0;
	a.p1.x = 0;
	a.p1.y = 0;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	gfx_rect_envelope(&a, &b, &e);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, e.p0.y);
	PCUT_ASSERT_INT_EQUALS(3, e.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, e.p1.y);
}

/** Rectangle envelope with second rectangle empty should return the first rectangle. */
PCUT_TEST(rect_envelope_b_empty)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t e;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 0;
	b.p0.y = 0;
	b.p1.x = 0;
	b.p1.y = 0;

	gfx_rect_envelope(&a, &b, &e);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, e.p0.y);
	PCUT_ASSERT_INT_EQUALS(3, e.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, e.p1.y);
}

/** Rectangle envelope, a has both coordinates lower than b */
PCUT_TEST(rect_envelope_nonempty_a_lt_b)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t e;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 5;
	b.p0.y = 6;
	b.p1.x = 7;
	b.p1.y = 8;

	gfx_rect_envelope(&a, &b, &e);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, e.p0.y);
	PCUT_ASSERT_INT_EQUALS(7, e.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, e.p1.y);
}

/** Rectangle envelope, a has both coordinates higher than b */
PCUT_TEST(rect_envelope_nonempty_a_gt_b)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t e;

	a.p0.x = 5;
	a.p0.y = 6;
	a.p1.x = 7;
	a.p1.y = 8;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	gfx_rect_envelope(&a, &b, &e);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, e.p0.y);
	PCUT_ASSERT_INT_EQUALS(7, e.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, e.p1.y);
}

/** Rectangle envelope, a is inside b */
PCUT_TEST(rect_envelope_nonempty_a_inside_b)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t e;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 7;
	a.p1.y = 8;

	b.p0.x = 3;
	b.p0.y = 4;
	b.p1.x = 5;
	b.p1.y = 6;

	gfx_rect_envelope(&a, &b, &e);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, e.p0.y);
	PCUT_ASSERT_INT_EQUALS(7, e.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, e.p1.y);
}

/** Rectangle envelope, b is inside a */
PCUT_TEST(rect_envelope_nonempty_b_inside_a)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t e;

	a.p0.x = 3;
	a.p0.y = 4;
	a.p1.x = 5;
	a.p1.y = 6;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 7;
	b.p1.y = 8;

	gfx_rect_envelope(&a, &b, &e);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.x);
	PCUT_ASSERT_INT_EQUALS(2, e.p0.y);
	PCUT_ASSERT_INT_EQUALS(7, e.p1.x);
	PCUT_ASSERT_INT_EQUALS(8, e.p1.y);
}

/** Rectangle envelope, a and b cross */
PCUT_TEST(rect_envelope_nonempty_a_crosses_b)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t e;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 4;
	a.p1.y = 3;

	b.p0.x = 2;
	b.p0.y = 1;
	b.p1.x = 3;
	b.p1.y = 4;

	gfx_rect_envelope(&a, &b, &e);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.x);
	PCUT_ASSERT_INT_EQUALS(1, e.p0.y);
	PCUT_ASSERT_INT_EQUALS(4, e.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, e.p1.y);
}

/** Clip rectangle with rect completely inside the clipping rectangle */
PCUT_TEST(rect_clip_rect_inside)
{
	gfx_rect_t rect;
	gfx_rect_t clip;
	gfx_rect_t dest;

	rect.p0.x = 3;
	rect.p0.y = 4;
	rect.p1.x = 5;
	rect.p1.y = 6;

	clip.p0.x = 1;
	clip.p0.y = 2;
	clip.p1.x = 7;
	clip.p1.y = 8;

	gfx_rect_clip(&rect, &clip, &dest);
	PCUT_ASSERT_INT_EQUALS(3, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(4, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(5, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(6, dest.p1.y);
}

/** Clip rectangle with rect covering the clipping rectangle */
PCUT_TEST(rect_clip_rect_covering)
{
	gfx_rect_t rect;
	gfx_rect_t clip;
	gfx_rect_t dest;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 7;
	rect.p1.y = 8;

	clip.p0.x = 3;
	clip.p0.y = 4;
	clip.p1.x = 5;
	clip.p1.y = 6;

	gfx_rect_clip(&rect, &clip, &dest);
	PCUT_ASSERT_INT_EQUALS(3, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(4, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(5, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(6, dest.p1.y);
}

/** Clip rectangle with rect outside, having lower coordinates */
PCUT_TEST(rect_clip_rect_out_ll)
{
	gfx_rect_t rect;
	gfx_rect_t clip;
	gfx_rect_t dest;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	clip.p0.x = 5;
	clip.p0.y = 6;
	clip.p1.x = 7;
	clip.p1.y = 8;

	gfx_rect_clip(&rect, &clip, &dest);
	PCUT_ASSERT_INT_EQUALS(5, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(6, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(5, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(6, dest.p1.y);
}

/** Clip rectangle with rect outside, having higher coordinates */
PCUT_TEST(rect_clip_rect_out_hh)
{
	gfx_rect_t rect;
	gfx_rect_t clip;
	gfx_rect_t dest;

	rect.p0.x = 5;
	rect.p0.y = 6;
	rect.p1.x = 7;
	rect.p1.y = 8;

	clip.p0.x = 1;
	clip.p0.y = 2;
	clip.p1.x = 3;
	clip.p1.y = 4;

	gfx_rect_clip(&rect, &clip, &dest);
	PCUT_ASSERT_INT_EQUALS(3, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(4, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(3, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(4, dest.p1.y);
}

/** Clip rectangle with rect partially outside, having lower coordinates */
PCUT_TEST(rect_clip_rect_ll)
{
	gfx_rect_t rect;
	gfx_rect_t clip;
	gfx_rect_t dest;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 5;
	rect.p1.y = 6;

	clip.p0.x = 3;
	clip.p0.y = 4;
	clip.p1.x = 7;
	clip.p1.y = 8;

	gfx_rect_clip(&rect, &clip, &dest);
	PCUT_ASSERT_INT_EQUALS(3, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(4, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(5, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(6, dest.p1.y);
}

/** Clip rectangle with rect partially outside, having higher coordinates */
PCUT_TEST(rect_clip_rect_hh)
{
	gfx_rect_t rect;
	gfx_rect_t clip;
	gfx_rect_t dest;

	rect.p0.x = 3;
	rect.p0.y = 4;
	rect.p1.x = 7;
	rect.p1.y = 8;

	clip.p0.x = 1;
	clip.p0.y = 2;
	clip.p1.x = 5;
	clip.p1.y = 6;

	gfx_rect_clip(&rect, &clip, &dest);
	PCUT_ASSERT_INT_EQUALS(3, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(4, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(5, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(6, dest.p1.y);
}

/** Clip rectangle with no clipping rectangle */
PCUT_TEST(rect_clip_rect_noclip)
{
	gfx_rect_t rect;
	gfx_rect_t dest;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	gfx_rect_clip(&rect, NULL, &dest);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, dest.p1.y);
}

/** Center rectangle on rectangle */
PCUT_TEST(rect_ctr_on_rect)
{
	gfx_rect_t a;
	gfx_rect_t b;
	gfx_rect_t dest;

	/* Dimensions: 20 x 20 */
	b.p0.x = 10;
	b.p0.y = 20;
	b.p1.x = 30;
	b.p1.y = 40;

	/* Dimensions: 20 x 20 */
	a.p0.x = 100;
	a.p0.y = 200;
	a.p1.x = 120;
	a.p1.y = 220;

	/* Centering rectangle of same size should give us the same rectangle */
	gfx_rect_ctr_on_rect(&a, &b, &dest);
	PCUT_ASSERT_INT_EQUALS(b.p0.x, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(b.p0.y, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(b.p1.x, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(b.p1.y, dest.p1.y);

	/* Dimensions: 10 x 10 */
	a.p0.x = 100;
	a.p0.y = 200;
	a.p1.x = 110;
	a.p1.y = 210;

	gfx_rect_ctr_on_rect(&a, &b, &dest);
	PCUT_ASSERT_INT_EQUALS(15, dest.p0.x);
	PCUT_ASSERT_INT_EQUALS(25, dest.p0.y);
	PCUT_ASSERT_INT_EQUALS(25, dest.p1.x);
	PCUT_ASSERT_INT_EQUALS(35, dest.p1.y);
}

/** Sort span points that are already sorted should produde indentical points */
PCUT_TEST(rect_points_sort_sorted)
{
	gfx_coord_t s0, s1;

	gfx_span_points_sort(1, 2, &s0, &s1);
	PCUT_ASSERT_INT_EQUALS(1, s0);
	PCUT_ASSERT_INT_EQUALS(2, s1);
}

/** Sort span points that are reversed should transpose them */
PCUT_TEST(rect_points_sort_reversed)
{
	gfx_coord_t s0, s1;

	gfx_span_points_sort(2, 1, &s0, &s1);
	PCUT_ASSERT_INT_EQUALS(2, s0);
	PCUT_ASSERT_INT_EQUALS(3, s1);
}

/** Rectangle dimensions for straight rectangle are computed correctly */
PCUT_TEST(rect_dims_straight)
{
	gfx_rect_t rect;
	gfx_coord2_t dims;

	rect.p0.x = 1;
	rect.p0.y = 10;
	rect.p1.x = 100;
	rect.p1.y = 1000;

	gfx_rect_dims(&rect, &dims);

	PCUT_ASSERT_INT_EQUALS(99, dims.x);
	PCUT_ASSERT_INT_EQUALS(990, dims.y);
}

/** Rectangle dimensions for reversed rectangle are computed correctly */
PCUT_TEST(rect_dims_reversed)
{
	gfx_rect_t rect;
	gfx_coord2_t dims;

	rect.p0.x = 1000;
	rect.p0.y = 100;
	rect.p1.x = 10;
	rect.p1.y = 1;

	gfx_rect_dims(&rect, &dims);

	PCUT_ASSERT_INT_EQUALS(990, dims.x);
	PCUT_ASSERT_INT_EQUALS(99, dims.y);
}

/** gfx_rect_is_empty for straight rectangle with zero columns returns true */
PCUT_TEST(rect_is_empty_pos_x)
{
	gfx_rect_t rect;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 1;
	rect.p1.y = 3;
	PCUT_ASSERT_TRUE(gfx_rect_is_empty(&rect));
}

/** gfx_rect_is_empty for straight rectangle with zero rows returns true */
PCUT_TEST(rect_is_empty_pos_y)
{
	gfx_rect_t rect;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 2;
	rect.p1.y = 2;
	PCUT_ASSERT_TRUE(gfx_rect_is_empty(&rect));
}

/** gfx_rect_is_empty for straight non-empty rectangle returns false */
PCUT_TEST(rect_is_empty_neg)
{
	gfx_rect_t rect;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 2;
	rect.p1.y = 3;
	PCUT_ASSERT_FALSE(gfx_rect_is_empty(&rect));
}

/** gfx_rect_is_empty for reverse non-empty rectangle returns false */
PCUT_TEST(rect_is_empty_reverse_neg)
{
	gfx_rect_t rect;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 0;
	rect.p1.y = 1;
	PCUT_ASSERT_FALSE(gfx_rect_is_empty(&rect));
}

/** gfx_rect_is_incident for neighboring rectangles returns false */
PCUT_TEST(rect_is_incident_neighbor)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 3;
	b.p0.y = 2;
	b.p1.x = 5;
	b.p1.y = 6;

	PCUT_ASSERT_FALSE(gfx_rect_is_incident(&a, &b));
}

/** gfx_rect_is_incident for a inside b returns true */
PCUT_TEST(rect_is_incident_a_inside_b)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 2;
	a.p0.y = 3;
	a.p1.x = 4;
	a.p1.y = 5;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 5;
	b.p1.y = 6;

	PCUT_ASSERT_TRUE(gfx_rect_is_incident(&a, &b));
}

/** gfx_rect_is_incident for b inside a returns true */
PCUT_TEST(rect_is_incident_b_inside_a)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 5;
	a.p1.y = 6;

	b.p0.x = 2;
	b.p0.y = 3;
	b.p1.x = 4;
	b.p1.y = 5;

	PCUT_ASSERT_TRUE(gfx_rect_is_incident(&a, &b));
}

/** gfx_rect_is_incident for a and b sharing corner returns true */
PCUT_TEST(rect_is_incident_corner)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 2;
	b.p0.y = 3;
	b.p1.x = 4;
	b.p1.y = 5;

	PCUT_ASSERT_TRUE(gfx_rect_is_incident(&a, &b));
}

/** gfx_rect_is_incident for a == b returns true */
PCUT_TEST(rect_is_incident_same)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	PCUT_ASSERT_TRUE(gfx_rect_is_incident(&a, &b));
}

/** gfx_rect_is_inside is true for rectangle strictly inside */
PCUT_TEST(rect_is_inside_strict)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 2;
	a.p0.y = 3;
	a.p1.x = 4;
	a.p1.y = 5;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 5;
	b.p1.y = 6;

	PCUT_ASSERT_TRUE(gfx_rect_is_inside(&a, &b));
}

/** gfx_rect_is_inside is true for two equal rectangles */
PCUT_TEST(rect_is_inside_same)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	PCUT_ASSERT_TRUE(gfx_rect_is_inside(&a, &b));
}

/** gfx_rect_is_inside is false for @c a.p0 outside */
PCUT_TEST(rect_is_inside_p0_outside)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 0;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	PCUT_ASSERT_FALSE(gfx_rect_is_inside(&a, &b));

	a.p0.x = 1;
	a.p0.y = 1;
	a.p1.x = 3;
	a.p1.y = 4;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	PCUT_ASSERT_FALSE(gfx_rect_is_inside(&a, &b));
}

/** gfx_rect_is_inside is false for @c a.p1 outside */
PCUT_TEST(rect_is_inside_p1_outside)
{
	gfx_rect_t a;
	gfx_rect_t b;

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 4;
	a.p1.y = 4;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	PCUT_ASSERT_FALSE(gfx_rect_is_inside(&a, &b));

	a.p0.x = 1;
	a.p0.y = 2;
	a.p1.x = 3;
	a.p1.y = 5;

	b.p0.x = 1;
	b.p0.y = 2;
	b.p1.x = 3;
	b.p1.y = 4;

	PCUT_ASSERT_FALSE(gfx_rect_is_inside(&a, &b));
}

/** gfx_pix_inside_rect for  */
PCUT_TEST(pix_inside_rect)
{
	gfx_coord2_t coord;
	gfx_rect_t rect;

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	coord.x = 0;
	coord.y = 1;
	PCUT_ASSERT_FALSE(gfx_pix_inside_rect(&coord, &rect));

	coord.x = 1;
	coord.y = 1;
	PCUT_ASSERT_FALSE(gfx_pix_inside_rect(&coord, &rect));

	coord.x = 0;
	coord.y = 2;
	PCUT_ASSERT_FALSE(gfx_pix_inside_rect(&coord, &rect));

	coord.x = 1;
	coord.y = 2;
	PCUT_ASSERT_TRUE(gfx_pix_inside_rect(&coord, &rect));

	coord.x = 2;
	coord.y = 3;
	PCUT_ASSERT_TRUE(gfx_pix_inside_rect(&coord, &rect));

	coord.x = 3;
	coord.y = 3;
	PCUT_ASSERT_FALSE(gfx_pix_inside_rect(&coord, &rect));

	coord.x = 2;
	coord.y = 4;
	PCUT_ASSERT_FALSE(gfx_pix_inside_rect(&coord, &rect));

	coord.x = 3;
	coord.y = 4;
	PCUT_ASSERT_FALSE(gfx_pix_inside_rect(&coord, &rect));
}

PCUT_EXPORT(coord);
