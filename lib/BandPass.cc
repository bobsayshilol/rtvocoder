#include "BandPass.h"

namespace pwv {

BandPass::BandPass(double sampling_rate, double hz, double q) {
    reset(sampling_rate, hz, q);
}

BandPass::~BandPass() {}

void BandPass::reset(double sampling_rate, double hz, double q) {
    // TODO
    (void)sampling_rate;
    (void)hz;
    (void)q;
}

void BandPass::process(std::span<float> input) {
    // TODO
    (void)input;
}

}  // namespace pwv
