#include "Vocoder.h"

#include "BandPass.h"
#include "LowPass.h"

#include <cassert>
#include <cmath>

// Reference: see vocoder.ny in audacity

// TODO: refactor this to be realtime capable

namespace pwv {

namespace {

std::vector<float> bandpass(float sampling_rate, std::span<float const> input,
                            float hz, float q) {
    std::vector<float> result(input.begin(), input.end());
    BandPass(sampling_rate, hz, q).process(result);
    return result;
}

std::vector<float> lowpass(float sampling_rate, std::span<float const> input,
                           float cutoff) {
    std::vector<float> result(input.begin(), input.end());
    LowPass(sampling_rate, cutoff).process(result);
    return result;
}

void abs(std::span<float> input) {
    for (float& sample : input) {
        sample = std::abs(sample);
    }
}

void mul(std::span<float> a, std::span<float const> b) {
    assert(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); i++) {
        a[i] *= b[i];
    }
}

void add(std::span<float> a, std::span<float const> b) {
    assert(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); i++) {
        a[i] += b[i];
    }
}

}  // namespace

Vocoder::Vocoder(int distance, int num_bands, int sampling_rate)
    : m_distance(distance),
      m_num_bands(num_bands),
      m_sampling_rate(sampling_rate),
      m_octaves(std::log2(m_sampling_rate / 2.205f / 20)),
      m_interval(12 * m_octaves / m_num_bands) {}

Vocoder::~Vocoder() {}

std::vector<float> Vocoder::process(std::span<float const> input,
                                    std::span<float const> carrier) {
    assert(input.size() == carrier.size());
    std::vector<float> result(input.size());

    float const q = m_octaves / m_num_bands / std::sqrt(2);
    float band_hz = next_hz(20, m_interval / 2.f);
    for (int band = 0; band < m_num_bands; band++) {
        // Bandpass both
        auto input_filtered = bandpass(m_sampling_rate, input, band_hz, q);
        auto carrier_filtered = bandpass(m_sampling_rate, carrier, band_hz, q);

        // Calculate envelope
        abs(input_filtered);
        auto mod_envelope =
            lowpass(m_sampling_rate, input_filtered, band_hz / m_distance);

        // Combine
        mul(mod_envelope, carrier_filtered);
        add(result, bandpass(m_sampling_rate, mod_envelope, band_hz, q));

        // Next band
        band_hz = next_hz(band_hz, m_interval);
    }

    return result;
}

float Vocoder::next_hz(float hz, float interval) const {
    // TODO: optimize this
    float const k = std::log(2) / 12;
    float const c = std::log(440) - k * 69;
    auto hz_to_step = [&](float pitch) { return (std::log(pitch) - c) / k; };
    auto step_to_hz = [&](float step) { return std::exp(k * step + c); };

    float const prev_step = hz_to_step(hz);
    float const next_step = prev_step + interval;
    return step_to_hz(next_step);
}

}  // namespace pwv