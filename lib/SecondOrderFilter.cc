#include "SecondOrderFilter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace pwv {

SecondOrderFilter::SecondOrderFilter() { reset(0, 0, 0, 0, 0); }

SecondOrderFilter::~SecondOrderFilter() {}

void SecondOrderFilter::reset(Coef a1, Coef a2, Coef b0, Coef b1, Coef b2) {
    m_a1 = a1;
    m_a2 = a2;
    m_b0 = b0;
    m_b1 = b1;
    m_b2 = b2;

    // Clear prior state.
    std::fill(std::begin(m_x), std::end(m_x), 0);
    std::fill(std::begin(m_y), std::end(m_y), 0);
}

void SecondOrderFilter::process(std::span<float> input) {
    using Accumulator = Coef;
    std::vector<float> output(input.size());

    // Apply the filter.
    // TODO: optimize
    for (std::size_t i = 0; i < input.size(); i++) {
        Accumulator sample = 0;
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

void SecondOrderFilter::process_block(std::span<float> data) {
    assert(data.size() == k_block_size);
    using Accumulator = Coef;

    // Store the last 2 values + space for the current block.
    std::array<float, k_block_size + 2> output{};
    output[0] = m_y[0];
    output[1] = m_y[1];
    std::array<float, k_block_size + 2> input{};
    input[0] = m_x[0];
    input[1] = m_x[1];
    std::copy_n(data.data(), k_block_size, input.data() + 2);

    // Apply the filter.
    for (std::size_t i = 0; i < k_block_size; i++) {
        Accumulator sample = 0;
        sample += m_b0 * input[i + 2];
        sample += m_b1 * input[i + 1];
        sample += m_b2 * input[i + 0];
        sample += m_a1 * output[i + 1];
        sample += m_a2 * output[i + 0];
        output[i + 2] = sample;
    }

    // Save state that can be referenced on next loop.
    m_x[0] = input[k_block_size + 0];
    m_x[1] = input[k_block_size + 1];
    m_y[0] = output[k_block_size + 0];
    m_y[1] = output[k_block_size + 1];

    // Copy the result back.
    std::copy_n(output.data() + 2, k_block_size, data.data());
}

}  // namespace pwv
