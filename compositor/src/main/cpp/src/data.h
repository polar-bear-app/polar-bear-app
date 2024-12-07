//
// Created by Nguyễn Hồng Phát on 3/11/24.
//

#pragma once

#include <jni.h>

struct TouchEventData {
    int action;
    int pointerId;
    float x;
    float y;
    long timestamp;
};

struct KeyboardEventData {
    int keyCode;
    int scanCode;
    int state;
    long timestamp;
};

TouchEventData convertToTouchEventData(JNIEnv *env, jobject eventData);

KeyboardEventData convertToKeyboardEventData(JNIEnv *env, jobject eventData);