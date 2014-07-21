/* Copyright © 2014 Manuel Bachmann */

#include <linux/input.h>
#include <string.h>
#include <glib.h>
#include <wayland-client.h>

#include "window.h"
#include "text-client-protocol.h"
#define MAX_LINES 6


struct message_window {
	struct window *window;
	struct widget *widget;
	cairo_surface_t *surface;

	char *message;
	char *title;
	cairo_surface_t *icon;
	struct entry *entry;
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

struct entry {
	struct widget *widget;
	int active;

	struct wl_text_input *text_input;
	char *text;
	int cursor_pos;
	int cursor_anchor;
	int last_vkb_len;
};

void message_window_destroy ();

struct message_window *message_window;
struct wl_text_input_manager *text_input_manager;
int default_value;


int
get_number_of_lines (char *text)
{
	int lines_num = 0;

	gchar **lines = g_strsplit (text, "\n", -1);

	if ( lines ) {
	  while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES))
	    lines_num++;

	  g_strfreev (lines);
	}

	return lines_num;
}

int
get_max_length_of_lines (char *text)
{
	int lines_num = 0;
	int length = 0;

	gchar **lines = g_strsplit (text, "\n", -1);
	
	if ( lines ) {
	  while ((lines[lines_num] != NULL) && (lines_num < MAX_LINES)) {
	    if (strlen (lines[lines_num]) > length)
	      length = strlen (lines[lines_num]);
	    lines_num++;
	  }
	  g_strfreev (lines);
	}

	return length;
}

char **
get_lines (char *text)
{
	gchar **lines = g_strsplit (text, "\n", -1);

	return lines;
}


static void
text_input_enter(void *data,
                 struct wl_text_input *text_input,
                 struct wl_surface *surface)
{
}

static void
text_input_leave(void *data,
                 struct wl_text_input *text_input)
{
}

static void
text_input_modifiers_map(void *data,
                         struct wl_text_input *text_input,
                         struct wl_array *map)
{
}

static void
text_input_input_panel_state(void *data,
                             struct wl_text_input *text_input,
                             uint32_t state)
{
}

static void
text_input_preedit_string(void *data,
                          struct wl_text_input *text_input,
                          uint32_t serial,
                          const char *text,
                          const char *commit)
{
	struct entry *entry = data;
	char *new_text;

	if (strlen(entry->text) >= 18)
		return;

	 /* workaround to prevent using Backspace for now */
	if (strlen(text) < entry->last_vkb_len) {
		entry->last_vkb_len = strlen(text);
		return;
	} else {
		entry->last_vkb_len = strlen(text);
	}

	new_text = malloc (strlen(entry->text) + 1 + 1);
	strncpy (new_text, entry->text, entry->cursor_pos);
	strcpy (new_text+entry->cursor_pos, text+(strlen(text)-1));
	strcpy (new_text+entry->cursor_pos+1, entry->text+entry->cursor_pos);
	free (entry->text);
	entry->text = new_text;
	entry->cursor_pos++;

	widget_schedule_redraw (entry->widget);
}

static void
text_input_preedit_styling(void *data,
                           struct wl_text_input *text_input,
                           uint32_t index,
                           uint32_t length,
                           uint32_t style)
{
}

static void
text_input_preedit_cursor(void *data,
                          struct wl_text_input *text_input,
                          int32_t index)
{
}

static void
text_input_commit_string(void *data,
                         struct wl_text_input *text_input,
                         uint32_t serial,
                         const char *text)
{
}

static void
text_input_cursor_position(void *data,
                           struct wl_text_input *text_input,
                           int32_t index,
                           int32_t anchor)
{
}

static void
text_input_keysym(void *data,
                  struct wl_text_input *text_input,
                  uint32_t serial,
                  uint32_t time,
                  uint32_t sym,
                  uint32_t state,
                  uint32_t modifiers)
{
	struct entry *entry = data;
	char *new_text;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	 /* use Tab as Backspace until I figure this out */
	if (sym == XKB_KEY_Tab) {
		if (entry->cursor_pos != 0) {
			new_text = malloc (strlen(entry->text));
			strncpy (new_text, entry->text, entry->cursor_pos - 1);
			strcpy (new_text+entry->cursor_pos-1, entry->text+entry->cursor_pos);
			free (entry->text);
			entry->text = new_text;
			entry->cursor_pos--;
		}
	}	

	if (sym == XKB_KEY_Left) {
		if (entry->cursor_pos != 0)
			entry->cursor_pos--;
	}

	if (sym == XKB_KEY_Right) {
		if (entry->cursor_pos != strlen (entry->text))
			entry->cursor_pos++;
	}

	if (sym == XKB_KEY_Return) {
		fprintf (stdout, entry->text);
		message_window_destroy ();
		exit (default_value);
	}

	widget_schedule_redraw (entry->widget);
}

