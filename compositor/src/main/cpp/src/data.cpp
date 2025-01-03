//
// Created by Nguyễn Hồng Phát on 3/11/24.
//

#include <jni.h>
#include "data.h"
#include "utils.h"

TouchEventData convertToTouchEventData(JNIEnv *env, jobject eventData) {
    TouchEventData data{};

    // Get the class and field IDs
    jclass eventClass = env->GetObjectClass(eventData);
    jfieldID actionField = env->GetFieldID(eventClass, "action", "I");
    jfieldID pointerIdField = env->GetFieldID(eventClass, "pointerIndex", "I");
    jfieldID xField = env->GetFieldID(eventClass, "x", "F");
    jfieldID yField = env->GetFieldID(eventClass, "y", "F");

    // Fill in the struct fields
    data.action = env->GetIntField(eventData, actionField);
    data.pointerId = env->GetIntField(eventData, pointerIdField);
    data.x = env->GetFloatField(eventData, xField);
    data.y = env->GetFloatField(eventData, yField);
    data.timestamp = get_current_timestamp();

    return data;
}

KeyboardEventData convertToKeyboardEventData(JNIEnv *env, jobject eventData) {
    KeyboardEventData data{};

    // Get the class and field IDs
    jclass eventClass = env->GetObjectClass(eventData);
    jfieldID keyCodeField = env->GetFieldID(eventClass, "keyCode", "I");
    jfieldID scanCodeField = env->GetFieldID(eventClass, "scanCode", "I");
    jfieldID stateField = env->GetFieldID(eventClass, "state", "I");

    // Fill in the struct fields
    data.keyCode = env->GetIntField(eventData, keyCodeField);
    data.scanCode = env->GetIntField(eventData, scanCodeField);
    data.state = env->GetIntField(eventData, stateField);
    data.timestamp = get_current_timestamp();

    return data;
}