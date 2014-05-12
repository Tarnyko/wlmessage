/* Copyright © 2014 Manuel Bachmann */

#include <string.h>
#include <wayland-client.h>

#include "window.h"


struct message_window {
	struct window *window;
	struct widget *widget;
	cairo_surface_t *surface;

	char *title;
	char *message;
};

struct message_window *message_window;


static void
resize_handler (struct widget *widget, int32_t width, int32_t height, void *data)
{
	struct message_window *message_window = data;
}

static void
redraw_handler (struct widget *widget, void *data)
{
	struct message_window *message_window = data;
	struct rectangle allocation;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_text_extents_t extents;

	widget_get_allocation (message_window->widget, &allocation);

	surface = window_get_surface (message_window->window);
	cr = cairo_create (surface);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 1.0);
	cairo_fill (cr);

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
	cairo_select_font_face (cr, "sans",
	                        CAIRO_FONT_SLANT_NORMAL,
	                        CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 18);
	cairo_text_extents (cr, message_window->message, &extents);
	cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
	                   allocation.y + (allocation.height - extents.height)/2);
	cairo_show_text (cr, message_window->message);
}

void
message_window_create (struct display *display, char *message, char *title)
{
	message_window = xzalloc (sizeof *message_window);
	message_window->window = window_create (display);
	message_window->widget = window_frame_create (message_window->window, message_window);

	message_window->message = message;

	if (title)
		message_window->title = title;
	else
		message_window->title = strdup ("wlmessage");
	window_set_title (message_window->window, message_window->title);

	window_set_user_data (message_window->window, message_window);
	widget_set_redraw_handler (message_window->widget, redraw_handler);
	widget_set_resize_handler (message_window->widget, resize_handler);

	widget_schedule_resize (message_window->widget, 480, 200);
}

void
message_window_destroy ()
{
	if (message_window->surface)
		cairo_surface_destroy (message_window->surface);

	widget_destroy (message_window->widget);
	window_destroy (message_window->window);
	free (message_window->title);
	free (message_window->message);
	free (message_window);
}

void
wlmessage_run (char *message, char *title)
{
	struct display *display = NULL;

	display = display_create (NULL, NULL);
	if (!display) {
		fprintf (stderr, "Failed to connect to a Wayland compositor !\n");
		return;
	}
	
	message_window_create (display, message, title);
	display_run (display);

	message_window_destroy ();
	display_destroy (display);
}


int
main (int argc, char *argv[])
{
	if (argc < 2) {
		printf ("usage: wlmessage [-options] [message ...]\n"
                        "\n"
                        "where options include:\n"
                        "    -title title\n"
                        "    -file filename\n"
                        "\n");
		return 0;
	}


	int i; char *message;
	char *title = NULL;

	for (i = 1; i < argc ; i++) {

		if (!strcmp (argv[i], "-title")) {
			if (argc >= i+2)
				title = strdup (argv[i+1]);
			i++; continue;
		}

		if (!strcmp (argv[i], "-file")) {
			if (argc >= i+2)
				printf ("MESSAGE FILE\n");
			i++; continue;
		}

		message = strdup (argv[i]);
	}

	wlmessage_run (message, title);



	return 0;
}

