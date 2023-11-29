#pragma once

#include <cstddef>
#include <cstdint>

namespace pwv {

static constexpr std::size_t k_module_version = 1;

struct IModule {
    virtual void process(float const *input, float *output,
                         std::size_t sample_count) = 0;
    virtual ~IModule() = 0;
};

struct ModuleMaker {
    std::size_t version;
    IModule *(*create_module)(double sampling_rate);
};
static_assert(offsetof(ModuleMaker, version) == 0);

}  // namespace pwv

extern "C" pwv::ModuleMaker const pwv_module_maker;
