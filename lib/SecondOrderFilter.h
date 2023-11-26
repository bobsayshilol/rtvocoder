#pragma once

#include <span>

namespace pwv {

class SecondOrderFilter {
  public:
    SecondOrderFilter();
    ~SecondOrderFilter();

    void reset(float a1, float a2, float b0, float b1, float b2);

    void process(std::span<float> input);

  private:
    SecondOrderFilter(SecondOrderFilter const&) = delete;
    SecondOrderFilter& operator=(SecondOrderFilter const&) = delete;

  private:
    float m_a1, m_a2, m_b0, m_b1, m_b2;
    float m_x[2];
    float m_y[2];
};

}  // namespace pwv
