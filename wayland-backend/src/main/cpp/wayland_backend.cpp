#include <jni.h>
#include <thread>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "compositor.h"

#define LOG_TAG "WaylandBackend"
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


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_wayland_1backend_NativeLib_start(
        JNIEnv *env,
        jobject /* this */,
        jobject surface) {
    auto socket_name = "wayland-0";
    auto compositor = setup_wayland_backend(socket_name);

    render_buffer(env, surface, compositor->wl_shm_buffer);

    std::thread wayland_backend_thread(run_display);
    wayland_backend_thread.detach();
    return env->NewStringUTF(socket_name);
}
