#include "Processor.h"

// TODO: this is basically a copy of the DSP filter plugin example
// TODO: still need to work out how to automate the graph changes
/* SPDX-FileCopyrightText: Copyright C 2019 Wim Taymans */
/* SPDX-License-Identifier: MIT */

#include <Vocoder.h>
#include <WAVFile.h>
#include <cstdio>
#include <memory>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/latency-utils.h>
#include <utility>
#include <vector>

namespace pipewire {

namespace {

#if 0
pw_stream_flags operator|(pw_stream_flags lhs, pw_stream_flags rhs) {
    using U = std::underlying_type_t<pw_stream_flags>;
    return static_cast<pw_stream_flags>(static_cast<U>(lhs) |
                                        static_cast<U>(rhs));
}

// TODO: write tests for these
void deinterleave(float const *input, std::size_t num_channels,
                  std::size_t num_frames, float *output) {
    // abcabc -> aabbcc
    for (std::size_t channel = 0; channel < num_channels; channel++) {
        for (std::size_t frame = 0; frame < num_frames; frame++) {
            output[channel * num_frames + frame] =
                input[frame * num_channels + channel];
        }
    }
}

void interleave(float const *input, std::size_t num_channels,
                std::size_t num_frames, float *output) {
    // aabbcc -> abcabc
    for (std::size_t frame = 0; frame < num_frames; frame++) {
        for (std::size_t channel = 0; channel < num_channels; channel++) {
            output[frame * num_channels + channel] =
                input[channel * num_frames + frame];
        }
    }
}
#endif

struct UserData;
struct Port {
    UserData *data;
};
struct UserData {
    pw_main_loop *loop;
    pw_filter *filter;
    spa_audio_info format;

    Port *in_port;
    Port *out_port;

    std::vector<std::unique_ptr<pwv::VocoderRT>> filters;
    // std::vector<float> interleave_buffer;

    std::vector<float> carrier_wave;
    std::size_t offset = 0;
};

static void on_process(void *userdata, struct spa_io_position *position) {
    UserData *data = static_cast<UserData *>(userdata);

    uint32_t const num_frames = position->clock.duration;

    float *input = static_cast<float *>(
        pw_filter_get_dsp_buffer(data->in_port, num_frames));
    float *output = static_cast<float *>(
        pw_filter_get_dsp_buffer(data->out_port, num_frames));

    if (input == nullptr || output == nullptr) {
        return;
    }

    size_t const num_channels = 1;

    if (num_channels != data->filters.size()) {
        printf("Bad number of channels\n");
        return;
    }
    if (num_frames % pwv::VocoderRT::k_block_size) {
        printf("Can't handle chunk size\n");
        return;
    }

    // Data is interleaved.
    // data->interleave_buffer.resize(num_channels * num_frames);
    // deinterleave(input, num_channels, num_frames,
    //             data->interleave_buffer.data());

    // Handle eof.
    if (data->offset + num_frames > data->carrier_wave.size()) {
        data->offset = 0;
    }
    float const *const carrier = data->carrier_wave.data() + data->offset;

    // Update carrier wave offset
    data->offset += num_frames;

    // Apply the filter to each channel
    // float *const signal = data->interleave_buffer.data();
    for (std::size_t channel = 0; channel < num_channels; channel++) {
        float const *const channel_signal = input + channel * num_frames;
        float *const channel_output = output + channel * num_frames;
        data->filters[channel]->process(channel_signal, carrier, num_frames,
                                        channel_output);
    }

    // interleave(data->interleave_buffer.data(), num_channels, num_frames,
    //            output);
}

const struct pw_filter_events filter_events = {
    .version = PW_VERSION_FILTER_EVENTS,
    .destroy = nullptr,
    .state_changed = nullptr,
    .io_changed = nullptr,
    .param_changed = nullptr,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = on_process,
    .drained = nullptr,
    .command = nullptr,
};

static void do_quit(void *userdata, int /*signal_number*/) {
    auto *data = static_cast<UserData *>(userdata);
    pw_main_loop_quit(data->loop);
}

}  // namespace

void run_processor(pw_main_loop *loop) {
    UserData data = {};
    data.loop = loop;

    // Load the carrier wave.
    auto carrier = pwv::load_wav("data/input_hbfs_mono.wav");
    if (!carrier) {
        printf("Failed to load carrier: %s\n", carrier.error().c_str());
        return;
    }
    // TODO: resample it
    data.carrier_wave = std::move(carrier->samples);
    data.offset = 0;

    // TODO: how do we get the hz of the thing(s) we're plugging into?
    data.filters.resize(1);
    for (auto &filter : data.filters) {
        filter = std::make_unique<pwv::VocoderRT>(20, 60, 44100);
    }

    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit,
                       &data);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit,
                       &data);

    data.filter = pw_filter_new_simple(
        pw_main_loop_get_loop(data.loop), "vocoder-filter",
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY,
                          "Filter", PW_KEY_MEDIA_ROLE, "DSP", NULL),
        &filter_events, &data);

    /* make an audio DSP input port */
    data.in_port = static_cast<Port *>(pw_filter_add_port(
        data.filter, PW_DIRECTION_INPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        sizeof(Port),
        pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                          PW_KEY_PORT_NAME, "input", NULL),
        NULL, 0));

    /* make an audio DSP output port */
    data.out_port = static_cast<Port *>(pw_filter_add_port(
        data.filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        sizeof(Port),
        pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit float mono audio",
                          PW_KEY_PORT_NAME, "output", NULL),
        NULL, 0));

    uint8_t buffer[1024];
    spa_pod_builder builder = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    auto opts = SPA_PROCESS_LATENCY_INFO_INIT(.ns = 10 * SPA_NSEC_PER_MSEC);
    spa_pod const *params =
        spa_process_latency_build(&builder, SPA_PARAM_ProcessLatency, &opts);

    if (pw_filter_connect(data.filter, PW_FILTER_FLAG_RT_PROCESS, &params, 1) <
        0) {
        printf("Can't connect: %i\n", errno);
        return;
    }

    printf("Running\n");
    pw_main_loop_run(data.loop);

    pw_filter_destroy(data.filter);
}

}  // namespace pipewire
