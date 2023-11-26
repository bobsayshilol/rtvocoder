#include "Module.h"

#include <lv2.h>
#include <memory>

namespace {

constexpr uint32_t k_input_port = 0;   /* lv2:AudioPort */
constexpr uint32_t k_output_port = 1;  /* lv2:AudioPort */
constexpr uint32_t k_control_port = 2; /* TODO */

auto to_module(LV2_Handle handle) { return static_cast<pwv::Module*>(handle); }

LV2_Handle lv2_instantiate(const struct LV2_Descriptor* descriptor,
                           double sample_rate, char const* bundle_path,
                           LV2_Feature const* const* features) {
    if (descriptor == nullptr || descriptor->instantiate == lv2_instantiate) {
        return nullptr;
    }

    // TODO
    (void)bundle_path;
    (void)features;

    auto module = std::make_unique<pwv::Module>(sample_rate);
    return module.release();
}

void lv2_connect_port(LV2_Handle instance, uint32_t port, void* data_location) {
    auto* module = to_module(instance);
    switch (port) {
        case k_input_port:
            module->set_input_buffer(static_cast<float*>(data_location));
            break;
        case k_output_port:
            module->set_output_buffer(static_cast<float*>(data_location));
            break;
        case k_control_port:
            // module->set_option(data_location); // TODO
            break;
    }
}

void lv2_activate(LV2_Handle instance) {
    auto* module = to_module(instance);
    module->activate();
}

void lv2_run(LV2_Handle instance, uint32_t sample_count) {
    auto* module = to_module(instance);
    module->process(sample_count);
}

void lv2_deactivate(LV2_Handle instance) {
    auto* module = to_module(instance);
    module->deactivate();
}

void lv2_cleanup(LV2_Handle instance) {
    std::unique_ptr<pwv::Module> module(to_module(instance));
}

void const* lv2_extension_data(char const* uri) {
    // TODO
    (void)uri;
    return nullptr;
}

LV2_Descriptor const g_descriptor{
    "TODO",  lv2_instantiate, lv2_connect_port, lv2_activate,
    lv2_run, lv2_deactivate,  lv2_cleanup,      lv2_extension_data,
};

}  // namespace

LV2_Descriptor const* lv2_descriptor(uint32_t index) {
    if (index == 0) {
        return &g_descriptor;
    }
    return nullptr;
}
