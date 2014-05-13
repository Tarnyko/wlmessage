/* Copyright © 2014 Manuel Bachmann */

#include <string.h>
#include <glib.h>
#include <wayland-client.h>

#include "window.h"


struct message_window {
	struct window *window;
	struct widget *widget;
	cairo_surface_t *surface;

	char *title;
	char *message;
	int buttons_nb;
	struct wl_list button_list;
};

struct button {
	struct widget *widget;
	int focused, pressed;
	struct wl_list link;

	char *caption;
	int value;
};

struct message_window *message_window;


void
button_send_activate (int value)
{
	exit (value);
}

static void
button_click_handler(struct widget *widget,
		struct input *input, uint32_t time,
		uint32_t butt,
		enum wl_pointer_button_state state, void *data)
{
	struct button *button = data;

	widget_schedule_redraw (widget);

	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		button->pressed = 1;
	}
	else {
		button->pressed = 0;

		if (button->focused)
			button_send_activate (button->value);
	}
}

static int
button_enter_handler(struct widget *widget, struct input *input,
			     float x, float y, void *data)
{
	struct button *button = data;

	button->focused = 1;
	widget_schedule_redraw (widget);

	return CURSOR_LEFT_PTR;
}

static void
button_leave_handler(struct widget *widget,
			     struct input *input, void *data)
{
	struct button *button = data;

	button->focused = 0;
	widget_schedule_redraw (widget);
}

static void
button_redraw_handler (struct widget *widget, void *data)
{
	struct button *button = data;
	struct rectangle allocation;
	cairo_t *cr;
	cairo_text_extents_t extents;

	widget_get_allocation (widget, &allocation);
	if (button->pressed) {
		allocation.x++;
		allocation.y++;
	}

	cr = widget_cairo_create (message_window->widget);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	if (button->focused)
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	else
		cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
	cairo_fill (cr);
	cairo_set_line_width (cr, 3);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke_preserve(cr);
	cairo_select_font_face (cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 14);
	cairo_text_extents (cr, button->caption, &extents);
	cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
                           allocation.y + (allocation.height - extents.height)/2 + 10);
	cairo_show_text (cr, button->caption);
	cairo_destroy (cr);
}

static void
resize_handler (struct widget *widget, int32_t width, int32_t height, void *data)
{
	struct message_window *message_window = data;
	struct button *button;
	struct rectangle allocation;
	int x;

	widget_get_allocation (widget, &allocation);
	x = (allocation.width - message_window->buttons_nb*60)/2 + 10;

	wl_list_for_each (button, &message_window->button_list, link) {
		widget_set_allocation (button->widget, x, allocation.height-10, 60, 32); 
		x += 60+10;
	}
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

	gchar **messages = g_strsplit (message_window->message, "\n", 5);
	int lines = 0;
	while (messages[lines] != NULL)
		lines++;
		
	int i;
	for (i = 0; i < lines; i++) {
		cairo_text_extents (cr, messages[i], &extents);
		cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
	        	           allocation.y + (allocation.height - lines * extents.height)/2 + i*(extents.height+10));
		cairo_show_text (cr, messages[i]);
	}
	g_strfreev (messages);

	cairo_destroy (cr);
}

void
message_window_add_button (char *button_desc)
{
	struct button *button;

	gchar **desc = g_strsplit (button_desc, ":", 2);

	button = xzalloc (sizeof *button);
	button->widget = widget_add_widget (message_window->widget, button);
	button->caption = strdup (desc[0]);
	button->value = atoi (desc[1]);

	g_strfreev (desc);

	widget_set_redraw_handler (button->widget, button_redraw_handler);
	widget_set_enter_handler (button->widget, button_enter_handler);
	widget_set_leave_handler (button->widget, button_leave_handler);
	widget_set_button_handler (button->widget, button_click_handler);

	wl_list_insert (message_window->button_list.prev, &button->link);
}

void
message_window_create (struct display *display, char *message, char *title, char *buttons)
{
	int have_buttons = 0;

	message_window = xzalloc (sizeof *message_window);
	message_window->window = window_create (display);
	message_window->widget = window_frame_create (message_window->window, message_window);

	message_window->message = message;

	if (title)
		message_window->title = title;
	else
		message_window->title = strdup ("wlmessage");
	window_set_title (message_window->window, message_window->title);

	message_window->buttons_nb = 0;
	wl_list_init (&message_window->button_list);
	if (buttons) {
		gchar **button_list = g_strsplit (buttons, ",", 3);
		while (button_list[message_window->buttons_nb] != NULL) {
			message_window_add_button (button_list[message_window->buttons_nb]);
			message_window->buttons_nb++;
		}
		g_strfreev (button_list);
		have_buttons = 1;
	}

	window_set_user_data (message_window->window, message_window);
	widget_set_redraw_handler (message_window->widget, redraw_handler);
	widget_set_resize_handler (message_window->widget, resize_handler);

	widget_schedule_resize (message_window->widget, 480, 280 + have_buttons*32);
}

void
message_window_destroy ()
{
	if (message_window->surface)
		cairo_surface_destroy (message_window->surface);

	struct button *button, *tmp;
	wl_list_for_each_safe (button, tmp, &message_window->button_list, link) {
		wl_list_remove(&button->link);
		widget_destroy(button->widget);
		free (button->caption);
		free (button);
	}

	widget_destroy (message_window->widget);
	window_destroy (message_window->window);
	free (message_window->title);
	free (message_window->message);
	free (message_window);
}

void
wlmessage_run (char *message, char *title, char *buttons)
{
	struct display *display = NULL;

	display = display_create (NULL, NULL);
	if (!display) {
		fprintf (stderr, "Failed to connect to a Wayland compositor !\n");
		return;
	}
	
	message_window_create (display, message, title, buttons);
	display_run (display);

	message_window_destroy ();
	display_destroy (display);
}


char *
read_from_file (char *filename)
{
	FILE *file = NULL;
	char *text = NULL;
	int i, c;

	file = fopen (filename, "r");
	if (file) {
		i = 0;
		text = malloc (sizeof(char));
		while (c != EOF) {
			c = fgetc (file);
			if (c != EOF) {
				realloc (text, (i+1)*sizeof(char));
				text[i] = c;
				i++;
			}
		}
		realloc (text, (i+1)*sizeof(char));
		text[i] = '\0';
		fclose (file);
	}

	return text;
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
                        "    -buttons string\n"
                        "\n");
		return 0;
	}


	int i;
	char *message = NULL;
	char *title = NULL;
	char *buttons = NULL;

	for (i = 1; i < argc ; i++) {

		if (!strcmp (argv[i], "-title")) {
			if (argc >= i+2)
				title = strdup (argv[i+1]);
			i++; continue;
		}

		if (!strcmp (argv[i], "-file")) {
			if (argc >= i+2)
				message = read_from_file (argv[i+1]);
			i++; continue;
		}

		if (!strcmp (argv[i], "-buttons")) {
			if (argc >= i+2)
				buttons = strdup (argv[i+1]);
			i++; continue;
		}

		if (!message)
			message = strdup (argv[i]);
	}

	wlmessage_run (message, title, buttons);



	return 0;
}

