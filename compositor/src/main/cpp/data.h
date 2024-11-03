//
// Created by Nguyễn Hồng Phát on 3/11/24.
//

#include <jni.h>

#ifndef POLARBEAR_DATA_H
#define POLARBEAR_DATA_H

struct TouchEventData {
    int action;
    int pointerId;
    float x;
    float y;
    long timestamp;
};

TouchEventData convertToTouchEventData(JNIEnv* env, jobject eventData);

#endif //POLARBEAR_DATA_H
