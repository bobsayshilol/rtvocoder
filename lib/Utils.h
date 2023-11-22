#pragma once

#include <span>

namespace pwv {

double average_energy(std::span<float const> data);

void add_sine(std::span<float> samples, float sampling_rate, float hz,
              float scale);

}  // namespace pwv
