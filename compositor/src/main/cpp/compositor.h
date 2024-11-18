//
// Created by Nguyễn Hồng Phát on 20/10/24.
//

#pragma once

#include "data.h"

using namespace std;

string implement(const string& socket_name);

void run(const function<void(const string&, const vector<string>&)>& callJVM);

void handle_event(TouchEventData event);

