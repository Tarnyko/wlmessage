#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_surface_interface;

static const struct wl_interface *types[] = {
	&wl_surface_interface,
	NULL,
	NULL,
};

static const struct wl_message text_cursor_position_requests[] = {
	{ "notify", "off", types + 0 },
};

WL_EXPORT const struct wl_interface text_cursor_position_interface = {
	"text_cursor_position", 1,
	1, text_cursor_position_requests,
	0, NULL,
};

