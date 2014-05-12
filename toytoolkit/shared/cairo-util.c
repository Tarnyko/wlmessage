/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2012 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

//#include "config.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <cairo.h>
#include "cairo-util.h"

#include "image-loader.h"
//#include "config-parser.h"

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

void
surface_flush_device(cairo_surface_t *surface)
{
	cairo_device_t *device;

	device = cairo_surface_get_device(surface);
	if (device)
		cairo_device_flush(device);
}

static int
blur_surface(cairo_surface_t *surface, int margin)
{
	int32_t width, height, stride, x, y, z, w;
	uint8_t *src, *dst;
	uint32_t *s, *d, a, p;
	int i, j, k, size, half;
	uint32_t kernel[71];
	double f;

	size = ARRAY_LENGTH(kernel);
	width = cairo_image_surface_get_width(surface);
	height = cairo_image_surface_get_height(surface);
	stride = cairo_image_surface_get_stride(surface);
	src = cairo_image_surface_get_data(surface);

	dst = malloc(height * stride);
	if (dst == NULL)
		return -1;

	half = size / 2;
	a = 0;
	for (i = 0; i < size; i++) {
		f = (i - half);
		kernel[i] = exp(- f * f / ARRAY_LENGTH(kernel)) * 10000;
		a += kernel[i];
	}

	for (i = 0; i < height; i++) {
		s = (uint32_t *) (src + i * stride);
		d = (uint32_t *) (dst + i * stride);
		for (j = 0; j < width; j++) {
			if (margin < j && j < width - margin) {
				d[j] = s[j];
				continue;
			}

			x = 0;
			y = 0;
			z = 0;
			w = 0;
			for (k = 0; k < size; k++) {
				if (j - half + k < 0 || j - half + k >= width)
					continue;
				p = s[j - half + k];

				x += (p >> 24) * kernel[k];
				y += ((p >> 16) & 0xff) * kernel[k];
				z += ((p >> 8) & 0xff) * kernel[k];
				w += (p & 0xff) * kernel[k];
			}
			d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
		}
	}

	for (i = 0; i < height; i++) {
		s = (uint32_t *) (dst + i * stride);
		d = (uint32_t *) (src + i * stride);
		for (j = 0; j < width; j++) {
			if (margin <= i && i < height - margin) {
				d[j] = s[j];
				continue;
			}

			x = 0;
			y = 0;
			z = 0;
			w = 0;
			for (k = 0; k < size; k++) {
				if (i - half + k < 0 || i - half + k >= height)
					continue;
				s = (uint32_t *) (dst + (i - half + k) * stride);
				p = s[j];

				x += (p >> 24) * kernel[k];
				y += ((p >> 16) & 0xff) * kernel[k];
				z += ((p >> 8) & 0xff) * kernel[k];
				w += (p & 0xff) * kernel[k];
			}
			d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
		}
	}

	free(dst);
	cairo_surface_mark_dirty(surface);

	return 0;
}

void
tile_mask(cairo_t *cr, cairo_surface_t *surface,
	  int x, int y, int width, int height, int margin, int top_margin)
{
	cairo_pattern_t *pattern;
	cairo_matrix_t matrix;
	int i, fx, fy, vmargin;

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	pattern = cairo_pattern_create_for_surface (surface);
	cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);

	for (i = 0; i < 4; i++) {
		fx = i & 1;
		fy = i >> 1;

		cairo_matrix_init_translate(&matrix,
					    -x + fx * (128 - width),
					    -y + fy * (128 - height));
		cairo_pattern_set_matrix(pattern, &matrix);

		if (fy)
			vmargin = margin;
		else
			vmargin = top_margin;

		cairo_reset_clip(cr);
		cairo_rectangle(cr,
				x + fx * (width - margin),
				y + fy * (height - vmargin),
				margin, vmargin);
		cairo_clip (cr);
		cairo_mask(cr, pattern);
	}

	/* Top stretch */
	cairo_matrix_init_translate(&matrix, 60, 0);
	cairo_matrix_scale(&matrix, 8.0 / width, 1);
	cairo_matrix_translate(&matrix, -x - width / 2, -y);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_rectangle(cr, x + margin, y, width - 2 * margin, margin);

	cairo_reset_clip(cr);
	cairo_rectangle(cr,
			x + margin,
			y,
			width - 2 * margin, margin);
	cairo_clip (cr);
	cairo_mask(cr, pattern);

	/* Bottom stretch */
	cairo_matrix_translate(&matrix, 0, -height + 128);
	cairo_pattern_set_matrix(pattern, &matrix);

	cairo_reset_clip(cr);
	cairo_rectangle(cr, x + margin, y + height - margin,
			width - 2 * margin, margin);
	cairo_clip (cr);
	cairo_mask(cr, pattern);

	/* Left stretch */
	cairo_matrix_init_translate(&matrix, 0, 60);
	cairo_matrix_scale(&matrix, 1, 8.0 / height);
	cairo_matrix_translate(&matrix, -x, -y - height / 2);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_reset_clip(cr);
	cairo_rectangle(cr, x, y + margin, margin, height - 2 * margin);
	cairo_clip (cr);
	cairo_mask(cr, pattern);

	/* Right stretch */
	cairo_matrix_translate(&matrix, -width + 128, 0);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_rectangle(cr, x + width - margin, y + margin,
			margin, height - 2 * margin);
	cairo_reset_clip(cr);
	cairo_clip (cr);
	cairo_mask(cr, pattern);

	cairo_pattern_destroy(pattern);
	cairo_reset_clip(cr);
}

