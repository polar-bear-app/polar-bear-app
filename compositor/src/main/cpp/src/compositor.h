//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#pragma once

#include "data.h"

using namespace std;

const string implement(const string &socket_name);

void run(const function<string(const string &, const vector<string> &)> &callJVM);

void set_surface(uint32_t id, ANativeWindow *pWindow);

void handle_touch_event(uint32_t surface_id, TouchEventData event);

void handle_keyboard_event(uint32_t surface_id, KeyboardEventData event);
