#include <jni.h>
#include <string>
#include <unistd.h>
#include "wayland/src/wayland-server.h"
#include <android/log.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>

#define LOG_TAG "WaylandBackend"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct my_state {

};

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
    if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
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

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_wayland_1backend_NativeLib_start(
        JNIEnv* env,
        jobject /* this */) {
    struct wl_display *display = wl_display_create();
    if (!display) {
        LOGI("Unable to create Wayland display.");
        return (jstring) "";
    }

    struct my_state state = {

    };

    char *socket_path = "/data/data/app.polarbear/files/archlinux-aarch64/tmp/wayland-0";
    int sock_fd = create_unix_socket(socket_path);
    if (sock_fd < 0) {
        LOGI("Failed to create Unix socket.");
        return (jstring) "";
    }

    if (!wl_display_add_socket_fd(display, sock_fd) < 0) {
        LOGI( "Unable to add socket to Wayland display.");
        close(sock_fd);
        return (jstring) "";
    }

    LOGI("Running Wayland display on %s", socket_path);
    wl_display_run(display);

    wl_display_destroy(display);
    return (jstring) "$WAYLAND_DISPLAY=";
}
