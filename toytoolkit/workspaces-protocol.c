#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_surface_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	&wl_surface_interface,
	NULL,
};

static const struct wl_message workspace_manager_requests[] = {
	{ "move_surface", "ou", types + 2 },
};

static const struct wl_message workspace_manager_events[] = {
	{ "state", "uu", types + 0 },
};

WL_EXPORT const struct wl_interface workspace_manager_interface = {
	"workspace_manager", 1,
	1, workspace_manager_requests,
	1, workspace_manager_events,
};

