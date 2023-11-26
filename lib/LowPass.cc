#include "LowPass.h"

#include <cmath>
#include <vector>

namespace pwv {

LowPass::LowPass(double sampling_rate, double cutoff_hz) {
    reset(sampling_rate, cutoff_hz);
}

LowPass::~LowPass() {}

void LowPass::reset(double sampling_rate, double cutoff_hz) {
    // Stolen from here:
    // https://stackoverflow.com/a/20932062
    // TODO: higher order
    double const ff = cutoff_hz / sampling_rate;
    double const ita = 1.0 / std::tan(M_PI * ff);
    double const q = std::sqrt(2.0);
    double const b0 = 1.0 / (1.0 + q * ita + ita * ita);
    double const b1 = 2 * b0;
    double const b2 = b0;
    double const a1 = 2.0 * (ita * ita - 1.0) * b0;
    double const a2 = -(1.0 - q * ita + ita * ita) * b0;

    m_filter.reset(a1, a2, b0, b1, b2);
}

}  // namespace pwv
