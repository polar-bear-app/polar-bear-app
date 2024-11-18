#include <jni.h>
#include <thread>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "wayland/src/wayland-server.h"
#include "compositor.h"
#include "utils.h"
#include "wayland/tests/test-compositor.h"
#include "data.h"

#define LOG_TAG "PolarBearCompositor"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;

//void render_buffer(struct wl_shm_buffer *waylandBuffer) {
//    if (globalSurface == nullptr) {
//        return;
//    }
//
//    // Convert the Surface (jobject) to ANativeWindow
//    ANativeWindow *androidNativeWindow = ANativeWindow_fromSurface(globalEnv, globalSurface);
//    wl_shm_buffer_begin_access(waylandBuffer);
//
//    // Step 2: Get buffer details.
//    void *waylandData = wl_shm_buffer_get_data(waylandBuffer);
//    int32_t width = wl_shm_buffer_get_width(waylandBuffer);
//    int32_t height = wl_shm_buffer_get_height(waylandBuffer);
//    int32_t stride = wl_shm_buffer_get_stride(waylandBuffer);
//    uint32_t format = wl_shm_buffer_get_format(waylandBuffer);
//
//    // Step 3: Configure the native window.
//    ANativeWindow_setBuffersGeometry(androidNativeWindow, width, height,
//                                     format == WL_SHM_FORMAT_ARGB8888
//                                     ? AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM
//                                     : AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM);
//
//    // Step 4: Lock the native window buffer for writing.
//    ANativeWindow_Buffer buffer;
//    if (ANativeWindow_lock(androidNativeWindow, &buffer, nullptr) == 0) {
//        // Step 5: Copy the data from the Wayland buffer to the native window buffer.
//        uint8_t *dest = static_cast<uint8_t *>(buffer.bits);
//        uint8_t *src = static_cast<uint8_t *>(waylandData);
//
//        for (int y = 0; y < height; y++) {
//            memcpy(dest, src, width * 4); // 4 bytes per pixel for RGBX_8888.
//            dest += buffer.stride * 4;    // Move to the next line in the native buffer.
//            src += stride;                // Move to the next line in the Wayland buffer.
//        }
//
//        // Step 6: Unlock and post the buffer to display the content.
//        ANativeWindow_unlockAndPost(androidNativeWindow);
//    }
//
//    // Step 7: End access to the Wayland buffer.
//    wl_shm_buffer_end_access(waylandBuffer);
//}


// Function to call the Kotlin method with arguments
const string callJVM(JNIEnv *env, const string &request, const vector<string> &args) {
    // Find the Kotlin class and method
    jclass clazz = env->FindClass("app/polarbear/compositor/NativeLib");
    if (!clazz) {
        return "Class not found";
    }

    jmethodID methodID = env->GetStaticMethodID(clazz, "callJVM", "(Ljava/lang/String;[Ljava/lang/String;)Ljava/lang/String;");
    if (!methodID) {
        return "Method not found";
    }

    // Convert vector<string> to jobjectArray
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray jArgs = env->NewObjectArray(args.size(), stringClass, nullptr);

    for (size_t i = 0; i < args.size(); ++i) {
        jstring jstr = env->NewStringUTF(args[i].c_str());
        env->SetObjectArrayElement(jArgs, i, jstr);
    }

    // Create a jstring for the request
    jstring jRequest = env->NewStringUTF(request.c_str());

    // Call the Kotlin method
    jobject result = env->CallStaticObjectMethod(clazz, methodID, jRequest, jArgs);

    // Handle the result
    string resultStr;
    if (result != nullptr) {
        const char *cResultStr = env->GetStringUTFChars((jstring) result, nullptr);
        resultStr = string(cResultStr);
        env->ReleaseStringUTFChars((jstring) result, cResultStr);
    } else {
        resultStr = nullptr;
    }

    return resultStr;
}

extern "C" JNIEXPORT void JNICALL
Java_app_polarbear_compositor_NativeLib_start(
        JNIEnv *env, jclass clazz, jstring socket_name) {
    implement(jstringToString(env, socket_name));
    run([&env](const string &request, const vector<string> &args) {
        callJVM(env, request, args);
    });
}

extern "C" JNIEXPORT void JNICALL
Java_app_polarbear_compositor_NativeLib_sendTouchEvent(JNIEnv *env, jclass clazz,
                                                       jobject event) {
    auto data = convertToTouchEventData(env, event);
    handle_event(data);
}