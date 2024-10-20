#include <jni.h>
#include <thread>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "wayland/src/wayland-server.h"
#include "implementations.h"
#include "utils.h"

#define LOG_TAG "PolarBearCompositor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

void render_buffer(JNIEnv *env, jobject surface, struct wl_shm_buffer *shmBuffer) {
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
            memcpy(buffer.bits, shmBuffer, height * buffer.stride * 4);
        }
        ANativeWindow_unlockAndPost(nativeWindow);
    }

    // Release the native window when done
    ANativeWindow_release(nativeWindow);
}

wl_display *setup_compositor(const char *socket_name) {
    redirect_stds();

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

    implement(wl_display);

    return wl_display;
}

extern "C" JNIEXPORT jstring JNICALL
Java_app_polarbear_compositor_NativeLib_start(
        JNIEnv *env,
        jobject /* this */,
        jobject surface) {
    auto socket_name = "wayland-0";
    auto display = setup_compositor(socket_name);

    //    render_buffer(env, surface, compositor->shm_buffer);

    std::thread compositor_thread([display]() {
        LOGI("Running Wayland display...");
        wl_display_run(display);
        wl_display_destroy(display);
    });
    compositor_thread.detach();
    return env->NewStringUTF(socket_name);
}
