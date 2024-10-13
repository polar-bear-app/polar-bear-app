//
// Created by Nguyễn Hồng Phát on 12/10/24.
//

#ifndef POLARBEAR_COMPOSITOR_H
#define POLARBEAR_COMPOSITOR_H

#endif //POLARBEAR_COMPOSITOR_H

struct polar_bear_compositor {
    struct wl_display *wl_display;
    struct wl_shm_buffer *wl_shm_buffer;
};

polar_bear_compositor *setup_wayland_backend(const char *socket_name);

void run_display();