static void
text_input_language(void *data,
                    struct wl_text_input *text_input,
                    uint32_t serial,
                    const char *language)
{
}

static void
text_input_text_direction(void *data,
                          struct wl_text_input *text_input,
                          uint32_t serial,
                          uint32_t direction)
{
}

static const struct wl_text_input_listener text_input_listener = {
	text_input_enter,
	text_input_leave,
	text_input_modifiers_map,
	text_input_input_panel_state,
	text_input_preedit_string,
	text_input_preedit_styling,
	text_input_preedit_cursor,
	text_input_commit_string,
	text_input_cursor_position,
	NULL,
	text_input_keysym,
	text_input_language,
	text_input_text_direction
};


void
button_send_activate (int value)
{
	if (message_window->entry)
		fprintf (stdout, message_window->entry->text);
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
button_touch_down_handler(struct widget *widget, struct input *input,
		 uint32_t serial, uint32_t time, int32_t id,
		 float tx, float ty, void *data)
{
	struct button *button = data;

	button->focused = 1;
	widget_schedule_redraw (widget);
}

static void
button_touch_up_handler(struct widget *widget, struct input *input,
		 uint32_t serial, uint32_t time, int32_t id,
		 void *data)
{
	struct button *button = data;

	button->focused = 0;
	widget_schedule_redraw (widget);

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
entry_click_handler(struct widget *widget,
		struct input *input, uint32_t time,
		uint32_t button,
		enum wl_pointer_button_state state, void *data)
{
	struct entry *entry = data;

	widget_schedule_redraw (widget);

	if (state == WL_POINTER_BUTTON_STATE_PRESSED && button == BTN_LEFT) {
		if (!entry->text_input) {
			entry->text_input = wl_text_input_manager_create_text_input (text_input_manager);
			wl_text_input_add_listener (entry->text_input, &text_input_listener, entry);
		}

		struct wl_seat *seat = input_get_seat (input);
		struct wl_surface *surface = window_get_wl_surface (message_window->window);
		wl_text_input_show_input_panel (entry->text_input);
		wl_text_input_activate (entry->text_input, seat, surface);

		entry->active = 1;
	}
}

static void
entry_touch_handler(struct widget *widget, struct input *input,
		 uint32_t serial, uint32_t time, int32_t id,
		 float tx, float ty, void *data)
{
	struct entry *entry = data;

	widget_schedule_redraw (widget);

	if (!entry->text_input) {
		entry->text_input = wl_text_input_manager_create_text_input (text_input_manager);
		wl_text_input_add_listener (entry->text_input, &text_input_listener, entry);
	}

	struct wl_seat *seat = input_get_seat (input);
	struct wl_surface *surface = window_get_wl_surface (message_window->window);
	wl_text_input_show_input_panel (entry->text_input);
	wl_text_input_activate (entry->text_input, seat, surface);

	entry->active = 1;
}

static int
entry_motion_handler(struct widget *widget,
		struct input *input, uint32_t time,
		float x, float y, void *data)
{
	return CURSOR_IBEAM;
}

static void
entry_redraw_handler (struct widget *widget, void *data)
{
	struct entry *entry = data;
	struct rectangle allocation;
	cairo_t *cr;
	cairo_text_extents_t extents;
	cairo_text_extents_t leftp_extents;
	char *leftp_text;
	int char_pos;

	widget_get_allocation (widget, &allocation);

	cr = widget_cairo_create (message_window->widget);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
	                allocation.x,
	                allocation.y,
	                allocation.width,
	                allocation.height);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_fill (cr);
	cairo_set_line_width (cr, 1);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);
	if (entry->active)
		cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
	else
		cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_stroke_preserve(cr);

	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
	cairo_select_font_face (cr, "sans",
	                        CAIRO_FONT_SLANT_NORMAL,
	                        CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 14);
	cairo_text_extents (cr, entry->text, &extents);
			char_pos = strlen(entry->text) -1;						/* for spaces at the end */
			while (char_pos >= 0 && entry->text[char_pos] == ' ') {
				extents.width += 5.0;
				char_pos--;
			}
	cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
	        	   allocation.y + (allocation.height - extents.height)/2 + 10);
	cairo_show_text (cr, entry->text);

	if (entry->active) {
		leftp_text = malloc (entry->cursor_pos + 1);
		strncpy (leftp_text, entry->text, entry->cursor_pos);
		leftp_text[entry->cursor_pos] = '\0';
		cairo_text_extents (cr, leftp_text, &leftp_extents);
			char_pos = strlen(leftp_text) -1;
			while (char_pos >= 0 && leftp_text[char_pos] == ' ') {
				leftp_extents.width += 5.0;
				char_pos--;
			}
		free (leftp_text);

		cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2 + leftp_extents.width,
		        	   allocation.y + (allocation.height - extents.height)/2 + 15);
		cairo_line_to(cr, allocation.x + (allocation.width - extents.width)/2 + leftp_extents.width,
		        	   allocation.y + (allocation.height - extents.height)/2 - 5);
		cairo_stroke(cr);
	}

	cairo_destroy (cr);
}

