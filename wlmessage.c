/* Copyright © 2014 Manuel Bachmann */

#include <string.h>
#include <glib.h>
#include <wayland-client.h>

#include "window.h"
#define MAX_LINES 6


struct message_window {
	struct window *window;
	struct widget *widget;
	cairo_surface_t *surface;

	char *message;
	char *title;
	cairo_surface_t *icon;
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


int
get_number_of_lines (char *text)
{
	int lines_num = 0;

	gchar **lines = g_strsplit (text, "\n", -1);

	while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES))
		lines_num++;

	g_strfreev (lines);

	return lines_num;
}

int
get_max_length_of_lines (char *text)
{
	int lines_num = 0;
	int length = 0;

	gchar **lines = g_strsplit (text, "\n", -1);

	while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES)) {
		if (strlen (lines[lines_num]) > length)
			length = strlen (lines[lines_num]);
		lines_num++;
	}

	g_strfreev (lines);

	return length;
}

char **
get_lines (char *text)
{
	gchar **lines = g_strsplit (text, "\n", -1);

	return lines;
}


void message_window_destroy ();

void
button_send_activate (int value)
{
	message_window_destroy ();
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
	} else {
		button->pressed = 0;
		button_send_activate (button->value);
	}
}

static void
button_touch_handler(struct widget *widget, struct input *input,
		 uint32_t serial, uint32_t time, int32_t id,
		 float tx, float ty, void *data)
{
	struct button *button = data;

	widget_schedule_redraw (widget);

	button->pressed = 1;
	button_send_activate (button->value);
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
	cairo_set_line_width (cr, 1);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
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
	int buttons_width, extended_width;
	int x;

	widget_get_allocation (widget, &allocation);

	buttons_width = 0;
	wl_list_for_each (button, &message_window->button_list, link) {
		extended_width = strlen(button->caption) - 5;
		if (extended_width < 0) extended_width = 0;
		buttons_width += 60 + extended_width*10;
	}

	x = allocation.x + (width - buttons_width)/2
	                 - (message_window->buttons_nb-1)*10;

	wl_list_for_each (button, &message_window->button_list, link) {
		extended_width = strlen(button->caption) - 5;
		if (extended_width < 0) extended_width = 0;
		widget_set_allocation (button->widget, x, allocation.y + height - 16 - 32,
		                                       60 + extended_width*10, 32); 
		x += 60 + extended_width*10 + 10;
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
	int lines_nb;
	char **lines;

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

	if (message_window->icon) {
			cairo_set_source_surface (cr, message_window->icon,
			                              allocation.x + (allocation.width - 64.0)/2,
			                              allocation.y + 10);
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			cairo_paint (cr);
			cairo_set_source_surface (cr, surface, 0.0, 0.0);
	}

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
	cairo_select_font_face (cr, "sans",
	                        CAIRO_FONT_SLANT_NORMAL,
	                        CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 18);

	lines_nb = get_number_of_lines (message_window->message);
	lines = get_lines (message_window->message);

	int i;
	for (i = 0; i < lines_nb; i++) {
		cairo_text_extents (cr, lines[i], &extents);
		cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
	        	           allocation.y + (allocation.height - lines_nb * extents.height)/2
		                                + i*(extents.height+10)
		                                + (!message_window->icon ? 0 : 32)
                                                - (!message_window->buttons_nb ? 0 : 32));
		cairo_show_text (cr, lines[i]);
	}

	g_strfreev (lines);

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
	widget_set_touch_down_handler (button->widget, button_touch_handler);

	wl_list_insert (message_window->button_list.prev, &button->link);
}

