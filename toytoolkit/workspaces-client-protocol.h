#ifndef WORKSPACES_CLIENT_PROTOCOL_H
#define WORKSPACES_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct workspace_manager;

extern const struct wl_interface workspace_manager_interface;

/**
 * workspace_manager - workspaces manager
 * @state: workspace state
 *
 * An interface for managing surfaces in workspaces.
 */
struct workspace_manager_listener {
	/**
	 * state - workspace state
	 * @current: (none)
	 * @count: (none)
	 *
	 * The current workspace state, such as current workspace and
	 * workspace count, has changed.
	 */
	void (*state)(void *data,
		      struct workspace_manager *workspace_manager,
		      uint32_t current,
		      uint32_t count);
};

static inline int
workspace_manager_add_listener(struct workspace_manager *workspace_manager,
			       const struct workspace_manager_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) workspace_manager,
				     (void (**)(void)) listener, data);
}

#define WORKSPACE_MANAGER_MOVE_SURFACE	0

static inline void
workspace_manager_set_user_data(struct workspace_manager *workspace_manager, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) workspace_manager, user_data);
}

static inline void *
workspace_manager_get_user_data(struct workspace_manager *workspace_manager)
{
	return wl_proxy_get_user_data((struct wl_proxy *) workspace_manager);
}

static inline void
workspace_manager_destroy(struct workspace_manager *workspace_manager)
{
	wl_proxy_destroy((struct wl_proxy *) workspace_manager);
}

static inline void
workspace_manager_move_surface(struct workspace_manager *workspace_manager, struct wl_surface *surface, uint32_t workspace)
{
	wl_proxy_marshal((struct wl_proxy *) workspace_manager,
			 WORKSPACE_MANAGER_MOVE_SURFACE, surface, workspace);
}

#ifdef  __cplusplus
}
#endif

#endif
