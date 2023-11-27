#pragma once

#include "SecondOrderFilter.h"

namespace pwv {

class BandPass {
  public:
    BandPass(double sampling_rate, double hz, double q);
    ~BandPass();
    BandPass(BandPass&&) = default;
    BandPass& operator=(BandPass&&) = default;

    void reset(double sampling_rate, double hz, double q);

    void process(std::span<float> input) { m_filter.process(input); }

    static double approximate_q(double sampling_rate, int num_bands);

  private:
    BandPass(BandPass const&) = delete;
    BandPass& operator=(BandPass const&) = delete;

  private:
    SecondOrderFilter m_filter;
};

}  // namespace pwv
