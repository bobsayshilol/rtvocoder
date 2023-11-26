#include "Module.h"

#include "IPC.h"

#include <Vocoder.h>
#include <algorithm>

namespace pwv {

Module::Module(double sampling_rate)
    : m_vocoder(std::make_unique<Vocoder>(20, 40, sampling_rate)) {}

Module::~Module() { m_vocoder.reset(); }

void Module::activate() {
    // Spin up the coms thread.
    m_communication_state.store(ThreadState::Idle, std::memory_order_relaxed);
    m_communication_thread = std::jthread([this] { communication_thread(); });

    // Reset the filter.
    // TODO
}

void Module::process(uint32_t sample_count) {
    // Run the filter.
    m_vocoder->process(std::span{m_input, sample_count},
                       std::span{m_input, sample_count});

    // Copy back only if we need to.
    if (m_input != m_output) {
        std::copy_n(m_input, sample_count, m_output);
    }

    // Read the communication port and tell the thread to use it.
    m_communication_port = *m_port_addr;
    m_communication_state.store(ThreadState::Run, std::memory_order_release);
}

void Module::deactivate() {
    // Tell the thread to stop and wait for it.
    m_communication_state.store(ThreadState::Join, std::memory_order_relaxed);
    m_communication_thread.join();
}

void Module::communication_thread() {
    // Wait until process() has been called at least once so that we can read
    // the port.
    while (m_communication_state.load(std::memory_order_relaxed) ==
           ThreadState::Idle) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // If we're not running then quit.
    if (m_communication_state.load(std::memory_order_acquire) !=
        ThreadState::Run) {
        return;
    }

    // Make the IPC connection.
    ipc::Client client;
    if (!client.connect(m_communication_port)) {
        return;
    }

    // Process messages until we're told to quit.
    while (m_communication_state.load(std::memory_order_relaxed) ==
           ThreadState::Run) {
        auto msg = client.read_message();
        if (!msg) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // TODO
    }
}

}  // namespace pwv
