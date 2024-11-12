//
// Created by Nguyễn Hồng Phát on 12/10/24.
//
#include <thread>
#include <string>
#include <cassert>

#include <jni.h>
#include <unistd.h>
#include <android/log.h>
#include <sys/socket.h>

#include "wayland-server.h"
#include "xdg-shell.h"
#include "utils.h"
#include "data.h"

#define LOG_TAG "PolarBearCompositorImplementations"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct PolarBearGlobal {
    wl_display *display;
    wl_shm_buffer *buffer;
    wl_resource *surface;
    wl_resource *xdg_surface;
    wl_resource *xdg_toplevel_surface;
    wl_resource *pointer;
    wl_resource *touch;

    void (*render)(wl_shm_buffer *);
};

struct PolarBearGlobal pb_global = {nullptr};

void send_configures() {
    struct wl_array states;
    wl_array_init(&states);
    uint32_t *s = static_cast<uint32_t *>(wl_array_add(&states,
                                                       sizeof(uint32_t)));
    *s = XDG_TOPLEVEL_STATE_FULLSCREEN;
    xdg_toplevel_send_configure(pb_global.xdg_toplevel_surface, 1024,
                                768,
                                &states);
    wl_array_release(&states);

    xdg_surface_send_configure(pb_global.xdg_surface,
                               wl_display_next_serial(
                                       pb_global.display));
}

