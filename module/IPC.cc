#include "IPC.h"

namespace pwv::ipc {

bool Client::connect(uint16_t port) {
    // TODO
    (void)port;
    return false;
}

std::optional<Message> read_message() {
    // TODO
    return std::nullopt;
}

bool Server::host(uint16_t port) {
    // TODO
    (void)port;
    return false;
}

void Server::send_message(Message msg) {
    // TODO
    (void)msg;
}

}  // namespace pwv::ipc
