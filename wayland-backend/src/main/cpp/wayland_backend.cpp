#include <jni.h>
#include <string>
#include <unistd.h>
#include "wayland/src/wayland-server.h"
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/mman.h>  // For mmap and protection flags
#include <fcntl.h>     // For file control flags (optional)
#include <unistd.h>    // For close()

#define LOG_TAG "WaylandBackend"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct WaylandGlobal {
    struct wl_buffer *buffer;
};

// Declare a global object
struct WaylandGlobal g_wayland_global = {nullptr};

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

wl_display * setup_wayland_backend() {
    struct wl_display *display = wl_display_create();
    if (!display) {
        LOGI("Unable to create Wayland display.");
        return nullptr;
    }
    char *socket_path = "/data/data/app.polarbear/files/archlinux-aarch64/tmp/wayland-0";
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
    return display;
}

void render_buffer(JNIEnv *env, jobject surface) {
    auto width = 1280;
    auto height = 968;

    // Convert the Surface (jobject) to ANativeWindow
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    if (nativeWindow == NULL) {
        // Handle error
        return;
    }

    // Now you can render using the ANativeWindow, for example:
    ANativeWindow_Buffer buffer;

    if (ANativeWindow_lock(nativeWindow, &buffer, NULL) == 0) {
        if (buffer.width == width && buffer.height == height) {
            // Assuming your pixel format is compatible
            memcpy(buffer.bits, g_wayland_global.buffer, height * buffer.stride * 4);
        }
        ANativeWindow_unlockAndPost(nativeWindow);
    }


    // Release the native window when done
    ANativeWindow_release(nativeWindow);
}

jstring run_display(wl_display *display) {
    LOGI("Running Wayland display...");
    wl_display_run(display);

    wl_display_destroy(display);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_wayland_1backend_NativeLib_start(
        JNIEnv *env,
        jobject /* this */,
        jobject surface) {

    auto display = setup_wayland_backend();

    render_buffer(env, surface);

    return run_display(display);
}
