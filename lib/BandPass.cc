#include "BandPass.h"

namespace pwv {

BandPass::BandPass(float sampling_rate, float hz, float q) {
    reset(sampling_rate, hz, q);
}

BandPass::~BandPass() {}

void BandPass::reset(float sampling_rate, float hz, float q) {
    // TODO
}

void BandPass::process(std::span<float> input) {
    // TODO
}

}  // namespace pwv