void
tile_source(cairo_t *cr, cairo_surface_t *surface,
	    int x, int y, int width, int height, int margin, int top_margin)
{
	cairo_pattern_t *pattern;
	cairo_matrix_t matrix;
	int i, fx, fy, vmargin;

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	pattern = cairo_pattern_create_for_surface (surface);
	cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
	cairo_set_source(cr, pattern);
	cairo_pattern_destroy(pattern);

	for (i = 0; i < 4; i++) {
		fx = i & 1;
		fy = i >> 1;

		cairo_matrix_init_translate(&matrix,
					    -x + fx * (128 - width),
					    -y + fy * (128 - height));
		cairo_pattern_set_matrix(pattern, &matrix);

		if (fy)
			vmargin = margin;
		else
			vmargin = top_margin;

		cairo_rectangle(cr,
				x + fx * (width - margin),
				y + fy * (height - vmargin),
				margin, vmargin);
		cairo_fill(cr);
	}

	/* Top stretch */
	cairo_matrix_init_translate(&matrix, 60, 0);
	cairo_matrix_scale(&matrix, 8.0 / (width - 2 * margin), 1);
	cairo_matrix_translate(&matrix, -x - width / 2, -y);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_rectangle(cr, x + margin, y, width - 2 * margin, top_margin);
	cairo_fill(cr);

	/* Bottom stretch */
	cairo_matrix_translate(&matrix, 0, -height + 128);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_rectangle(cr, x + margin, y + height - margin,
			width - 2 * margin, margin);
	cairo_fill(cr);

	/* Left stretch */
	cairo_matrix_init_translate(&matrix, 0, 60);
	cairo_matrix_scale(&matrix, 1, 8.0 / (height - margin - top_margin));
	cairo_matrix_translate(&matrix, -x, -y - height / 2);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_rectangle(cr, x, y + top_margin,
			margin, height - margin - top_margin);
	cairo_fill(cr);

	/* Right stretch */
	cairo_matrix_translate(&matrix, -width + 128, 0);
	cairo_pattern_set_matrix(pattern, &matrix);
	cairo_rectangle(cr, x + width - margin, y + top_margin,
			margin, height - margin - top_margin);
	cairo_fill(cr);
}

void
rounded_rect(cairo_t *cr, int x0, int y0, int x1, int y1, int radius)
{
	cairo_move_to(cr, x0, y0 + radius);
	cairo_arc(cr, x0 + radius, y0 + radius, radius, M_PI, 3 * M_PI / 2);
	cairo_line_to(cr, x1 - radius, y0);
	cairo_arc(cr, x1 - radius, y0 + radius, radius, 3 * M_PI / 2, 2 * M_PI);
	cairo_line_to(cr, x1, y1 - radius);
	cairo_arc(cr, x1 - radius, y1 - radius, radius, 0, M_PI / 2);
	cairo_line_to(cr, x0 + radius, y1);
	cairo_arc(cr, x0 + radius, y1 - radius, radius, M_PI / 2, M_PI);
	cairo_close_path(cr);
}

cairo_surface_t *
load_cairo_surface(const char *filename)
{
	pixman_image_t *image;
	int width, height, stride;
	void *data;

	image = load_image(filename);
	if (image == NULL) {
		return NULL;
	}

	data = pixman_image_get_data(image);
	width = pixman_image_get_width(image);
	height = pixman_image_get_height(image);
	stride = pixman_image_get_stride(image);

	return cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32,
						   width, height, stride);
}

void
theme_set_background_source(struct theme *t, cairo_t *cr, uint32_t flags)
{
	cairo_pattern_t *pattern;

	if (flags & THEME_FRAME_ACTIVE) {
		pattern = cairo_pattern_create_linear(16, 16, 16, 112);
		cairo_pattern_add_color_stop_rgb(pattern, 0.0, 1.0, 1.0, 1.0);
		cairo_pattern_add_color_stop_rgb(pattern, 0.2, 0.8, 0.8, 0.8);
		cairo_set_source(cr, pattern);
		cairo_pattern_destroy(pattern);
	} else {
		cairo_set_source_rgba(cr, 0.75, 0.75, 0.75, 1);
	}
}

