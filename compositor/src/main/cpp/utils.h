//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#ifndef POLARBEAR_UTILS_H
#define POLARBEAR_UTILS_H

#include <string>

int create_unix_socket(const std::string &socket_path);

void close_unix_socket(int fd);

int64_t timespec_to_msec(const struct timespec *a);

uint32_t get_current_timestamp();

#endif //POLARBEAR_UTILS_H
