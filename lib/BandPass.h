#pragma once

#include "SecondOrderFilter.h"

namespace pwv {

class BandPass {
  public:
    static constexpr std::size_t k_block_size = SecondOrderFilter::k_block_size;

  public:
    BandPass(double sampling_rate, double hz, double q);
    ~BandPass();
    BandPass(BandPass&&) = default;
    BandPass& operator=(BandPass&&) = default;

    void reset(double sampling_rate, double hz, double q);

    void process(std::span<float> input) { m_filter.process(input); }
    void process_block(std::span<float> input) {
        m_filter.process_block(input);
    }

    static double approximate_q(double sampling_rate, int num_bands);

  private:
    BandPass(BandPass const&) = delete;
    BandPass& operator=(BandPass const&) = delete;

  private:
    SecondOrderFilter m_filter;
};

}  // namespace pwv
