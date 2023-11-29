#pragma once

#include "IModule.h"

#include <Vocoder.h>

namespace pwv {

class Module : public IModule {
  public:
    explicit Module(double sampling_rate);
    ~Module() override;

    void process(float const *input, float *output,
                 std::size_t sample_count) override;

  private:
    Module(Module const &) = delete;
    Module &operator=(Module const &) = delete;

  private:
    VocoderRT m_vocoder;
};

}  // namespace pwv
