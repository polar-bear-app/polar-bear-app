//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#include <string>
#include <vector>

#include <jni.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <android/log.h>
#include <sys/mman.h>

#define LOG_TAG "PolarBearCompositorUtils"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace std;

int create_unix_socket(const string &socket_name) {
    int sock_fd;
    struct sockaddr_un addr;

    // Create the socket
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        LOGI("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    // Zero out the address structure
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // Copy the socket path to the address structure
    auto socket_path =
            string("/data/data/app.polarbear/files/archlinux-aarch64/tmp/") + socket_name;
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0'; // Ensure null termination

    // Remove any existing file at the socket path
    if (unlink(socket_path.c_str()) < 0 && errno != ENOENT) {
        LOGI("Failed to remove existing socket file: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    // Bind the socket to the specified path
    if (bind(sock_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOGI("bind() failed with error: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    // Set the socket to listen for incoming connections
    if (listen(sock_fd, 128) < 0) {
        LOGI("listen() failed with error: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

void close_unix_socket(int fd) {
    close(fd);
}

int64_t timespec_to_msec(const struct timespec *a) {
    return (int64_t) a->tv_sec * 1000 + a->tv_nsec / 1000000;
}

uint32_t get_current_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint32_t>(ts.tv_sec) * 1000 + static_cast<uint32_t>(ts.tv_nsec / 1000000);
}

string jstringToString(JNIEnv *env, jstring jStr) {
    if (!jStr) {
        return ""; // Return an empty string if the jstring is null
    }

    // Convert the jstring to a UTF-8 C-style string
    const char *chars = env->GetStringUTFChars(jStr, nullptr);
    if (!chars) {
        return ""; // Return an empty string if the conversion fails
    }

    // Copy the C-style string into a string
    string result(chars);

    // Release the UTF-8 C-style string
    env->ReleaseStringUTFChars(jStr, chars);

    return result;
}
