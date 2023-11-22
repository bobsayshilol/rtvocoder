#include "LowPass.h"

#include <cmath>
#include <vector>

namespace pwv {

LowPass::LowPass(float sampling_rate, float cutoff_hz) {
    reset(sampling_rate, cutoff_hz);
}

void LowPass::reset(float sampling_rate, float cutoff_hz) {
    // Stolen from here:
    // https://stackoverflow.com/a/20932062
    // TODO: higher order
    double const ff = cutoff_hz / sampling_rate;
    double const ita = 1.0 / std::tan(M_PI * ff);
    double const q = std::sqrt(2.0);
    m_b0 = 1.0 / (1.0 + q * ita + ita * ita);
    m_b1 = 2 * m_b0;
    m_b2 = m_b0;
    m_a1 = 2.0 * (ita * ita - 1.0) * m_b0;
    m_a2 = -(1.0 - q * ita + ita * ita) * m_b0;

    // Clear prior state.
    std::fill(std::begin(m_x), std::end(m_x), 0);
    std::fill(std::begin(m_y), std::end(m_y), 0);
}

LowPass::~LowPass() {}

void LowPass::process(std::span<float> input) {
    std::vector<float> output(input.size());

    // |-3|-2|-1| 0| 1| 2|

    // Apply the filter.
    // TODO: optimize
    for (std::size_t i = 0; i < input.size(); i++) {
        float sample = 0;
        sample += m_b0 * input[i];
        sample += m_b1 * (i < 1 ? m_x[i + 1] : input[i - 1]);
        sample += m_b2 * (i < 2 ? m_x[i + 0] : input[i - 2]);
        sample += m_a1 * (i < 1 ? m_y[i + 1] : output[i - 1]);
        sample += m_a2 * (i < 2 ? m_y[i + 0] : output[i - 2]);
        output[i] = sample;
    }

    // Save state that can be referenced on next loop.
    m_x[1] = input[input.size() - 1];
    m_x[0] = input[input.size() - 2];
    m_y[1] = output[output.size() - 1];
    m_y[0] = output[output.size() - 2];

    // TODO: don't use a temporary buffer, cache it, or return it
    std::copy(output.begin(), output.end(), input.begin());
}

}  // namespace pwv
