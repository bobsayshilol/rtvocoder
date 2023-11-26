#pragma once

#include <span>

namespace pwv {

class SecondOrderFilter {
  public:
    using Coef = float;

  public:
    SecondOrderFilter();
    ~SecondOrderFilter();

    void reset(Coef a1, Coef a2, Coef b0, Coef b1, Coef b2);

    void process(std::span<float> input);

  private:
    SecondOrderFilter(SecondOrderFilter const&) = delete;
    SecondOrderFilter& operator=(SecondOrderFilter const&) = delete;

  private:
    Coef m_a1, m_a2, m_b0, m_b1, m_b2;
    float m_x[2];
    float m_y[2];
};

}  // namespace pwv
