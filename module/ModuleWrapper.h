#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

namespace pwv {

class ModuleWrapper {
    struct ModuleHolder;

  public:
    explicit ModuleWrapper(double sampling_rate);
    ~ModuleWrapper();

    void set_input_buffer(float* input) { m_input = input; }
    void set_output_buffer(float* output) { m_output = output; }
    void set_control_port(float* port_addr) { m_port_addr = port_addr; }

    void activate();
    void process(uint32_t sample_count);
    void deactivate();

  private:
    ModuleWrapper(ModuleWrapper const&) = delete;
    ModuleWrapper& operator=(ModuleWrapper const&) = delete;

  private:
    enum class ThreadState { Idle, Run, Join };
    void communication_thread();
    std::jthread m_communication_thread;
    std::atomic<ThreadState> m_communication_state{ThreadState::Idle};
    uint16_t m_communication_port = 0;

  private:
    double const m_sampling_rate;
    float* m_input = nullptr;
    float* m_output = nullptr;
    float* m_port_addr = nullptr;
    std::unique_ptr<ModuleHolder> m_module;

  private:
    enum class ModuleQueueState { None, QueuedIn, QueuedOut };
    std::atomic<ModuleQueueState> m_module_queue_state;
    std::unique_ptr<ModuleHolder> m_module_queue;
    bool reload_module();
};

}  // namespace pwv
