#include "BandPass.h"

#include <cmath>

namespace pwv {

BandPass::BandPass(double sampling_rate, double hz, double q) {
    reset(sampling_rate, hz, q);
}

BandPass::~BandPass() {}

void BandPass::reset(double sampling_rate, double hz, double q) {
    // BPF from https://www.w3.org/TR/audio-eq-cookbook
    double const omega = 2 * M_PI * hz / sampling_rate;
    // alpha seems incorrect as just /2Q from the cookbook.
    // double const alpha_q = std::sin(omega) / 2;
    double const alpha = std::sin(omega) * std::sinh(1 / (2 * q));
    double const inv_a0 = 1 / (1 + alpha);
    double const b0 = alpha * inv_a0;
    double const b1 = 0;
    double const b2 = -b0;
    double const a1 = 2 * std::cos(omega) * inv_a0;
    double const a2 = -(1 - alpha) * inv_a0;

    m_filter.reset(a1, a2, b0, b1, b2);
}

double BandPass::approximate_q(double sampling_rate, int num_bands) {
    double const octaves = std::log2(sampling_rate / (2.205 * 20));
    return std::sqrt(2) * num_bands / octaves;
}

}  // namespace pwv
