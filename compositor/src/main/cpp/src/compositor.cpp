//
// Created by Nguyễn Hồng Phát on 12/10/24.
//
#include <thread>
#include <string>
#include <cassert>
#include <jni.h>
#include <unistd.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/sharedmem.h>
#include <sys/socket.h>
#include <sys/mman.h>

#include "wayland-server.h"
#include "xdg-shell.h"
#include "xkbcommon/xkbcommon.h"

#include "utils.h"
#include "data.h"

#define LOG_TAG "PolarBearCompositorImplementations"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;

struct PolarBearSurface {
    ANativeWindow *androidNativeWindow = nullptr;
    wl_shm_buffer *waylandBuffer = nullptr;
    wl_resource *wl_surface = nullptr;
    wl_resource *xdg_surface = nullptr;
    wl_resource *xdg_toplevel_surface = nullptr;
};

struct PolarBearState {
    unordered_map<uint32_t, PolarBearSurface> surfaces;
    function<void(const string &, const vector<string> &)> callJVM;

    // to be structured
    wl_display *display;
    wl_resource *output;
    wl_resource *seat;
    wl_resource *touch;
    wl_resource *keyboard;
    wl_resource *pointer;
    int32_t touchDownId;
    bool sent_enter = false;
};

static PolarBearState state;

void send_configures(const PolarBearSurface &surface) {
    if (surface.xdg_surface != nullptr
        && surface.xdg_toplevel_surface != nullptr
        && surface.androidNativeWindow != nullptr) {
        // Send toplevel configure
        struct wl_array states;
        wl_array_init(&states);
        auto *s = static_cast<uint32_t *>(wl_array_add(&states,
                                                       sizeof(uint32_t)));
        *s = XDG_TOPLEVEL_STATE_FULLSCREEN;
        xdg_toplevel_send_configure(surface.xdg_toplevel_surface, 1024,
                                    768,
                                    &states);
        wl_array_release(&states);

        // Send xdg configure
        xdg_surface_send_configure(surface.xdg_surface,
                                   wl_display_next_serial(
                                           state.display));

        wl_surface_send_enter(surface.wl_surface, state.output);
    } else {
        wl_display_flush_clients(state.display);
    }
}

void render(const PolarBearSurface &surface) {
    auto waylandBuffer = surface.waylandBuffer;
    auto androidNativeWindow = surface.androidNativeWindow;

    if (surface.waylandBuffer == nullptr || surface.androidNativeWindow == nullptr) {
        return;
    }

    // Convert the Surface (jobject) to ANativeWindow
    wl_shm_buffer_begin_access(waylandBuffer);

    // Step 2: Get buffer details.
    void *waylandData = wl_shm_buffer_get_data(waylandBuffer);
    int32_t width = wl_shm_buffer_get_width(waylandBuffer);
    int32_t height = wl_shm_buffer_get_height(waylandBuffer);
    int32_t stride = wl_shm_buffer_get_stride(waylandBuffer);
    uint32_t format = wl_shm_buffer_get_format(waylandBuffer);

    // Step 3: Configure the native window.
    ANativeWindow_setBuffersGeometry(androidNativeWindow, width, height,
                                     format == WL_SHM_FORMAT_ARGB8888
                                     ? AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM
                                     : AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM);

    // Step 4: Lock the native window buffer for writing.
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(androidNativeWindow, &buffer, nullptr) == 0) {
        // Step 5: Copy the data from the Wayland buffer to the native window buffer.
        uint8_t *dest = static_cast<uint8_t *>(buffer.bits);
        uint8_t *src = static_cast<uint8_t *>(waylandData);

        for (int y = 0; y < height; y++) {
            memcpy(dest, src, width * 4); // 4 bytes per pixel for RGBX_8888.
            dest += buffer.stride * 4;    // Move to the next line in the native buffer.
            src += stride;                // Move to the next line in the Wayland buffer.
        }

        // Step 6: Unlock and post the buffer to display the content.
        ANativeWindow_unlockAndPost(androidNativeWindow);
    }

    // Step 7: End access to the Wayland buffer.
    wl_shm_buffer_end_access(waylandBuffer);
}

