#pragma once

#include "SecondOrderFilter.h"

namespace pwv {

class LowPass {
  public:
    LowPass(double sampling_rate, double cutoff_hz);
    ~LowPass();

    void reset(double sampling_rate, double cutoff_hz);

    void process(std::span<float> input) { m_filter.process(input); }

  private:
    LowPass(LowPass const&) = delete;
    LowPass& operator=(LowPass const&) = delete;

  private:
    SecondOrderFilter m_filter;
};

}  // namespace pwv
