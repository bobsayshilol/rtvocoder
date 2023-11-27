#pragma once

#include <span>
#include <vector>

namespace pwv {

class BandPass;
class LowPass;

// Reference implementation.
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

// Realtime version.
class VocoderRT {
  public:
    static constexpr std::size_t k_block_size = 16;

  public:
    VocoderRT(double distance, int num_bands, double sampling_rate);
    ~VocoderRT();

    void process(float const* signal, float const* carrier, std::size_t count,
                 float* output);

  private:
    VocoderRT(VocoderRT const&) = delete;
    VocoderRT& operator=(VocoderRT const&) = delete;

  private:
    void process_block(float const* signal, float const* carrier,
                       float* output);

  private:
    std::vector<BandPass> m_bandpass;
    std::vector<LowPass> m_lowpass;
};

}  // namespace pwv
