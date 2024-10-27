//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#include <string>
#include <errno.h>
#include <unistd.h>
#include <android/log.h>
#include <sys/socket.h>
#include <sys/un.h>

#define LOG_TAG "PolarBearCompositorUtils"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

int create_unix_socket(const std::string &socket_path) {
    int sock_fd;
    struct sockaddr_un addr;

    // Create the socket
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        LOGI("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    // Zero out the address structure
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // Copy the socket path to the address structure
    strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0'; // Ensure null termination

    // Remove any existing file at the socket path
    if (unlink(socket_path.c_str()) < 0 && errno != ENOENT)
    {
        LOGI("Failed to remove existing socket file: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    // Bind the socket to the specified path
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOGI("bind() failed with error: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    // Set the socket to listen for incoming connections
    if (listen(sock_fd, 128) < 0)
    {
        LOGI("listen() failed with error: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

void close_unix_socket(int fd) {
    close(fd);
}

// Custom function to capture stderr and log it using LOGI
// File descriptors for the stdout and stderr pipes
int stdout_pipe_fds[2];
int stderr_pipe_fds[2];

// Thread function to read from the pipe and log it to LOGI
void *log_pipe_thread(void *arg)
{
    int fd = *(int *)arg; // The file descriptor to read from
    char buffer[256];
    ssize_t count;

    // Continuously read from the pipe and log the output to LOGI
    while ((count = read(fd, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[count] = '\0'; // Null-terminate the buffer
        LOGI("%s", buffer);   // Log the output using LOGI
    }
    return nullptr;
}

// Function to create a pipe and redirect stdout or stderr to it
void redirect_fd_to_pipe(int fd, int pipe_fds[2])
{
    // Create a pipe
    if (pipe(pipe_fds) == -1)
    {
        LOGI("Failed to create pipe for fd %d redirection", fd);
        return;
    }

    // Redirect the file descriptor to the write end of the pipe
    if (dup2(pipe_fds[1], fd) == -1)
    {
        LOGI("Failed to redirect fd %d to pipe", fd);
        return;
    }

    // Create a thread to read from the pipe and log the output
    pthread_t thread;
    if (pthread_create(&thread, nullptr, log_pipe_thread, (void *)&pipe_fds[0]) != 0)
    {
        LOGI("Failed to create thread for logging fd %d", fd);
    }

    // Detach the thread, so it runs independently
    pthread_detach(thread);
}

void redirect_stds() {
    // Redirect stdout and stderr to LOGI
    redirect_fd_to_pipe(STDOUT_FILENO, stdout_pipe_fds); // Redirect stdout
    redirect_fd_to_pipe(STDERR_FILENO, stderr_pipe_fds); // Redirect stderr
}

int64_t timespec_to_msec(const struct timespec *a) {
    return (int64_t)a->tv_sec * 1000 + a->tv_nsec / 1000000;
}
