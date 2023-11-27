#include "Vocoder.h"

#include "BandPass.h"
#include "LowPass.h"

#include <algorithm>
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

double next_hz(double hz, double interval) {
    // TODO: optimize this
    double const k = std::log(2) / 12;
    double const c = std::log(440) - k * 69;
    auto hz_to_step = [&](double pitch) { return (std::log(pitch) - c) / k; };
    auto step_to_hz = [&](double step) { return std::exp(k * step + c); };

    double const prev_step = hz_to_step(hz);
    double const next_step = prev_step + interval;
    return step_to_hz(next_step);
}

}  // namespace

Vocoder::Vocoder(double distance, int num_bands, double sampling_rate)
    : m_distance(distance),
      m_num_bands(num_bands),
      m_sampling_rate(sampling_rate),
      m_q(BandPass::approximate_q(m_sampling_rate, num_bands)) {}

Vocoder::~Vocoder() {}

std::vector<float> Vocoder::process(std::span<float const> signal,
                                    std::span<float const> carrier) {
    assert(signal.size() == carrier.size());
    std::vector<float> result(signal.size());

    double const interval = 12 * std::sqrt(2) / m_q;
    double band_hz = next_hz(20, interval / 2.0);
    for (int band = 0; band < m_num_bands; band++) {
        // Bandpass both
        auto signal_filtered = bandpass(m_sampling_rate, signal, band_hz, m_q);
        auto carrier_filtered =
            bandpass(m_sampling_rate, carrier, band_hz, m_q);

        // Calculate envelope
        abs(signal_filtered);
        auto mod_envelope =
            lowpass(m_sampling_rate, signal_filtered, band_hz / m_distance);

        // Combine
        mul(mod_envelope, carrier_filtered);
        add(result, bandpass(m_sampling_rate, mod_envelope, band_hz, m_q));

        // Next band
        band_hz = next_hz(band_hz, interval);
    }

    return result;
}

VocoderRT::VocoderRT(double distance, int num_bands, double sampling_rate) {
    // Build the filters.
    double const q = BandPass::approximate_q(sampling_rate, num_bands);
    double const interval = 12 * std::sqrt(2) / q;
    double band_hz = next_hz(20, interval / 2.0);
    for (int band = 0; band < num_bands; band++) {
        // TODO: deduplicate the bandpass filters.
        m_bandpass.emplace_back(sampling_rate, band_hz, q);
        m_bandpass.emplace_back(sampling_rate, band_hz, q);
        m_bandpass.emplace_back(sampling_rate, band_hz, q);
        m_lowpass.emplace_back(sampling_rate, band_hz / distance);

        // Next band
        band_hz = next_hz(band_hz, interval);
    }
}

VocoderRT::~VocoderRT() {}

void VocoderRT::process(float const* signal, float const* carrier,
                        std::size_t count, float* output) {
    assert((count % k_block_size) == 0);
    for (std::size_t i = 0; i < count; i += k_block_size) {
        process_block(signal + i, carrier + i, output + i);
    }
}

void VocoderRT::process_block(float const* signal, float const* carrier,
                              float* output) {
    using Block = std::array<float, k_block_size>;
    auto load_block = [](float const* ptr) {
        Block block;
        std::copy_n(ptr, k_block_size, block.data());
        return block;
    };

    // Setup the blocks.
    Block const signal_input = load_block(signal);
    Block const carrier_input = load_block(carrier);

    // Run the filters on it.
    Block result_block{};
    std::size_t const num_bands = m_lowpass.size();
    for (std::size_t band = 0; band < num_bands; band++) {
        // Grab the filters for this band.
        BandPass& bandpass0 = m_bandpass[3 * band + 0];
        BandPass& bandpass1 = m_bandpass[3 * band + 1];
        BandPass& bandpass2 = m_bandpass[3 * band + 2];
        LowPass& lowpass = m_lowpass[band];

        // Initialise this block.
        Block signal_block = signal_input;
        Block carrier_block = carrier_input;

        // Bandpass both inputs.
        bandpass0.process(signal_block);
        bandpass1.process(carrier_block);

        // Calculate envelope.
        abs(signal_block);
        lowpass.process(signal_block);

        // Combine.
        mul(signal_block, carrier_block);
        bandpass2.process(signal_block);
        add(result_block, signal_block);
    }

    // Store it back.
    std::copy_n(result_block.data(), k_block_size, output);
}

}  // namespace pwv