void
message_window_create (struct display *display, char *message, char *title, char *titlebuttons, int noresize, char *buttons, char *icon)
{
	int frame_type = FRAME_ALL;
	int extended_width = 0;
	int lines_nb = 0;

	if (titlebuttons) {
		frame_type = FRAME_NONE;
		if (strstr (titlebuttons, "Minimize"))
			frame_type = frame_type | FRAME_MINIMIZE;
		if (strstr (titlebuttons, "Maximize"))
			frame_type = frame_type | FRAME_MAXIMIZE;
		if (strstr (titlebuttons, "Close"))
			frame_type = frame_type | FRAME_CLOSE;
	}

	message_window = xzalloc (sizeof *message_window);
	message_window->window = window_create (display);
	message_window->widget = window_frame_create (message_window->window, frame_type, !noresize,  message_window);

	message_window->message = message;

	if (title)
		message_window->title = strdup (title);
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
	}

	if (icon) {
		cairo_surface_t *icon_temp = cairo_image_surface_create_from_png (icon);
		cairo_status_t status = cairo_surface_status (icon_temp);
		if (status == CAIRO_STATUS_SUCCESS) {
			message_window->icon = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
			cairo_t *icon_cr = cairo_create (message_window->icon);
			 /* rescale to 64x64 */
			int width = cairo_image_surface_get_width (icon_temp);
			int height = cairo_image_surface_get_height (icon_temp);
			if (width != height != 64) {
				double ratio = ((64.0/width) < (64.0/height) ? (64.0/width) : (64.0/height));
				cairo_scale (icon_cr, ratio, ratio);
			}
			cairo_set_source_surface (icon_cr, icon_temp, 0.0, 0.0);
			cairo_paint (icon_cr);
			cairo_destroy (icon_cr);
			cairo_surface_destroy (icon_temp);
		}
	}
	else {
		message_window->icon = NULL;
	}

	extended_width = (get_max_length_of_lines (message)) - 35;
	 if (extended_width < 0) extended_width = 0;
	lines_nb = get_number_of_lines (message);

	window_set_user_data (message_window->window, message_window);
	widget_set_redraw_handler (message_window->widget, redraw_handler);
	widget_set_resize_handler (message_window->widget, resize_handler);

	window_schedule_resize (message_window->window,
	                        480 + extended_width*10,
	                        280 + lines_nb*16 + (!message_window->buttons_nb ? 0 : 1)*32);
}

void
message_window_destroy ()
{
	if (message_window->surface)
		cairo_surface_destroy (message_window->surface);

	if (message_window->icon)
		cairo_surface_destroy (message_window->icon);

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
wlmessage_run (char *message, char *title, char *titlebuttons, int noresize, char *buttons, char *icon)
{
	struct display *display = NULL;

	display = display_create (NULL, NULL);
	if (!display) {
		fprintf (stderr, "Failed to connect to a Wayland compositor !\n");
		return;
	}
	
	message_window_create (display, message, title, titlebuttons, noresize, buttons, icon);
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
                        "    -file filename\n"
                        "    -buttons string\n"
                        "    -timeout secs\n"
                        "    -title title\n"
                        "    -titlebuttons string\n"
                        "    -no-resize\n"
                        "    -icon filename\n"
                        "\n");
		return 0;
	}


	int i;
	char *message = NULL;
	char *buttons = NULL;
	char *title = NULL;
	char *titlebuttons = NULL;
	char *icon = NULL;
	int timeout = 0;
	int noresize = 0;

	for (i = 1; i < argc ; i++) {

		if (!strcmp (argv[i], "-file")) {
			if (argc >= i+2)
				message = read_from_file (argv[i+1]);
			i++; continue;
		}

		if (!strcmp (argv[i], "-buttons")) {
			if (argc >= i+2)
				buttons = argv[i+1];
			i++; continue;
		}

		if (!strcmp (argv[i], "-title")) {
			if (argc >= i+2)
				title = argv[i+1];
			i++; continue;
		}

		if (!strcmp (argv[i], "-titlebuttons")) {
			if (argc >= i+2)
				titlebuttons = argv[i+1];
			i++; continue;
		}

		if (!strcmp (argv[i], "-no-resize")) {
			noresize = 1;
			continue;
		}

		if (!strcmp (argv[i], "-icon")) {
			if (argc >= i+2)
				icon = argv[i+1];
			i++; continue;
		}

		if (!strcmp (argv[i], "-timeout")) {
			if (argc >= i+2)
				timeout = atoi (argv[i+1]);
			i++; continue;
		}

		if (!message)
			message = strdup (argv[i]);
	}

	wlmessage_run (message, title, titlebuttons, noresize, buttons, icon);



	return 0;
}

