//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#include "data.h"

#ifndef POLARBEAR_IMPLEMENTATIONS_H
#define POLARBEAR_IMPLEMENTATIONS_H

std::string implement(void (*render)(wl_shm_buffer *));

void run();

void handle_event(TouchEventData event);

#endif //POLARBEAR_IMPLEMENTATIONS_H
