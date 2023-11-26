#include "tests.h"

#include <BandPass.h>
#include <Utils.h>
#include <cmath>
#include <vector>

MAKE_TEST(BandPass_ctor) {
    for (int sampling_rate : {100, 1000, 16000, 48000}) {
        pwv::BandPass bandpass(sampling_rate, sampling_rate / 10,
                               pwv::BandPass::approximate_q(sampling_rate, 20));
    }
}

MAKE_TEST(BandPass_process) {
    std::size_t const sampling_rate = 100;
    std::size_t const pass_hz = 13;
    double const q = pwv::BandPass::approximate_q(sampling_rate, 40);

    auto test_hz = [=](float hz) {
        // Generate some data.
        // Bandpass takes a while to get going, so generate a lot.
        std::vector<float> samples(sampling_rate * 20);
        pwv::add_sine(samples, sampling_rate, hz, 1);

        // Grab the energy level before running the filter.
        const double before = pwv::average_energy(samples);

        // Run the filter on it.
        pwv::BandPass(sampling_rate, pass_hz, q).process(samples);

        // Return the ratio of the energy remaining.
        const double after = pwv::average_energy(samples);
        return after / before;
    };

    // Low and high frequencies shouldn't pass, only those near the pass band.
    for (std::size_t hz = 1; hz < pass_hz; hz++) {
        CHECK_LT(test_hz(hz), 0.1);
        CHECK_LT(test_hz(hz + 0.5), 0.1);
    }
    for (std::size_t hz = pass_hz; hz < pass_hz + 1; hz++) {
        CHECK_GT(test_hz(hz), 0.9);
    }
    for (std::size_t hz = pass_hz + 1; hz < 25; hz++) {
        CHECK_LT(test_hz(hz), 0.1);
        CHECK_LT(test_hz(hz + 0.5), 0.1);
    }
}

MAKE_TEST(BandPass_process_block) {
    std::size_t const sampling_rate = 100;
    std::size_t const pass_hz = 10;
    double const q = 0.9;

    // Generate some data.
    std::vector<float> samples_all(sampling_rate);
    pwv::add_sine(samples_all, sampling_rate, pass_hz, 0.1);
    auto samples_chunked = samples_all;

    // Apply it in a single block.
    pwv::BandPass(sampling_rate, pass_hz, q).process(samples_all);

    // Apply it in blocks.
    std::size_t const block_size = 10;
    static_assert(sampling_rate % block_size == 0);
    pwv::BandPass filter(sampling_rate, pass_hz, q);
    for (std::size_t block_start = 0; block_start < samples_chunked.size();
         block_start += block_size) {
        filter.process(
            std::span{samples_chunked}.subspan(block_start, block_size));
    }

    // Check that they match.
    for (std::size_t i = 0; i < samples_all.size(); i++) {
        CHECK_EQ(samples_all[i], samples_chunked[i]);
    }
}
