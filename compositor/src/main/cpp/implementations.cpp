//
// Created by Nguyễn Hồng Phát on 12/10/24.
//
#include <jni.h>
#include <string>
#include <unistd.h>
#include <android/log.h>
#include <sys/socket.h>
#include <assert.h>

#include "wayland-server.h"
#include "xdg-shell.h"

#define LOG_TAG "PolarBearCompositorImplementations"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct PolarBearGlobal {
    wl_display *display;
    wl_shm_buffer *buffer;
    wl_resource *xdg_top_level;

    void (*render)(wl_shm_buffer *);
};

struct PolarBearGlobal pb_global = {nullptr};

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
        pb_global.buffer = buffer;
    }
}

static void
surface_damage(struct wl_client *client,
               struct wl_resource *resource,
               int32_t x, int32_t y, int32_t width, int32_t height) {
    LOGI("surface_damage");
}

static void
surface_commit(struct wl_client *client, struct wl_resource *resource) {
    LOGI("surface_commit");
    if (pb_global.buffer != nullptr) {
        pb_global.render(pb_global.buffer);
    }
    xdg_surface_send_configure(pb_global.xdg_top_level,
                               wl_display_next_serial(pb_global.display));
}

static const struct wl_surface_interface surface_interface = {
        nullptr,        // surface_destroy
        surface_attach, // surface_attach
        surface_damage,        // surface_damage
        nullptr,        // surface_frame
        nullptr,        // surface_set_opaque_region
        nullptr,        // surface_set_input_region
        surface_commit,        // surface_commit
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


static const struct xdg_toplevel_interface xdg_toplevel_impl = {
        .set_title           = [](struct wl_client *wl_client,
                                  struct wl_resource *resource,
                                  const char *title)
        {
            LOGI("set_title %s", title);
        },
};

static const struct xdg_surface_interface xdg_surface_impl = {
        .get_toplevel        = [](struct wl_client *wl_client,
                                  struct wl_resource *xdg_toplevel_resource,
                                  uint32_t id) {
            auto resource = wl_resource_create(wl_client,
                                               &xdg_toplevel_interface,
                                               wl_resource_get_version(xdg_toplevel_resource),
                                               id);
            wl_resource_set_implementation(resource, &xdg_toplevel_impl, nullptr,
                                           nullptr);

            pb_global.xdg_top_level = xdg_toplevel_resource;
            xdg_surface_send_configure(pb_global.xdg_top_level,
                                       wl_display_next_serial(pb_global.display));
        },
        .ack_configure       = [](struct wl_client *wl_client,
                                  struct wl_resource *resource,
                                  uint32_t serial) {
            LOGI("ack_configure");
        },
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
                              struct wl_resource *xdg_surface_resource,
                              uint32_t id,
                              struct wl_resource *surface_resource) {
            auto resource = wl_resource_create(wl_client,
                                               &xdg_surface_interface,
                                               wl_resource_get_version(xdg_surface_resource),
                                               id);
            wl_resource_set_implementation(resource, &xdg_surface_impl, nullptr, nullptr);

        },
        .pong = [](struct wl_client *wl_client,
                   struct wl_resource *resource,
                   uint32_t serial) {
            assert(false);
        },
};

void implement(wl_display *wl_display, void (*render)(wl_shm_buffer *)) {
    pb_global.display = wl_display;
    pb_global.render = render;
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