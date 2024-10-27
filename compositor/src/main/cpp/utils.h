//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#include <__fwd/string.h>

#ifndef POLARBEAR_UTILS_H
#define POLARBEAR_UTILS_H

#endif //POLARBEAR_UTILS_H

int create_unix_socket(const std::string &socket_path);

void close_unix_socket(int fd);

void redirect_stds();

int64_t timespec_to_msec(const struct timespec *a);