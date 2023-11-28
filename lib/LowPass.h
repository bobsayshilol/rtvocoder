#pragma once

#include "SecondOrderFilter.h"

namespace pwv {

class LowPass {
  public:
    static constexpr std::size_t k_block_size = SecondOrderFilter::k_block_size;

  public:
    LowPass(double sampling_rate, double cutoff_hz);
    ~LowPass();
    LowPass(LowPass&&) = default;
    LowPass& operator=(LowPass&&) = default;

    void reset(double sampling_rate, double cutoff_hz);

    void process(std::span<float> input) { m_filter.process(input); }
    void process_block(std::span<float> input) {
        m_filter.process_block(input);
    }

  private:
    LowPass(LowPass const&) = delete;
    LowPass& operator=(LowPass const&) = delete;

  private:
    SecondOrderFilter m_filter;
};

}  // namespace pwv
