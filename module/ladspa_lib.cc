#include "Module.h"

#include <ladspa.h>
#include <memory>

namespace {

constexpr std::size_t k_input_port = 0;
constexpr std::size_t k_output_port = 1;
constexpr std::size_t k_control_port = 2;
constexpr std::size_t k_num_ports = 3;

LADSPA_PortDescriptor const g_ports[] = {
    /* k_input_port */ LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
    /* k_output_port */ LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
    /* k_control_port */ LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
};
static_assert(std::size(g_ports) == k_num_ports);

char const* const g_port_names[] = {
    /* k_input_port */ "Input audio buffer",
    /* k_output_port */ "Output audio buffer",
    /* k_control_port */ "control 0",
};
static_assert(std::size(g_port_names) == k_num_ports);

LADSPA_PortRangeHint const g_port_hints[] = {
    /* k_input_port */ {0, 0, 0},
    /* k_output_port */ {0, 0, 0},
    /* k_control_port */
    {LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE, 0, 65535},
};
static_assert(std::size(g_port_hints) == k_num_ports);

auto to_module(LADSPA_Handle handle) {
    return static_cast<pwv::Module*>(handle);
}

LADSPA_Handle ladspa_instantiate(LADSPA_Descriptor const* descriptor,
                                 unsigned long sample_rate) {
    if (descriptor == nullptr ||
        descriptor->instantiate == ladspa_instantiate) {
        return nullptr;
    }

    auto module = std::make_unique<pwv::Module>(sample_rate);
    return module.release();
}

void ladspa_connect_port(LADSPA_Handle instance, unsigned long port,
                         LADSPA_Data* data_location) {
    auto* module = to_module(instance);
    switch (port) {
        case k_input_port:
            module->set_input_buffer(data_location);
            break;
        case k_output_port:
            module->set_output_buffer(data_location);
            break;
        case k_control_port:
            module->set_control_port(data_location);
            break;
    }
}

void ladspa_activate(LADSPA_Handle instance) {
    auto* module = to_module(instance);
    module->activate();
}

void ladspa_run(LADSPA_Handle instance, unsigned long sample_count) {
    auto* module = to_module(instance);
    module->process(sample_count);
}

void ladspa_deactivate(LADSPA_Handle instance) {
    auto* module = to_module(instance);
    module->deactivate();
}

void ladspa_cleanup(LADSPA_Handle instance) {
    std::unique_ptr<pwv::Module> module(to_module(instance));
}

LADSPA_Descriptor const g_descriptor{
    .UniqueID = 0x1234 /* TODO */,

    .Label = "TODO",

    .Properties =
        LADSPA_PROPERTY_INPLACE_BROKEN /* | LADSPA_PROPERTY_HARD_RT_CAPABLE */,

    .Name = "Vocoder thing",

    .Maker = "me",

    .Copyright = "None",

    .PortCount = k_num_ports,

    .PortDescriptors = g_ports,

    .PortNames = g_port_names,

    .PortRangeHints = g_port_hints,

    .ImplementationData = nullptr,

    .instantiate = ladspa_instantiate,

    .connect_port = ladspa_connect_port,

    .activate = ladspa_activate,

    .run = ladspa_run,

    .run_adding = nullptr,

    .set_run_adding_gain = nullptr,

    .deactivate = ladspa_deactivate,

    .cleanup = ladspa_cleanup,
};

}  // namespace

LADSPA_Descriptor const* ladspa_descriptor(unsigned long index) {
    if (index == 0) {
        return &g_descriptor;
    }
    return nullptr;
}
