//
// Created by Nguyễn Hồng Phát on 12/10/24.
//
#include <string>
#include <android/log.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h> // For what?

#include "wayland/src/wayland-server.h"

#define LOG_TAG "PolarBearCompositor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct polar_bear_compositor {
    struct wl_display *wl_display;
    struct wl_shm_buffer *wl_shm_buffer;
};

struct polar_bear_compositor pb_compositor = {};

int create_unix_socket(const std::string &socket_path) {
    int sock_fd;
    struct sockaddr_un addr;

    // Create the socket
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        LOGI("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    // Zero out the address structure
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // Copy the socket path to the address structure
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0'; // Ensure null termination

    // Remove any existing file at the socket path
    if (unlink(socket_path.c_str()) < 0 && errno != ENOENT) {
        LOGI("Failed to remove existing socket file: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    // Bind the socket to the specified path
    if (bind(sock_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOGI("bind() failed with error: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    // Set the socket to listen for incoming connections
    if (listen(sock_fd, 128) < 0) {
        LOGI("listen() failed with error: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

/**
 * === [Global Implementation] Compositor ===
 */
static void
surface_attach(struct wl_client *client,
               struct wl_resource *resource,
               struct wl_resource *buffer_resource, int32_t sx, int32_t sy)
{
    if (buffer_resource) {
        auto buffer = wl_shm_buffer_get(buffer_resource);
        if (buffer == NULL) {
            wl_client_post_no_memory(client);
            return;
        }
        pb_compositor.wl_shm_buffer = buffer;
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

//static void
//surface_damage(struct wl_client *client,
//               struct wl_resource *resource,
//               int32_t x, int32_t y, int32_t width, int32_t height)
//{
//    struct weston_surface *surface = wl_resource_get_user_data(resource);
//
//    if (width <= 0 || height <= 0)
//        return;
//
//    pixman_region32_union_rect(&surface->pending.damage_surface,
//                               &surface->pending.damage_surface,
//                               x, y, width, height);
//}
//static void
//surface_commit(struct wl_client *client, struct wl_resource *resource)
//{
//    struct weston_surface *surface = wl_resource_get_user_data(resource);
//    struct weston_subsurface *sub = weston_surface_to_subsurface(surface);
//    enum weston_surface_status status;
//
//    if (!weston_surface_is_pending_viewport_source_valid(surface)) {
//        assert(surface->viewport_resource);
//
//        wl_resource_post_error(surface->viewport_resource,
//                               WP_VIEWPORT_ERROR_OUT_OF_BUFFER,
//                               "wl_surface@%d has viewport source outside buffer",
//                               wl_resource_get_id(resource));
//        return;
//    }
//
//    if (!weston_surface_is_pending_viewport_dst_size_int(surface)) {
//        assert(surface->viewport_resource);
//
//        wl_resource_post_error(surface->viewport_resource,
//                               WP_VIEWPORT_ERROR_BAD_SIZE,
//                               "wl_surface@%d viewport dst size not integer",
//                               wl_resource_get_id(resource));
//        return;
//    }
//
//    if (surface->pending.acquire_fence_fd >= 0) {
//        assert(surface->synchronization_resource);
//
//        if (!surface->pending.buffer) {
//            fd_clear(&surface->pending.acquire_fence_fd);
//            wl_resource_post_error(surface->synchronization_resource,
//                                   ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
//                                   "wl_surface@%"PRIu32" no buffer for synchronization",
//                                   wl_resource_get_id(resource));
//            return;
//        }
//
//        if (surface->pending.buffer->type == WESTON_BUFFER_SHM) {
//            fd_clear(&surface->pending.acquire_fence_fd);
//            wl_resource_post_error(surface->synchronization_resource,
//                                   ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_UNSUPPORTED_BUFFER,
//                                   "wl_surface@%"PRIu32" unsupported buffer for synchronization",
//                                   wl_resource_get_id(resource));
//            return;
//        }
//    }
//
//    if (surface->pending.buffer_release_ref.buffer_release &&
//        !surface->pending.buffer) {
//        assert(surface->synchronization_resource);
//
//        wl_resource_post_error(surface->synchronization_resource,
//                               ZWP_LINUX_SURFACE_SYNCHRONIZATION_V1_ERROR_NO_BUFFER,
//                               "wl_surface@%"PRIu32" no buffer for synchronization",
//                               wl_resource_get_id(resource));
//        return;
//    }
//
//    if (sub) {
//        status = weston_subsurface_commit(sub);
//    } else {
//        status = WESTON_SURFACE_CLEAN;
//        wl_list_for_each(sub, &surface->subsurface_list, parent_link) {
//            if (sub->surface != surface)
//                status |= weston_subsurface_parent_commit(sub, 0);
//        }
//        status |= weston_surface_commit(surface);
//    }
//
//    if (status & WESTON_SURFACE_DIRTY_SUBSURFACE_CONFIG)
//        surface->compositor->view_list_needs_rebuild = true;
//}

/**
 * === Registering all globals ===
 */
struct PolarBearGlobal {
    const wl_interface *interface;
    const void *implementation;
};

static const struct wl_surface_interface surface_interface = {
        nullptr, //surface_destroy
        surface_attach, //surface_attach
        nullptr, //surface_damage
        nullptr, //surface_frame
        nullptr, //surface_set_opaque_region
        nullptr, //surface_set_input_region
        nullptr, //surface_commit
        nullptr, //surface_set_buffer_transform
        nullptr, //surface_set_buffer_scale
        nullptr, //surface_damage_buffer
        nullptr, //surface_offset
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

static const struct wl_compositor_interface compositor_interface = {
        compositor_create_surface,
        compositor_create_region
};

std::vector<PolarBearGlobal> GLOBALS = {
        {&wl_compositor_interface, &compositor_interface},
};


static void bind_generic(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    auto global = static_cast<PolarBearGlobal *>(data);
    struct wl_resource *resource = wl_resource_create(client, global->interface, version, id);
    if (resource == nullptr) {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, global->implementation, data, nullptr);
}

void register_globals() {
    for (auto &global: GLOBALS) {
        if (!wl_global_create(pb_compositor.wl_display, global.interface, global.interface->version,
                              &global, bind_generic)) {
            LOGI("Global creation for interface failed.");
            throw std::exception();
        }
        LOGI("Global created for interface %s v%d.", global.interface->name,
             global.interface->version);
    }
}

// Custom function to capture stderr and log it using LOGI
// File descriptors for the stdout and stderr pipes
int stdout_pipe_fds[2];
int stderr_pipe_fds[2];

// Thread function to read from the pipe and log it to LOGI
void *log_pipe_thread(void *arg) {
    int fd = *(int *) arg;  // The file descriptor to read from
    char buffer[256];
    ssize_t count;

    // Continuously read from the pipe and log the output to LOGI
    while ((count = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[count] = '\0';  // Null-terminate the buffer
        LOGI("%s", buffer);    // Log the output using LOGI
    }
    return nullptr;
}

// Function to create a pipe and redirect stdout or stderr to it
void redirect_fd_to_pipe(int fd, int pipe_fds[2]) {
    // Create a pipe
    if (pipe(pipe_fds) == -1) {
        LOGI("Failed to create pipe for fd %d redirection", fd);
        return;
    }

    // Redirect the file descriptor to the write end of the pipe
    if (dup2(pipe_fds[1], fd) == -1) {
        LOGI("Failed to redirect fd %d to pipe", fd);
        return;
    }

    // Create a thread to read from the pipe and log the output
    pthread_t thread;
    if (pthread_create(&thread, nullptr, log_pipe_thread, (void *) &pipe_fds[0]) != 0) {
        LOGI("Failed to create thread for logging fd %d", fd);
    }

    // Detach the thread, so it runs independently
    pthread_detach(thread);
}

// Custom logging function
void log_wayland_debug_output(const char *msg) {
    LOGI("%s", msg);  // Print Wayland debug logs using LOGI
}

polar_bear_compositor *setup_wayland_backend(const char *socket_name) {
//    setenv("WAYLAND_DEBUG", "1", 1);  // Set WAYLAND_DEBUG=1 for verbose logs
    // Redirect stdout and stderr to LOGI
    redirect_fd_to_pipe(STDOUT_FILENO, stdout_pipe_fds);  // Redirect stdout
    redirect_fd_to_pipe(STDERR_FILENO, stderr_pipe_fds);  // Redirect stderr


    struct wl_display *display = wl_display_create();
    if (!display) {
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

    if (wl_display_add_socket_fd(display, sock_fd) < 0) {
        LOGI("Unable to add socket to Wayland display.");
        close(sock_fd);
        return nullptr;
    }
    pb_compositor.wl_display = display;

    register_globals();

    wl_display_init_shm(display);
    wl_display_add_shm_format(display, WL_SHM_FORMAT_XBGR8888);
    wl_display_add_shm_format(display, WL_SHM_FORMAT_ABGR8888);

    return &pb_compositor;
}

void run_display() {
    auto display = pb_compositor.wl_display;

    LOGI("Running Wayland display...");
    wl_display_run(display);

    wl_display_destroy(display);
}
