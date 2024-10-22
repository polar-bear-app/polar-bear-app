//
// Created by Nguyễn Hồng Phát on 12/10/24.
//
#include <string>
#include <unistd.h>
#include <android/log.h>
#include <sys/socket.h>
#include <assert.h>

#include "wayland-server.h"
#include "xdg-shell.h"

#define LOG_TAG "PolarBearCompositorImplementations"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static void
surface_attach(struct wl_client *client,
               struct wl_resource *resource,
               struct wl_resource *buffer_resource, int32_t sx, int32_t sy) {
    if (buffer_resource) {
        auto buffer = wl_shm_buffer_get(buffer_resource);
        if (buffer == NULL) {
            wl_client_post_no_memory(client);
            return;
        }
    }

    if (wl_resource_get_version(resource) >= WL_SURFACE_OFFSET_SINCE_VERSION) {
        if (sx != 0 || sy != 0) {
            wl_resource_post_error(resource,
                                   WL_SURFACE_ERROR_INVALID_OFFSET,
                                   "Can't attach with an offset");
            return;
        }
    }
}

// static void
// surface_damage(struct wl_client *client,
//                struct wl_resource *resource,
//                int32_t x, int32_t y, int32_t width, int32_t height)
//{
//     struct weston_surface *surface = wl_resource_get_user_data(resource);
//
//     if (width <= 0 || height <= 0)
//         return;
//
//     pixman_region32_union_rect(&surface->pending.damage_surface,
//                                &surface->pending.damage_surface,
//                                x, y, width, height);
// }
// static void
// surface_commit(struct wl_client *client, struct wl_resource *resource)
//{
//     struct weston_surface *surface = wl_resource_get_user_data(resource);
//     struct weston_subsurface *sub = weston_surface_to_subsurface(surface);
//     enum weston_surface_status status;
//
//     if (!weston_surface_is_pending_viewport_source_valid(surface)) {
//         assert(surface->viewport_resource);
//
//         wl_resource_post_error(surface->viewport_resource,
//                                WP_VIEWPORT_ERROR_OUT_OF_BUFFER,
//                                "wl_surface@%d has viewport source outside buffer",
//                                wl_resource_get_id(resource));
//         return;
//     }
//
//     if (!weston_surface_is_pending_viewport_dst_size_int(surface)) {
//         assert(surface->viewport_resource);
//
//         wl_resource_post_error(surface->viewport_resource,
//                                WP_VIEWPORT_ERROR_BAD_SIZE,
//                                "wl_surface@%d viewport dst size not integer",
//                                wl_resource_get_id(resource));
//         return;
//     }
//
//     if (surface->pending.acquire_fence_fd >= 0) {
//         assert(surface->synchronization_resource);
//
//         if (!surface->pending.buffer) {
//             fd_clear(&surface->pending.acquire_fence_fd);
//             wl_resource_post_error(surface->synchronization_resource,
//                                    ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
//                                    "wl_surface@%"PRIu32" no buffer for synchronization",
//                                    wl_resource_get_id(resource));
//             return;
//         }
//
//         if (surface->pending.buffer->type == WESTON_BUFFER_SHM) {
//             fd_clear(&surface->pending.acquire_fence_fd);
//             wl_resource_post_error(surface->synchronization_resource,
//                                    ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_UNSUPPORTED_BUFFER,
//                                    "wl_surface@%"PRIu32" unsupported buffer for synchronization",
//                                    wl_resource_get_id(resource));
//             return;
//         }
//     }
//
//     if (surface->pending.buffer_release_ref.buffer_release &&
//         !surface->pending.buffer) {
//         assert(surface->synchronization_resource);
//
//         wl_resource_post_error(surface->synchronization_resource,
//                                ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
//                                "wl_surface@%"PRIu32" no buffer for synchronization",
//                                wl_resource_get_id(resource));
//         return;
//     }
//
//     if (sub) {
//         status = weston_subsurface_commit(sub);
//     } else {
//         status = WESTON_SURFACE_CLEAN;
//         wl_list_for_each(sub, &surface->subsurface_list, parent_link) {
//             if (sub->surface != surface)
//                 status |= weston_subsurface_parent_commit(sub, 0);
//         }
//         status |= weston_surface_commit(surface);
//     }
//
//     if (status & WESTON_SURFACE_DIRTY_SUBSURFACE_CONFIG)
//         surface->compositor->view_list_needs_rebuild = true;
// }