static const struct wl_surface_interface surface_impl = {
        [](struct wl_client *client, struct wl_resource *resource) {},
        [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer,
           int32_t x, int32_t y) {
            if (buffer) {
                auto pShmBuffer = wl_shm_buffer_get(buffer);
                if (pShmBuffer == NULL) {
                    wl_client_post_no_memory(client);
                    return;
                }
                pb_global.buffer = pShmBuffer;
            }
        },
        [](struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y,
           int32_t width, int32_t height) {
            LOGI("surface_damage");
        },
        [](struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
            auto cb = wl_resource_create(client, &wl_callback_interface, 1, callback);
            wl_resource_set_implementation(cb, nullptr, nullptr, nullptr);
            wl_callback_send_done(cb, get_current_timestamp());
        },
        [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {},
        [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *region) {},
        [](struct wl_client *client, struct wl_resource *resource) {
            LOGI("surface_commit");
            if (pb_global.buffer != nullptr) {
                pb_global.render(pb_global.buffer);
            }
            send_configures();
        },
        [](struct wl_client *client, struct wl_resource *resource, int32_t transform) {},
        [](struct wl_client *client, struct wl_resource *resource, int32_t scale) {},
        [](struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y,
           int32_t width, int32_t height) {},
        [](struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y) {},
};

static const struct wl_region_interface region_impl = {
        [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        },
        [](struct wl_client *client, struct wl_resource *resource,
           int32_t x, int32_t y, int32_t width, int32_t height) {
            LOGI("region_add");
        },
        [](struct wl_client *client, struct wl_resource *resource,
           int32_t x, int32_t y, int32_t width, int32_t height) {
            LOGI("region_subtract");
        }
};

static const struct wl_compositor_interface compositor_impl = {
        [](struct wl_client *client,
           struct wl_resource *resource, uint32_t id) {
            auto surface =
                    wl_resource_create(client, &wl_surface_interface,
                                       wl_resource_get_version(resource), id);
            wl_resource_set_implementation(surface, &surface_impl,
                                           nullptr, nullptr);
        },
        [](struct wl_client *client,
           struct wl_resource *resource, uint32_t id) {
            auto region =
                    wl_resource_create(client, &wl_region_interface, 1, id);
            if (region == nullptr) {
                free(region);
                wl_resource_post_no_memory(resource);
                return;
            }
            wl_resource_set_implementation(region, &region_impl, nullptr, nullptr);
        }
};

static const struct xdg_toplevel_interface xdg_toplevel_impl = {

        [](struct wl_client *client, struct wl_resource *resource) {},

        [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent) {},

        [](struct wl_client *wl_client,
           struct wl_resource *resource,
           const char *title) {
            LOGI("set_title %s", title);
            send_configures();
        },

        [](struct wl_client *wl_client,
           struct wl_resource *resource,
           const char *app_id) {
            LOGI("set_app_id %s", app_id);
            send_configures();
        },

        [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat,
           uint32_t serial, int32_t x, int32_t y) {},

        [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat,
           uint32_t serial) {},

        [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat,
           uint32_t serial, uint32_t edges) {},

        [](struct wl_client *client, struct wl_resource *resource, int32_t width,
           int32_t height) {},

        [](struct wl_client *client, struct wl_resource *resource, int32_t width,
           int32_t height) {},

        [](struct wl_client *client, struct wl_resource *resource) {},

        [](struct wl_client *client, struct wl_resource *resource) {},

        [](struct wl_client *wl_client,
           struct wl_resource *resource,
           struct wl_resource *output_resource) {
            send_configures();
        },

        [](struct wl_client *client, struct wl_resource *resource) {},

        [](struct wl_client *client, struct wl_resource *resource) {},
};

static const struct xdg_surface_interface xdg_surface_impl = {

        [](struct wl_client *client, struct wl_resource *resource) {},

        [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            auto res = wl_resource_create(client,
                                          &xdg_toplevel_interface,
                                          wl_resource_get_version(resource),
                                          id);
            wl_resource_set_implementation(res, &xdg_toplevel_impl, nullptr,
                                           nullptr);

            pb_global.xdg_toplevel_surface = res;
        },

        [](struct wl_client *client, struct wl_resource *resource, uint32_t id,
           struct wl_resource *parent, struct wl_resource *positioner) {
            assert(false);
        },

        [](struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y,
           int32_t width, int32_t height) {
//            assert(false);
        },

        [](struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
//            assert(false);
        },

};

static const struct xdg_wm_base_interface xdg_shell_impl = {
        [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        },
        [](struct wl_client *wl_client, struct wl_resource *wl_resource, uint32_t id) {
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
        [](struct wl_client *wl_client, struct wl_resource *xdg_wm_base_resource, uint32_t id,
           struct wl_resource *wl_surface_resource) {
            auto resource = wl_resource_create(wl_client,
                                               &xdg_surface_interface,
                                               wl_resource_get_version(wl_surface_resource),
                                               id);
            wl_resource_set_implementation(resource, &xdg_surface_impl, nullptr, nullptr);

            pb_global.xdg_surface = resource;
            pb_global.surface = wl_surface_resource;
        },
        [](struct wl_client *wl_client, struct wl_resource *resource, uint32_t serial) {
            LOGI("pong");
        },
};

static const struct wl_output_interface output_impl = {
        [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        },
};

static const struct wl_pointer_interface pointer_impl = {
        [](struct wl_client *client,
           struct wl_resource *resource,
           uint32_t serial,
           struct wl_resource *surface,
           int32_t hotspot_x,
           int32_t hotspot_y) {
            LOGI("wl_pointer_interface@set_cursor");
        },
        [](struct wl_client *client,
           struct wl_resource *resource) {

        }
};

static const struct wl_touch_interface touch_impl = {
        [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        }
};

static const struct wl_seat_interface seat_impl = {
        [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            auto pointer = wl_resource_create(client, &wl_pointer_interface,
                                              wl_resource_get_version(resource), id);

            wl_resource_set_implementation(pointer, &pointer_impl, nullptr,
                                           nullptr);
            pb_global.pointer = pointer;

            wl_pointer_send_enter(pointer,
                                  wl_display_next_serial(pb_global.display),
                                  pb_global.surface,
                                  wl_fixed_from_double(0),
                                  wl_fixed_from_double(0));
            wl_pointer_send_frame(pointer);
        },
        [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {},
        [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            auto touch = wl_resource_create(client, &wl_touch_interface,
                                            wl_resource_get_version(resource), id);

            wl_resource_set_implementation(touch, &touch_impl,
                                           nullptr, nullptr);

            pb_global.touch = touch;
        },
        [](struct wl_client *client, struct wl_resource *resource) {},
};


wl_display *setup_compositor(const char *socket_name) {
    struct wl_display *wl_display = wl_display_create();
    if (!wl_display) {
        LOGI("Unable to create Wayland display.");
        return nullptr;
    }
    auto socket_path =
            std::string("/data/data/app.polarbear/files/archlinux-aarch64/tmp/") + socket_name;
    int sock_fd = create_unix_socket(socket_path);
    if (sock_fd < 0) {
        LOGI("Failed to create Unix socket.");
        return nullptr;
    }

    if (wl_display_add_socket_fd(wl_display, sock_fd) < 0) {
        LOGI("Unable to add socket to Wayland display.");
        close_unix_socket(sock_fd);
        return nullptr;
    }

    return wl_display;
}

std::string implement(void (*render)(wl_shm_buffer *)) {
    std::string socket_name = "wayland-pb";
    auto wl_display = setup_compositor(socket_name.c_str());

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

                         xdg_wm_base_send_ping(resource, wl_display_next_serial(pb_global.display));
                     });

    // Output
    wl_global_create(wl_display,
                     &wl_output_interface, 4,
                     nullptr, [](struct wl_client *client,
                                 void *data, uint32_t version, uint32_t id) {

                auto resource = wl_resource_create(client, &wl_output_interface,
                                                   version, id);

                wl_resource_set_implementation(resource, &output_impl, nullptr,
                                               nullptr);

                wl_output_send_geometry(resource,
                                        0,
                                        0,
                                        1024,
                                        768,
                                        WL_OUTPUT_SUBPIXEL_NONE,
                                        "Polar Bear", "Polar Bear Virtual Wayland Display",
                                        WL_OUTPUT_TRANSFORM_NORMAL);
                if (version >= WL_OUTPUT_SCALE_SINCE_VERSION)
                    wl_output_send_scale(resource,
                                         1);

                wl_output_send_mode(resource,
                                    WL_OUTPUT_MODE_CURRENT,
                                    1024,
                                    768,
                                    60 * 1000LL);

                if (version >= WL_OUTPUT_NAME_SINCE_VERSION)
                    wl_output_send_name(resource, "Polar Bear");

                if (version >= WL_OUTPUT_DESCRIPTION_SINCE_VERSION)
                    wl_output_send_description(resource, "Polar Bear Virtual Wayland Display");

                if (version >= WL_OUTPUT_DONE_SINCE_VERSION)
                    wl_output_send_done(resource);
            });

    // Seat
    wl_global_create(wl_display,
                     &wl_seat_interface, wl_seat_interface.version,
                     nullptr, [](struct wl_client *client,
                                 void *data, uint32_t version, uint32_t id) {

                auto resource = wl_resource_create(client, &wl_seat_interface,
                                                   version, id);

                wl_resource_set_implementation(resource, &seat_impl, nullptr,
                                               nullptr);


                if (version >= WL_SEAT_NAME_SINCE_VERSION)
                    wl_seat_send_name(resource, "Polar Bear Virtual Input");

                wl_seat_send_capabilities(resource,
                                          WL_SEAT_CAPABILITY_TOUCH | WL_SEAT_CAPABILITY_POINTER);
            });

    // SHM
    wl_display_init_shm(wl_display);

    // Log
    wl_display_add_protocol_logger(wl_display, [](void *user_data,
                                                  enum wl_protocol_logger_type direction,
                                                  const struct wl_protocol_logger_message *message) {
        LOGI("[LOG] %s %s@%d:%s", direction == WL_PROTOCOL_LOGGER_REQUEST ? "<-" : "->",
             message->resource->object.interface->name,
             message->resource->object.id, message->message->name);
    }, nullptr);

    return socket_name;
}

void run() {
    LOGI("Running Wayland display...");

    wl_display_run(pb_global.display);
    wl_display_destroy(pb_global.display);
}

void handle_event(TouchEventData event) {
    if (pb_global.touch == nullptr) {
        return;
    }

    // Convert coordinates to wl_fixed_t format
    wl_fixed_t x_fixed = wl_fixed_from_double(static_cast<double>(event.x));
    wl_fixed_t y_fixed = wl_fixed_from_double(static_cast<double>(event.y));

    // Define constants for motion events (matching Android's MotionEvent constants)
    const int ACTION_DOWN = 0;
    const int ACTION_UP = 1;
    const int ACTION_MOVE = 2;

    // Serial number, incremented with each unique event
    auto serial = wl_display_next_serial(pb_global.display);
    auto id = 1;

    // Check the action type and call the corresponding wl_touch_* function
    switch (event.action) {
        case ACTION_DOWN:
            wl_touch_send_down(pb_global.touch, serial, event.timestamp, pb_global.surface,
                               id,
                               x_fixed, y_fixed);
            wl_touch_send_up(pb_global.touch, serial, event.timestamp, id);
            wl_pointer_send_enter(pb_global.pointer, serial, pb_global.surface, x_fixed, y_fixed);
            wl_pointer_send_button(pb_global.pointer, serial, event.timestamp, 0, 0);
            wl_pointer_send_frame(pb_global.pointer);
            break;

        case ACTION_UP:
            wl_touch_send_up(pb_global.touch, serial, event.timestamp, event.pointerId);
            break;

        case ACTION_MOVE:
            wl_touch_send_motion(pb_global.touch, event.timestamp, event.pointerId, x_fixed,
                                 y_fixed);
            break;
    }

    // Send a frame event after each touch sequence to signal the end of this touch event group
    wl_touch_send_frame(pb_global.touch);
}