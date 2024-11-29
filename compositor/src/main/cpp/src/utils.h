//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#pragma once

#include <string>

using namespace std;

int create_unix_socket(const string &socket_path);

void close_unix_socket(int fd);

int64_t timespec_to_msec(const struct timespec *a);

uint32_t get_current_timestamp();

string jstringToString(JNIEnv *env, jstring jStr);
