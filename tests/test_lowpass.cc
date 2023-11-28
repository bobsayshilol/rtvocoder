#include "tests.h"

#include <LowPass.h>
#include <Utils.h>
#include <cmath>
#include <vector>

MAKE_TEST(LowPass_ctor) { pwv::LowPass lowpass(44100, 1000); }

MAKE_TEST(LowPass_process) {
    std::size_t const sampling_rate = 100;
    std::size_t const cutoff_hz = 12;

    auto test_hz = [=](float hz) {
        // Generate some data.
        std::vector<float> samples(sampling_rate);
        pwv::add_sine(samples, sampling_rate, hz, 0.1);

        // Grab the energy level before running the filter.
        const double before = pwv::average_energy(samples);

        // Run the filter on it.
        pwv::LowPass lowpass(sampling_rate, cutoff_hz);
        lowpass.process(samples);

        // Return the ratio of the energy remaining.
        const double after = pwv::average_energy(samples);
        return after / before;
    };

    // Low frequencies should pass, high shouldn't.
    for (int hz = 1; hz < 5; hz++) {
        CHECK_GT(test_hz(hz), 0.95);
        CHECK_GT(test_hz(hz + 0.5), 0.95);
    }
    for (int hz = 20; hz <= 25; hz++) {
        CHECK_LT(test_hz(hz), 0.1);
        CHECK_LT(test_hz(hz + 0.5), 0.1);
    }
}

MAKE_TEST(LowPass_process_chunk) {
    std::size_t const sampling_rate = 100;
    std::size_t const cutoff_hz = 10;
    std::size_t const num_samples = pwv::LowPass::k_block_size * sampling_rate;

    // Generate some data.
    std::vector<float> samples_all(num_samples);
    pwv::add_sine(samples_all, sampling_rate, cutoff_hz, 0.1);
    auto samples_chunked = samples_all;
    auto samples_blocked = samples_all;

    // Apply it in a single chunk.
    pwv::LowPass(sampling_rate, cutoff_hz).process(samples_all);

    // Apply it in chunks.
    std::size_t const chunk_size = pwv::LowPass::k_block_size * 2;
    static_assert(num_samples % chunk_size == 0);
    pwv::LowPass filter_chunk(sampling_rate, cutoff_hz);
    pwv::LowPass filter_block(sampling_rate, cutoff_hz);
    for (std::size_t chunk_start = 0; chunk_start < samples_chunked.size();
         chunk_start += chunk_size) {
        // Single chunk.
        filter_chunk.process(
            std::span{samples_chunked}.subspan(chunk_start, chunk_size));

        // Single block.
        static_assert(chunk_size % pwv::LowPass::k_block_size == 0);
        for (std::size_t block_offset = 0; block_offset < chunk_size;
             block_offset += pwv::LowPass::k_block_size) {
            filter_block.process_block(std::span{samples_blocked}.subspan(
                chunk_start + block_offset, pwv::LowPass::k_block_size));
        }
    }

    // Check that they match.
    for (std::size_t i = 0; i < samples_all.size(); i++) {
        APPROX_EQ(samples_all[i], samples_chunked[i]);
        APPROX_EQ(samples_all[i], samples_blocked[i]);
    }
}
