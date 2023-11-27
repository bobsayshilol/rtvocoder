#pragma once

#include <span>
#include <vector>

namespace pwv {

class Vocoder {
  public:
    Vocoder(double distance, int num_bands, double sampling_rate);
    ~Vocoder();

    std::vector<float> process(std::span<float const> signal,
                               std::span<float const> carrier);

  private:
    Vocoder(Vocoder const&) = delete;
    Vocoder& operator=(Vocoder const&) = delete;

  private:
    double const m_distance;
    int const m_num_bands;
    double const m_sampling_rate;
    double const m_q;
};

}  // namespace pwv
