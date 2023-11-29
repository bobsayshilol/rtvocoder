#include "ModuleWrapper.h"

#include "IModule.h"
#include "IPC.h"

#include <algorithm>
#include <cassert>
#include <dlfcn.h>

namespace pwv {

struct ModuleWrapper::ModuleHolder {
    std::unique_ptr<IModule> module;
    void *handle = nullptr;

    ~ModuleHolder() {
        // Reset the module before we unload the lib.
        module.reset();
        if (handle) {
            dlclose(handle);
        }
    }
};

ModuleWrapper::ModuleWrapper(double sampling_rate)
    : m_sampling_rate(sampling_rate) {}

ModuleWrapper::~ModuleWrapper() {
    m_module_queue.reset();
    m_module.reset();
}

void ModuleWrapper::activate() {
    // Spin up the coms thread.
    m_communication_state.store(ThreadState::Idle, std::memory_order_relaxed);
    m_communication_thread = std::jthread([this] { communication_thread(); });
}

void ModuleWrapper::process(uint32_t sample_count) {
    // If there's a new module, swap it in. The coms thread will destroy it.
    ModuleQueueState const queue_state =
        m_module_queue_state.load(std::memory_order_acquire);
    if (queue_state == ModuleQueueState::QueuedIn) {
        m_module.swap(m_module_queue);
        m_module_queue_state.store(ModuleQueueState::QueuedOut,
                                   std::memory_order_release);
    }

    // Run the filter.
    if (m_module) {
        m_module->module->process(m_input, m_output, sample_count);
    } else {
        // No filter set, so just copy the data unmodified.
        std::copy_n(m_input, sample_count, m_output);
    }

    // Read the communication port and tell the thread to use it.
    m_communication_port = *m_port_addr;
    m_communication_state.store(ThreadState::Run, std::memory_order_release);
}

void ModuleWrapper::deactivate() {
    // Tell the thread to stop and wait for it.
    m_communication_state.store(ThreadState::Join, std::memory_order_relaxed);
    m_communication_thread.join();
}

void ModuleWrapper::communication_thread() {
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
        // Pop and clear the queue if there's something in it.
        if (m_module_queue_state.load(std::memory_order_acquire) ==
            ModuleQueueState::QueuedOut) {
            m_module_queue.reset();
            m_module_queue_state.store(ModuleQueueState::None,
                                       std::memory_order_relaxed);
        }

        // Process the next message
        auto msg = client.read_message();
        if (!msg) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // TODO
    }
}

bool ModuleWrapper::reload_module() {
    // Can't reload if we're in the process of switching modules.
    if (m_module_queue_state.load(std::memory_order_relaxed) !=
        ModuleQueueState::None) {
        return false;
    }

    // Load the lib.
    auto module = std::make_unique<ModuleHolder>();
    module->handle = dlopen("libinternal_vocoder_module.so", RTLD_NOW);
    if (!module->handle) {
        return false;
    }

    // Extract the maker symbol from the lib.
    auto *maker = reinterpret_cast<pwv::ModuleMaker const *>(
        dlsym(module->handle, "pwv_module_maker"));
    if (!maker || maker->version != k_module_version) {
        return false;
    }

    // Create the module.
    module->module.reset(maker->create_module(m_sampling_rate));
    if (!module->module) {
        return false;
    }

    // Pass the new module to be used on the next processor loop.
    m_module_queue = std::move(module);
    m_module_queue_state.store(ModuleQueueState::QueuedIn,
                               std::memory_order_release);
    return true;
}

}  // namespace pwv