static void
resize_handler (struct widget *widget, int32_t width, int32_t height, void *data)
{
	struct message_window *message_window = data;
	struct entry *entry;
	struct button *button;
	struct rectangle allocation;
	int buttons_width, extended_width;
	int x;

	widget_get_allocation (widget, &allocation);

	x = allocation.x + (width - 240)/2;

	if (message_window->entry) {
		entry = message_window->entry;
		widget_set_allocation (entry->widget, x, allocation.y + height - 16*2 - 32*2,
		                                      240, 32);
	}

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
                                                - (!message_window->entry ? 0 : 32)
                                                - (!message_window->buttons_nb ? 0 : 32));
		cairo_show_text (cr, lines[i]);
	}

	g_strfreev (lines);

	cairo_destroy (cr);
}

static void
key_handler (struct window *window, struct input *input, uint32_t time,
		 uint32_t key, uint32_t sym, enum wl_keyboard_key_state state,
		 void *data)
{
	struct message_window *message_window = data;
	struct entry *entry = message_window->entry;
	char *new_text;
	char text[16];

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	if (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter) {
		if (entry)
			fprintf (stdout, entry->text);
		message_window_destroy ();
		exit (default_value);
	}

	if (entry && entry->active) {
		switch (sym) {
			case XKB_KEY_BackSpace:
				if (entry->cursor_pos == 0)
					break;
				new_text = malloc (strlen(entry->text));
				strncpy (new_text, entry->text, entry->cursor_pos - 1);
				strcpy (new_text+entry->cursor_pos-1, entry->text+entry->cursor_pos);
				free (entry->text);
				entry->text = new_text;
				entry->cursor_pos--;
				break;
			case XKB_KEY_Delete:
				if (entry->cursor_pos == strlen (entry->text))
					break;
				new_text = malloc (strlen(entry->text));
				strncpy (new_text, entry->text, entry->cursor_pos);
				strcpy (new_text+entry->cursor_pos, entry->text+entry->cursor_pos+1);
				free (entry->text);
				entry->text = new_text;
				break;
			case XKB_KEY_Left:
				if (entry->cursor_pos != 0)
					entry->cursor_pos--;
				break;
			case XKB_KEY_Right:
				if (entry->cursor_pos != strlen (entry->text))
					entry->cursor_pos++;
				break;
			case XKB_KEY_Tab:
				break;
			default:
				if (strlen(entry->text) >= 18)
					break;
				if (xkb_keysym_to_utf8 (sym, text, sizeof(text)) <= 0)
					break;
				if (strlen(text) > 1)	/* dismiss non-ASCII characters for now */
					break;
				new_text = malloc (strlen(entry->text) + strlen(text) + 1);
				strncpy (new_text, entry->text, entry->cursor_pos);
				strcpy (new_text+entry->cursor_pos, text);
				strcpy (new_text+entry->cursor_pos+strlen(text), entry->text+entry->cursor_pos);
				free (entry->text);
				entry->text = new_text;
				entry->cursor_pos++;
		}
		widget_schedule_redraw(entry->widget);
	}
}

void
message_window_add_entry (char *textfield)
{
	struct entry *entry;

	entry = xzalloc (sizeof *entry);
	entry->widget = widget_add_widget (message_window->widget, entry);
	entry->text = strdup (textfield);
	entry->cursor_pos = strlen (entry->text);
	entry->cursor_anchor = entry->cursor_pos;
	entry->last_vkb_len = 0;
	entry->active = 0;

	message_window->entry = entry;

	widget_set_redraw_handler (entry->widget, entry_redraw_handler);
	widget_set_motion_handler (entry->widget, entry_motion_handler);
	widget_set_button_handler (entry->widget, entry_click_handler);
	widget_set_touch_down_handler (entry->widget, entry_touch_handler);
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
	widget_set_touch_down_handler (button->widget, button_touch_down_handler);
	widget_set_touch_up_handler (button->widget, button_touch_up_handler);

	wl_list_insert (message_window->button_list.prev, &button->link);
}

