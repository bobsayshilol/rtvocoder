#include "Module.h"

#include <memory>

namespace pwv {

Module::Module(double sampling_rate) : m_vocoder(20, 40, sampling_rate) {}

Module::~Module() {}

void Module::process(float const *input, float *output,
                     std::size_t sample_count) {
    // Run the filter.
    m_vocoder.process(input, input, sample_count, output);
}

}  // namespace pwv

extern "C" pwv::ModuleMaker const pwv_module_maker{
    pwv::k_module_version,
    [](double sampling_rate) -> pwv::IModule * {
        return std::make_unique<pwv::Module>(sampling_rate).release();
    },
};