struct theme *
theme_create(void)
{
	struct theme *t;
	cairo_t *cr;

	t = malloc(sizeof *t);
	if (t == NULL)
		return NULL;

	t->margin = 32;
	t->width = 6;
	t->titlebar_height = 27;
	t->frame_radius = 3;
	t->shadow = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 128, 128);
	cr = cairo_create(t->shadow);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	rounded_rect(cr, 32, 32, 96, 96, t->frame_radius);
	cairo_fill(cr);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
		goto err_shadow;
	cairo_destroy(cr);
	if (blur_surface(t->shadow, 64) == -1)
		goto err_shadow;

	t->active_frame =
		cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 128, 128);
	cr = cairo_create(t->active_frame);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	theme_set_background_source(t, cr, THEME_FRAME_ACTIVE);
	rounded_rect(cr, 0, 0, 128, 128, t->frame_radius);
	cairo_fill(cr);

	if (cairo_status(cr) != CAIRO_STATUS_SUCCESS)
		goto err_active_frame;

	cairo_destroy(cr);

	t->inactive_frame =
		cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 128, 128);
	cr = cairo_create(t->inactive_frame);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	theme_set_background_source(t, cr, 0);
	rounded_rect(cr, 0, 0, 128, 128, t->frame_radius);
	cairo_fill(cr);

	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
		goto err_inactive_frame;

	cairo_destroy(cr);

	return t;

 err_inactive_frame:
	cairo_surface_destroy(t->inactive_frame);
 err_active_frame:
	cairo_surface_destroy(t->active_frame);
 err_shadow:
	cairo_surface_destroy(t->shadow);
	free(t);
	return NULL;
}

void
theme_destroy(struct theme *t)
{
	cairo_surface_destroy(t->active_frame);
	cairo_surface_destroy(t->inactive_frame);
	cairo_surface_destroy(t->shadow);
	free(t);
}

void
theme_render_frame(struct theme *t,
		   cairo_t *cr, int width, int height,
		   const char *title, uint32_t flags)
{
	cairo_text_extents_t extents;
	cairo_font_extents_t font_extents;
	cairo_surface_t *source;
	int x, y, margin, top_margin;

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_paint(cr);

	if (flags & THEME_FRAME_MAXIMIZED)
		margin = 0;
	else {
		cairo_set_source_rgba(cr, 0, 0, 0, 0.45);
		tile_mask(cr, t->shadow,
			  2, 2, width + 8, height + 8,
			  64, 64);
		margin = t->margin;
	}

	if (flags & THEME_FRAME_ACTIVE)
		source = t->active_frame;
	else
		source = t->inactive_frame;

	if (title)
		top_margin = t->titlebar_height;
	else
		top_margin = t->width;

	tile_source(cr, source,
		    margin, margin,
		    width - margin * 2, height - margin * 2,
		    t->width, top_margin);

	if (title) {
		cairo_rectangle (cr, margin + t->width, margin,
				 width - (margin + t->width) * 2,
				 t->titlebar_height - t->width);
		cairo_clip(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_select_font_face(cr, "sans",
				       CAIRO_FONT_SLANT_NORMAL,
				       CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, 14);
		cairo_text_extents(cr, title, &extents);
		cairo_font_extents (cr, &font_extents);
		x = (width - extents.width) / 2;
		y = margin +
			(t->titlebar_height -
			 font_extents.ascent - font_extents.descent) / 2 +
			font_extents.ascent;

		if (flags & THEME_FRAME_ACTIVE) {
			cairo_move_to(cr, x + 1, y  + 1);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_show_text(cr, title);
			cairo_move_to(cr, x, y);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_show_text(cr, title);
		} else {
			cairo_move_to(cr, x, y);
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
			cairo_show_text(cr, title);
		}
	}
}

enum theme_location
theme_get_location(struct theme *t, int x, int y,
				int width, int height, int flags)
{
	int vlocation, hlocation, location;
	const int grip_size = 8;
	int margin, top_margin;

	margin = (flags & THEME_FRAME_MAXIMIZED) ? 0 : t->margin;

	if (flags & THEME_FRAME_NO_TITLE)
		top_margin = t->width;
	else
		top_margin = t->titlebar_height;

	if (x < margin)
		hlocation = THEME_LOCATION_EXTERIOR;
	else if (margin <= x && x < margin + grip_size)
		hlocation = THEME_LOCATION_RESIZING_LEFT;
	else if (x < width - margin - grip_size)
		hlocation = THEME_LOCATION_INTERIOR;
	else if (x < width - margin)
		hlocation = THEME_LOCATION_RESIZING_RIGHT;
	else
		hlocation = THEME_LOCATION_EXTERIOR;

	if (y < margin)
		vlocation = THEME_LOCATION_EXTERIOR;
	else if (margin <= y && y < margin + grip_size)
		vlocation = THEME_LOCATION_RESIZING_TOP;
	else if (y < height - margin - grip_size)
		vlocation = THEME_LOCATION_INTERIOR;
	else if (y < height - margin)
		vlocation = THEME_LOCATION_RESIZING_BOTTOM;
	else
		vlocation = THEME_LOCATION_EXTERIOR;

	location = vlocation | hlocation;
	if (location & THEME_LOCATION_EXTERIOR)
		location = THEME_LOCATION_EXTERIOR;
	if (location == THEME_LOCATION_INTERIOR &&
	    y < margin + top_margin)
		location = THEME_LOCATION_TITLEBAR;
	else if (location == THEME_LOCATION_INTERIOR)
		location = THEME_LOCATION_CLIENT_AREA;

	return location;
}
