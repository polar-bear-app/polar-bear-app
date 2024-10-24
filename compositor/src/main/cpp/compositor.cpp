#include <jni.h>
#include <thread>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "wayland/src/wayland-server.h"
#include "implementations.h"
#include "utils.h"
#include "wayland/tests/test-compositor.h"

#define LOG_TAG "PolarBearCompositor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

JNIEnv *globalEnv;
JavaVM *globalVM;
jobject globalSurface;

void render_buffer(struct wl_shm_buffer *shmBuffer) {
    // Convert the Surface (jobject) to ANativeWindow
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(globalEnv, globalSurface);

    if (nativeWindow == NULL) {
        // Handle error
        return;
    }

    // Now you can render using the ANativeWindow, for example:
    ANativeWindow_Buffer buffer;

    if (ANativeWindow_lock(nativeWindow, &buffer, NULL) == 0) {
        // Assuming your pixel format is compatible
        memcpy(buffer.bits, shmBuffer, buffer.height * buffer.stride * 4);
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

    return wl_display;
}

extern "C" JNIEXPORT jstring JNICALL
Java_app_polarbear_compositor_NativeLib_start(
        JNIEnv *env,
        jobject /* this */,
        jobject surface) {
    globalSurface = env->NewGlobalRef(surface);
    auto socket_name = "wayland-0";
    auto wl_display = setup_compositor(socket_name);

    implement(wl_display, render_buffer);

    env->GetJavaVM(&globalVM);
    std::thread compositor_thread([wl_display]() {
        globalVM->AttachCurrentThread(&globalEnv, nullptr);
        LOGI("Running Wayland display...");
        wl_display_run(wl_display);
        wl_display_destroy(wl_display);
        globalEnv->DeleteGlobalRef(globalSurface);
        globalVM->DetachCurrentThread();
    });

    compositor_thread.detach();
    return env->NewStringUTF(socket_name);
}
