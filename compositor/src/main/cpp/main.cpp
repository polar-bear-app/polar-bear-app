#include <cassert>
#include <thread>
#include <jni.h>
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

// Function to call the Kotlin method with arguments
const string callJVM(JNIEnv *env, jobject thiz, const string &request, const vector<string> &args) {
    // Find the Kotlin class
    jclass clazz = env->GetObjectClass(thiz);  // Use GetObjectClass for instance methods
    assert(clazz);

    // Find the method ID for the instance method
    jmethodID methodID = env->GetMethodID(clazz, "callJVM",
                                          "(Ljava/lang/String;[Ljava/lang/String;)Ljava/lang/Object;");
    assert(methodID); // Check the callJVM method in NativeLib.kt

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
    jobject result = env->CallObjectMethod(thiz, methodID, jRequest, jArgs);
    assert(result != nullptr);

    // Handle the result
    const char *cResultStr = env->GetStringUTFChars((jstring) result, nullptr);
    string resultStr = string(cResultStr);
    env->ReleaseStringUTFChars((jstring) result, cResultStr);

    return resultStr;
}

extern "C"
JNIEXPORT void JNICALL
Java_app_polarbear_compositor_NativeLib_start(JNIEnv *env, jobject thiz, jstring socket_name) {
    implement(jstringToString(env, socket_name));
    run([&env, &thiz](const string &request, const vector<string> &args) {
        return callJVM(env, thiz, request, args);
    });
}

extern "C"
JNIEXPORT void JNICALL
Java_app_polarbear_compositor_NativeLib_setSurface(JNIEnv *env, jobject thiz, jint id,
                                                   jobject surface) {
    ANativeWindow *androidNativeWindow = ANativeWindow_fromSurface(env, surface);
    set_surface((uint32_t) id, androidNativeWindow);
}

extern "C"
JNIEXPORT void JNICALL
Java_app_polarbear_compositor_NativeLib_sendTouchEvent(JNIEnv *env, jobject thiz, jint surface_id,
                                                       jobject event) {
    handle_event((uint32_t) surface_id, convertToTouchEventData(env, event));
}