#ifndef TEXT_CURSOR_POSITION_CLIENT_PROTOCOL_H
#define TEXT_CURSOR_POSITION_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct text_cursor_position;

extern const struct wl_interface text_cursor_position_interface;

#define TEXT_CURSOR_POSITION_NOTIFY	0

static inline void
text_cursor_position_set_user_data(struct text_cursor_position *text_cursor_position, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) text_cursor_position, user_data);
}

static inline void *
text_cursor_position_get_user_data(struct text_cursor_position *text_cursor_position)
{
	return wl_proxy_get_user_data((struct wl_proxy *) text_cursor_position);
}

static inline void
text_cursor_position_destroy(struct text_cursor_position *text_cursor_position)
{
	wl_proxy_destroy((struct wl_proxy *) text_cursor_position);
}

static inline void
text_cursor_position_notify(struct text_cursor_position *text_cursor_position, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
	wl_proxy_marshal((struct wl_proxy *) text_cursor_position,
			 TEXT_CURSOR_POSITION_NOTIFY, surface, x, y);
}

#ifdef  __cplusplus
}
#endif

#endif