void
message_window_create (struct display *display, char *message, char *title, char *titlebuttons, int noresize, char *buttons, char *icon, char *deflt, char *textfield)
{
	int frame_type = FRAME_ALL;
	int extended_width = 0;
	int lines_nb = 0;

	if (titlebuttons) {
		frame_type = FRAME_NONE;
		if (strstr (titlebuttons, "Min"))
			frame_type = frame_type | FRAME_MINIMIZE;
		if (strstr (titlebuttons, "Max"))
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

	default_value = 0;
	if (buttons && deflt) {
		struct button *button;
		wl_list_for_each (button, &message_window->button_list, link) {
			if (!strcmp(button->caption, deflt))
				default_value = button->value;
		}
	}

	if (textfield) {
		message_window_add_entry (textfield);
	} else {
		message_window->entry = NULL;
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
	window_set_key_handler (message_window->window, key_handler);
	widget_set_redraw_handler (message_window->widget, redraw_handler);
	widget_set_resize_handler (message_window->widget, resize_handler);

	window_schedule_resize (message_window->window,
	                        480 + extended_width*10,
	                        280 + lines_nb*16 + (!message_window->entry ? 0 : 1)*32
	                                          + (!message_window->buttons_nb ? 0 : 1)*32);
}

void
message_window_destroy ()
{
	if (message_window->surface)
		cairo_surface_destroy (message_window->surface);

	if (message_window->icon)
		cairo_surface_destroy (message_window->icon);

	struct entry *entry;
	if (message_window->entry) {
		entry = message_window->entry;
		if (entry->text_input)
			wl_text_input_destroy (entry->text_input);
		widget_destroy(entry->widget);
		free (entry->text);
		free (entry);
	}

	struct button *button, *tmp;
	wl_list_for_each_safe (button, tmp, &message_window->button_list, link) {
		wl_list_remove (&button->link);
		widget_destroy (button->widget);
		free (button->caption);
		free (button);
	}

	widget_destroy (message_window->widget);
	window_destroy (message_window->window);
	free (message_window->title);
	free (message_window->message);
	free (message_window);
}

static void
global_handler(struct display *display, uint32_t name,
	       const char *interface, uint32_t version, void *data)
{
	if (!strcmp(interface, "wl_text_input_manager")) {
		text_input_manager = display_bind (display, name, &wl_text_input_manager_interface, 1);
	}
}

void
wlmessage_run (char *message, char *title, char *titlebuttons, int noresize, char *buttons, char *icon, int timeout, char *deflt, char *textfield)
{
	struct display *display = NULL;

	display = display_create (NULL, NULL);
	if (!display) {
		fprintf (stderr, "Failed to connect to a Wayland compositor !\n");
		return;
	}

	if (timeout)
		display_set_timeout (display, timeout);

	message_window_create (display, message, title, titlebuttons, noresize, buttons, icon, deflt, textfield);
	display_set_global_handler (display, global_handler);
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
                        "    -file filename              file to read message from\n"
                        "    -buttons string             comma-separated list of label:exitcode\n"
                        "    -default button             button to activate if Return is pressed\n"
                        "    -textfield text             text field with default text\n"
                        "    -timeout secs               exit with status 0 after \"secs\" seconds\n"
                        "    -title title                window has this title\n"
                        "    -titlebuttons string        comma-separated list of \"Min, Max, Close, None\"\n"
                        "    -no-resize                  window is not resizable\n"
                        "    -icon filename              window shows this PNG icon\n"
                        "\n");
		return 0;
	}


	int i;
	char *message = NULL;
	char *buttons = NULL;
	char *deflt = NULL;
	char *textfield = NULL;
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

		if (!strcmp (argv[i], "-default")) {
			if (argc >= i+2)
				deflt = argv[i+1];
			i++; continue;
		}

		if (!strcmp (argv[i], "-textfield")) {
			if (argc >= i+2)
				textfield = argv[i+1];
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

	wlmessage_run (message, title, titlebuttons, noresize, buttons, icon, timeout, deflt, textfield);


	return 0;
}

