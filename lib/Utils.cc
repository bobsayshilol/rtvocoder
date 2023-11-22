#include "Utils.h"

#include <cmath>
#include <numeric>

namespace pwv {

double average_energy(std::span<float const> data) {
    std::size_t const length = data.size();
    return std::transform_reduce(
        data.begin(), data.end(), 0.0, std::plus{},
        [length](double sample) { return sample * sample / length; });
}

void add_sine(std::span<float> samples, float sampling_rate, float hz,
              float scale) {
    for (std::size_t t = 0; t < samples.size(); t++) {
        samples[t] +=
            std::sin(2 * static_cast<float>(M_PI) * t * hz / sampling_rate) *
            scale;
    }
}

}  // namespace pwv