static const struct wl_surface_interface surface_impl = {
        .destroy = [](struct wl_client *client, struct wl_resource *resource) {},
        .attach = [](struct wl_client *client, struct wl_resource *resource,
                     struct wl_resource *buffer,
                     int32_t x, int32_t y) {
            if (buffer) {
                auto pShmBuffer = wl_shm_buffer_get(buffer);
                if (pShmBuffer == NULL) {
                    wl_client_post_no_memory(client);
                    return;
                }
                state.surfaces[resource->object.id].waylandBuffer = pShmBuffer;
            }
        },
        .damage = [](struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y,
                     int32_t width, int32_t height) {},
        .frame = [](struct wl_client *client, struct wl_resource *resource, uint32_t callback) {
            auto cb = wl_resource_create(client, &wl_callback_interface, 1, callback);
            wl_resource_set_implementation(cb, nullptr, nullptr, nullptr);
            wl_callback_send_done(cb, get_current_timestamp());
        },
        .set_opaque_region = [](struct wl_client *client, struct wl_resource *resource,
                                struct wl_resource *region) {},
        .set_input_region = [](struct wl_client *client, struct wl_resource *resource,
                               struct wl_resource *region) {},
        .commit = [](struct wl_client *client, struct wl_resource *resource) {
            render(state.surfaces[resource->object.id]);
        },
        .set_buffer_transform = [](struct wl_client *client, struct wl_resource *resource,
                                   int32_t transform) {},
        .set_buffer_scale = [](struct wl_client *client, struct wl_resource *resource,
                               int32_t scale) {},
        .damage_buffer = [](struct wl_client *client, struct wl_resource *resource, int32_t x,
                            int32_t y,
                            int32_t width, int32_t height) {},
        .offset = [](struct wl_client *client, struct wl_resource *resource, int32_t x,
                     int32_t y) {},
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
        .create_surface = [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            PolarBearSurface surface;

            // Wayland protocol logic
            surface.wl_surface = wl_resource_create(client, &wl_surface_interface,
                                                    wl_resource_get_version(resource), id);
            wl_resource_set_implementation(surface.wl_surface, &surface_impl,
                                           nullptr, nullptr);

            state.surfaces.emplace(id, surface);
            // Polar Bear logic
            state.callJVM("createSurface", {to_string(id)}); // Tell Kotlin to create a SurfaceView
        },
        .create_region = [](struct wl_client *client,
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

        .destroy = [](struct wl_client *client, struct wl_resource *resource) {},

        .set_parent = [](struct wl_client *client, struct wl_resource *resource,
                         struct wl_resource *parent) {},

        .set_title = [](struct wl_client *wl_client,
                        struct wl_resource *resource,
                        const char *title) {
            LOGI("set_title %s", title);
        },

        .set_app_id = [](struct wl_client *wl_client,
                         struct wl_resource *resource,
                         const char *app_id) {
            LOGI("set_app_id %s", app_id);
        },

        .show_window_menu = [](struct wl_client *client, struct wl_resource *resource,
                               struct wl_resource *seat,
                               uint32_t serial, int32_t x, int32_t y) {},

        .move = [](struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat,
                   uint32_t serial) {},

        .resize = [](struct wl_client *client, struct wl_resource *resource,
                     struct wl_resource *seat,
                     uint32_t serial, uint32_t edges) {},

        .set_max_size = [](struct wl_client *client, struct wl_resource *resource, int32_t width,
                           int32_t height) {},

        .set_min_size = [](struct wl_client *client, struct wl_resource *resource, int32_t width,
                           int32_t height) {},

        .set_maximized = [](struct wl_client *client, struct wl_resource *resource) {},

        .unset_maximized = [](struct wl_client *client, struct wl_resource *resource) {},

        .set_fullscreen = [](struct wl_client *wl_client,
                             struct wl_resource *resource,
                             struct wl_resource *output_resource) {
        },

        .unset_fullscreen = [](struct wl_client *client, struct wl_resource *resource) {},

        .set_minimized = [](struct wl_client *client, struct wl_resource *resource) {},
};

static const struct xdg_surface_interface xdg_surface_impl = {

        .destroy = [](struct wl_client *client, struct wl_resource *resource) {},

        .get_toplevel = [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            auto res = wl_resource_create(client,
                                          &xdg_toplevel_interface,
                                          wl_resource_get_version(resource),
                                          id);
            wl_resource_set_implementation(res, &xdg_toplevel_impl, nullptr,
                                           nullptr);

            auto surface = find_if(state.surfaces.begin(), state.surfaces.end(),
                                   [&resource](const pair<uint32_t, PolarBearSurface> &surface) {
                                       return surface.second.xdg_surface != nullptr &&
                                              surface.second.xdg_surface->object.id ==
                                              resource->object.id;
                                   });
            assert(surface != state.surfaces.end());
            surface->second.xdg_toplevel_surface = res;

            send_configures(surface->second);
        },

        .get_popup = [](struct wl_client *client, struct wl_resource *resource, uint32_t id,
                        struct wl_resource *parent, struct wl_resource *positioner) {
            assert(false);
        },

        .set_window_geometry = [](struct wl_client *client, struct wl_resource *resource, int32_t x,
                                  int32_t y,
                                  int32_t width, int32_t height) {
//            assert(false);
        },

        .ack_configure = [](struct wl_client *client, struct wl_resource *resource,
                            uint32_t serial) {
//            assert(false);
        },

};

static const struct xdg_wm_base_interface xdg_shell_impl = {
        .destroy = [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        },
        .create_positioner = [](struct wl_client *wl_client, struct wl_resource *wl_resource,
                                uint32_t id) {
//            auto resource =
//                    wl_resource_create(wl_client,
//                                       &xdg_positioner_interface,
//                                       wl_resource_get_version(wl_resource), id);
//            if (resource == NULL) {
//                wl_client_post_no_memory(wl_client);
//                return;
//            }
//            wl_resource_set_implementation(resource,
//                                           &weston_desktop_xdg_positioner_implementation,
//                                           positioner, weston_desktop_xdg_positioner_destroy);
        },
        .get_xdg_surface = [](struct wl_client *wl_client, struct wl_resource *xdg_wm_base_resource,
                              uint32_t id,
                              struct wl_resource *wl_surface_resource) {
            auto resource = wl_resource_create(wl_client,
                                               &xdg_surface_interface,
                                               wl_resource_get_version(wl_surface_resource),
                                               id);
            wl_resource_set_implementation(resource, &xdg_surface_impl, nullptr, nullptr);
            assert(state.surfaces.count(wl_surface_resource->object.id) > 0);
            state.surfaces[wl_surface_resource->object.id].xdg_surface = resource;
        },
        .pong = [](struct wl_client *wl_client, struct wl_resource *resource, uint32_t serial) {
            LOGI("pong");
        },
};

static const struct wl_output_interface output_impl = {
        .release = [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        },
};

static const struct wl_pointer_interface pointer_impl = {
        .set_cursor = [](struct wl_client *client,
                         struct wl_resource *resource,
                         uint32_t serial,
                         struct wl_resource *surface,
                         int32_t hotspot_x,
                         int32_t hotspot_y) {
            LOGI("wl_pointer_interface@set_cursor");
        },
        .release = [](struct wl_client *client,
                      struct wl_resource *resource) {

        }
};

static const struct wl_touch_interface touch_impl = {
        .release = [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        }
};

static const struct wl_keyboard_interface keyboard_interface = {
        .release = [](struct wl_client *client, struct wl_resource *resource) {
            wl_resource_destroy(resource);
        }
};

static const struct wl_seat_interface seat_impl = {
        .get_pointer = [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            auto pointer = wl_resource_create(client, &wl_pointer_interface,
                                              wl_resource_get_version(resource), id);

            wl_resource_set_implementation(pointer, &pointer_impl, nullptr,
                                           nullptr);
            state.pointer = pointer;
            wl_pointer_send_frame(pointer);
        },
        .get_keyboard = [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            auto keyboard = wl_resource_create(client, &wl_keyboard_interface,
                                               wl_resource_get_version(resource), id);

            wl_resource_set_implementation(keyboard, &keyboard_interface,
                                           nullptr, nullptr);

            state.keyboard = keyboard;

            if (wl_resource_get_version(keyboard) >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION) {
//                wl_keyboard_send_repeat_info(keyboard,
//                                             seat->compositor->kb_repeat_rate,
//                                             seat->compositor->kb_repeat_delay);
            }

            // send keymap

            /* We need to prepare an XKB keymap and assign it to the keyboard. This
             * assumes the defaults (e.g. layout = "us"). */
            struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
            struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,
                                                                  XKB_KEYMAP_COMPILE_NO_FLAGS);
            struct xkb_state *xkb_state = xkb_state_new(keymap);

            char *keymap_str = xkb_keymap_get_as_string(keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
            size_t keymap_size = strlen(keymap_str) + 1;

            int fd = ASharedMemory_create("xkb_keymap",
                                          keymap_size); // By default it has PROT_READ | PROT_WRITE | PROT_EXEC.
            void *dst = mmap(NULL, keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            assert(dst != MAP_FAILED);
            memcpy(dst, keymap_str, keymap_size);
            munmap(dst, keymap_size);

            ASharedMemory_setProt(fd, PROT_READ); // limit access to read only
            wl_keyboard_send_keymap(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, keymap_size);
        },
        .get_touch = [](struct wl_client *client, struct wl_resource *resource, uint32_t id) {
            auto touch = wl_resource_create(client, &wl_touch_interface,
                                            wl_resource_get_version(resource), id);

            wl_resource_set_implementation(touch, &touch_impl,
                                           nullptr, nullptr);

            state.touch = touch;
        },
        .release = [](struct wl_client *client, struct wl_resource *resource) {},
};


wl_display *setup_compositor(const string socket_name) {
    struct wl_display *wl_display = wl_display_create();
    if (!wl_display) {
        LOGI("Unable to create Wayland display.");
        return nullptr;
    }
    int sock_fd = create_unix_socket(socket_name);
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

const string implement(const string &socket_name) {
    auto wl_display = setup_compositor(socket_name.c_str());

    state.display = wl_display;
    // Compositor
    wl_global_create(wl_display, &wl_compositor_interface,
                     wl_compositor_interface.version, nullptr,
                     [](struct wl_client *wl_client, void *data,
                        uint32_t version, uint32_t id) {
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

                         xdg_wm_base_send_ping(resource, wl_display_next_serial(state.display));
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

                state.output = resource;

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
                                          WL_SEAT_CAPABILITY_TOUCH | WL_SEAT_CAPABILITY_KEYBOARD);

                state.seat = resource;
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

void run(const function<string(const string &, const vector<string> &)> &callJVM) {
    state.callJVM = callJVM;
    callJVM("ready", {});

    LOGI("Running Wayland display...");
    wl_display_run(state.display);
    wl_display_destroy(state.display);
}

void set_surface(uint32_t id, ANativeWindow *pWindow) {
    assert(state.surfaces.count(id));

    // Link this surface with the SurfaceView MainActivity just created
    auto &surface = state.surfaces[id];
    surface.androidNativeWindow = pWindow;

    // Tell the client that this surface is ready
    send_configures(surface);
}

void handle_touch_event(uint32_t surface_id, TouchEventData event) {
    if (state.touch != nullptr) {
        auto surface = state.surfaces[surface_id].wl_surface;

        // Convert coordinates to wl_fixed_t format
        wl_fixed_t x_fixed = wl_fixed_from_double(static_cast<double>(event.x));
        wl_fixed_t y_fixed = wl_fixed_from_double(static_cast<double>(event.y));

        // Define constants for motion events (matching Android's MotionEvent constants)
        const int ACTION_DOWN = 0;
        const int ACTION_UP = 1;
        const int ACTION_MOVE = 2;

        // Serial number, incremented with each unique event
        auto serial = wl_display_next_serial(state.display);

        // Check the action type and call the corresponding wl_touch_* function
        switch (event.action) {
            case ACTION_DOWN:
                state.touchDownId = serial;
                wl_touch_send_down(state.touch, serial, event.timestamp, surface,
                                   state.touchDownId,
                                   x_fixed, y_fixed);
//            wl_pointer_send_enter(state.pointer, serial, surface, x_fixed, y_fixed);
//            wl_pointer_send_motion(state.pointer, event.timestamp, x_fixed, y_fixed);
//            wl_pointer_send_button(state.pointer, serial, event.timestamp, 0, 0);
//            wl_pointer_send_frame(state.pointer);
                wl_display_flush_clients(state.display);
                break;

            case ACTION_UP:
                wl_touch_send_up(state.touch, serial, event.timestamp, state.touchDownId);
                wl_display_flush_clients(state.display);
                break;

            case ACTION_MOVE:
                wl_touch_send_motion(state.touch, event.timestamp, event.pointerId, x_fixed,
                                     y_fixed);
                break;
        }

        // Send a frame event after each touch sequence to signal the end of this touch event group
        wl_touch_send_frame(state.touch);
    }
}

void handle_keyboard_event(uint32_t surface_id, KeyboardEventData event) {
    if (state.keyboard != nullptr) {
        if (!state.sent_enter) {
            state.sent_enter = true;
            auto surface = state.surfaces[surface_id].wl_surface;
            struct wl_array keys = {.size = 0};
            wl_keyboard_send_enter(state.keyboard, wl_display_next_serial(state.display), surface,
                                   &keys);
        }

        auto serial = wl_display_next_serial(state.display);
        wl_keyboard_send_key(state.keyboard, serial, event.timestamp, event.scancode + 8,
                             event.state);

        wl_display_flush_clients(state.display);
    }
}