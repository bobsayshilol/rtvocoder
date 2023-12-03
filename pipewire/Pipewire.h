#pragma once

#include <pipewire/pipewire.h>
#include <string>
#include <vector>

namespace pipewire {

struct Device {
    std::string description;
    std::string name;
    int id;
};

enum class Direction { In, Out };

std::vector<Device> fetch_devices(pw_context *context, pw_main_loop *loop,
                                  Direction dir);

}  // namespace pipewire
