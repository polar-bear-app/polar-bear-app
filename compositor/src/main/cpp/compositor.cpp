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

void render_buffer(struct wl_shm_buffer* waylandBuffer) {
    // Convert the Surface (jobject) to ANativeWindow
    ANativeWindow *androidNativeWindow = ANativeWindow_fromSurface(globalEnv, globalSurface);
    wl_shm_buffer_begin_access(waylandBuffer);

    // Step 2: Get buffer details.
    void *waylandData = wl_shm_buffer_get_data(waylandBuffer);
    int32_t width = wl_shm_buffer_get_width(waylandBuffer);
    int32_t height = wl_shm_buffer_get_height(waylandBuffer);
    int32_t stride = wl_shm_buffer_get_stride(waylandBuffer);
    uint32_t format = wl_shm_buffer_get_format(waylandBuffer);

    // Step 3: Configure the native window.
    ANativeWindow_setBuffersGeometry(androidNativeWindow, width, height, format == WL_SHM_FORMAT_ARGB8888 ? AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM : AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM);

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
    auto socket_name = "wayland-pb";
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
