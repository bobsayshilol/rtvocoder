#pragma once

#include <span>
#include <vector>

namespace pwv {

class Vocoder {
  public:
    Vocoder(int distance, int num_bands, int sampling_rate);
    ~Vocoder();

    std::vector<float> process(std::span<float const> input,
                               std::span<float const> modulator);

  private:
    Vocoder(Vocoder const&) = delete;
    Vocoder& operator=(Vocoder const&) = delete;

  private:
    float next_hz(float hz, float interval) const;

  private:
    int const m_distance;
    int const m_num_bands;
    int const m_sampling_rate;
    float const m_octaves;
    float const m_interval;
};

}  // namespace pwv
