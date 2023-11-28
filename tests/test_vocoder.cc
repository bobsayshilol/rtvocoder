#include "tests.h"

#include <Utils.h>
#include <Vocoder.h>
#include <cmath>
#include <vector>

MAKE_TEST(Vocoder_ctor) {
    pwv::Vocoder vocoder(20, 40, 44100);
    pwv::VocoderRT vocoder_rt(20, 40, 44100);
}

MAKE_TEST(Vocoder_process_chunk) {
    std::size_t const sampling_rate = 100;
    std::size_t const hz = 10;
    std::size_t const num_samples =
        pwv::VocoderRT::k_block_size * sampling_rate;

    // Generate some data.
    std::vector<float> input_signal(num_samples);
    pwv::add_sine(input_signal, sampling_rate, hz * 3.2, 0.5);
    std::vector<float> input_carrier(num_samples);
    pwv::add_sine(input_carrier, sampling_rate, hz * 2.7, 0.5);

    // Apply it in a single chunk.
    auto output_all = pwv::Vocoder(20, 40, sampling_rate)
                          .process(input_signal, input_carrier);
    std::vector<float> output_all_rt(num_samples);
    pwv::VocoderRT(20, 40, sampling_rate)
        .process(input_signal.data(), input_carrier.data(), num_samples,
                 output_all_rt.data());

    // Check that realtime matches bulk.
    CHECK_EQ(output_all.size(), num_samples);
    CHECK_EQ(output_all_rt.size(), num_samples);
    for (std::size_t i = 0; i < num_samples; i++) {
        APPROX_EQ(output_all[i], output_all_rt[i]);
    }

    // Apply it in chunks.
    std::size_t const chunk_size = pwv::VocoderRT::k_block_size * 2;
    static_assert(num_samples % chunk_size == 0);
    static_assert(chunk_size % pwv::VocoderRT::k_block_size == 0);
    pwv::VocoderRT vocoder_rt(20, 40, sampling_rate);
    std::vector<float> output_chunk_rt(num_samples);
    for (std::size_t chunk_start = 0; chunk_start < num_samples;
         chunk_start += chunk_size) {
        vocoder_rt.process(input_signal.data() + chunk_start,
                           input_carrier.data() + chunk_start, chunk_size,
                           output_chunk_rt.data() + chunk_start);
    }

    // Check that realtime matches.
    CHECK_EQ(output_chunk_rt.size(), num_samples);
    for (std::size_t i = 0; i < num_samples; i++) {
        APPROX_EQ(output_all[i], output_chunk_rt[i]);
    }
}
