#include "Vocoder.h"

#include "BandPass.h"
#include "LowPass.h"

#include <cassert>
#include <cmath>

// Reference: see vocoder.ny in audacity

// TODO: refactor this to be realtime capable

namespace pwv {

namespace {

std::vector<float> bandpass(double sampling_rate, std::span<float const> input,
                            double hz, double q) {
    std::vector<float> result(input.begin(), input.end());
    BandPass(sampling_rate, hz, q).process(result);
    return result;
}

std::vector<float> lowpass(double sampling_rate, std::span<float const> input,
                           double cutoff) {
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

Vocoder::Vocoder(double distance, int num_bands, double sampling_rate)
    : m_distance(distance),
      m_num_bands(num_bands),
      m_sampling_rate(sampling_rate),
      m_q(BandPass::approximate_q(m_sampling_rate, num_bands)) {}

Vocoder::~Vocoder() {}

std::vector<float> Vocoder::process(std::span<float const> input,
                                    std::span<float const> carrier) {
    assert(input.size() == carrier.size());
    std::vector<float> result(input.size());

    double const interval = 12 * std::sqrt(2) / m_q;
    double band_hz = next_hz(20, interval / 2.0);
    for (int band = 0; band < m_num_bands; band++) {
        // Bandpass both
        auto input_filtered = bandpass(m_sampling_rate, input, band_hz, m_q);
        auto carrier_filtered =
            bandpass(m_sampling_rate, carrier, band_hz, m_q);

        // Calculate envelope
        abs(input_filtered);
        auto mod_envelope =
            lowpass(m_sampling_rate, input_filtered, band_hz / m_distance);

        // Combine
        mul(mod_envelope, carrier_filtered);
        add(result, bandpass(m_sampling_rate, mod_envelope, band_hz, m_q));

        // Next band
        band_hz = next_hz(band_hz, interval);
    }

    return result;
}

double Vocoder::next_hz(double hz, double interval) const {
    // TODO: optimize this
    double const k = std::log(2) / 12;
    double const c = std::log(440) - k * 69;
    auto hz_to_step = [&](double pitch) { return (std::log(pitch) - c) / k; };
    auto step_to_hz = [&](double step) { return std::exp(k * step + c); };

    double const prev_step = hz_to_step(hz);
    double const next_step = prev_step + interval;
    return step_to_hz(next_step);
}

}  // namespace pwv
