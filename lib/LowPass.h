#pragma once

#include <span>

namespace pwv {

class LowPass {
  public:
    LowPass(float sampling_rate, float cutoff_hz);
    ~LowPass();

    void reset(float sampling_rate, float cutoff_hz);

    void process(std::span<float> input);

  private:
    LowPass(LowPass const&) = delete;
    LowPass& operator=(LowPass const&) = delete;

  private:
    float m_a1, m_a2, m_b0, m_b1, m_b2;
    float m_x[2];
    float m_y[2];
};

}  // namespace pwv
