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

TouchEventData convertToTouchEventData(JNIEnv *env, jobject eventData);
