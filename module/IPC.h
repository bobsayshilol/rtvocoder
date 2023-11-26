#pragma once

#include <cstdint>
#include <optional>

namespace pwv::ipc {

class Message {};

class Client {
  public:
    bool connect(uint16_t port);
    std::optional<Message> read_message();
};

class Server {
  public:
    bool host(uint16_t port);
    void send_message(Message msg);
};

}  // namespace pwv::ipc