static const struct wl_surface_interface surface_interface = {
        nullptr,        // surface_destroy
        surface_attach, // surface_attach
        nullptr,        // surface_damage
        nullptr,        // surface_frame
        nullptr,        // surface_set_opaque_region
        nullptr,        // surface_set_input_region
        nullptr,        // surface_commit
        nullptr,        // surface_set_buffer_transform
        nullptr,        // surface_set_buffer_scale
        nullptr,        // surface_damage_buffer
        nullptr,        // surface_offset
};

static void
compositor_create_surface(struct wl_client *client,
                          struct wl_resource *resource, uint32_t id) {
    auto surface =
            wl_resource_create(client, &wl_surface_interface,
                               wl_resource_get_version(resource), id);
    wl_resource_set_implementation(surface, &surface_interface,
                                   surface, nullptr);
}

static void
compositor_create_region(struct wl_client *client,
                         struct wl_resource *resource, uint32_t id) {
    auto region =
            wl_resource_create(client, &wl_region_interface, 1, id);
    if (region == nullptr) {
        free(region);
        wl_resource_post_no_memory(resource);
        return;
    }
    //    wl_resource_set_implementation(region, &region_interface, region, nullptr);
}

static const struct wl_compositor_interface compositor_impl = {
        compositor_create_surface,
        compositor_create_region
};

static const struct xdg_wm_base_interface xdg_shell_impl = {
        .destroy = [](struct wl_client *client,
                      struct wl_resource *resource) {
            wl_resource_destroy(resource);
        },
        .create_positioner = [](struct wl_client *wl_client,
                                struct wl_resource *wl_resource,
                                uint32_t id) {
            auto resource =
                    wl_resource_create(wl_client,
                                       &xdg_positioner_interface,
                                       wl_resource_get_version(wl_resource), id);
            if (resource == NULL) {
                wl_client_post_no_memory(wl_client);
                return;
            }
//            wl_resource_set_implementation(resource,
//                                           &weston_desktop_xdg_positioner_implementation,
//                                           positioner, weston_desktop_xdg_positioner_destroy);
        },
        .get_xdg_surface = [](struct wl_client *wl_client,
                              struct wl_resource *resource,
                              uint32_t id,
                              struct wl_resource *surface_resource) {
            assert(false);
        },
        .pong = [](struct wl_client *wl_client,
                   struct wl_resource *resource,
                   uint32_t serial) {
            assert(false);
        },
};

void implement(wl_display *wl_display) {
    // Compositor
    wl_global_create(wl_display, &wl_compositor_interface,
                     wl_compositor_interface.version, nullptr,
                     [](struct wl_client *wl_client, void *data,
                        uint32_t version, uint32_t id) {
                         //    struct wlr_compositor *compositor = data;

                         struct wl_resource *resource =
                                 wl_resource_create(wl_client, &wl_compositor_interface, version,
                                                    id);
                         if (resource == NULL) {
                             wl_client_post_no_memory(wl_client);
                             return;
                         }
                         wl_resource_set_implementation(resource, &compositor_impl, nullptr, NULL);
                     }
    );

    // XDG Shell
    wl_global_create(wl_display, &xdg_wm_base_interface,
                     xdg_wm_base_interface.version, nullptr,
                     [](struct wl_client *client, void *data,
                        uint32_t version, uint32_t id) {
                         struct wl_display *display;
                         struct wl_event_loop *loop;

                         auto resource = wl_resource_create(client, &xdg_wm_base_interface, version,
                                                            id);
                         if (resource == NULL) {
                             wl_client_post_no_memory(client);
                             return;
                         }

                         wl_resource_set_implementation(resource, &xdg_shell_impl,
                                                        client,
                                                        nullptr);

                         display = wl_client_get_display(client);
                         loop = wl_display_get_event_loop(display);
                         auto ping_timer =
                                 wl_event_loop_add_timer(loop,
                                                         [](void *user_data) {
                                                             return 1;
                                                         },
                                                         client);
                         if (ping_timer == NULL) {
                             wl_client_post_no_memory(client);
                         }
                     });

    // SHM
    wl_display_init_shm(wl_display);
    wl_display_add_shm_format(wl_display, WL_SHM_FORMAT_ARGB8888);
    wl_display_add_shm_format(wl_display, WL_SHM_FORMAT_XRGB8888);
}