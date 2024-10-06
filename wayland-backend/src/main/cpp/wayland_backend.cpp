#include <jni.h>
#include <string>
#include <unistd.h>
#include "wayland/src/wayland-server.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_wayland_1backend_NativeLib_start(
        JNIEnv* env,
        jobject /* this */) {
    struct wl_display *display = wl_display_create();
    if (!display) {
        fprintf(stderr, "Unable to create Wayland display.\n");
        return (jstring) "";
    }

    const char *socket = wl_display_add_socket_auto(display);
    if (!socket) {
        fprintf(stderr, "Unable to add socket to Wayland display.\n");
        return (jstring) "";
    }

    fprintf(stderr, "Running Wayland display on %s\n", socket);
    wl_display_run(display);

    wl_display_destroy(display);
    return (jstring) "$WAYLAND_DISPLAY=";
}

