#include <BandPass.h>
#include <LowPass.h>
#include <Vocoder.h>
#include <WAVFile.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace {

int run_vocoder(int argc, char **argv);
int run_lowpass(int argc, char **argv);
int run_noop(int argc, char **argv);
int run_benchmark(int argc, char **argv);

const struct {
    char const *name;
    char const *args;
    int (*run)(int argc, char **argv);
} g_modes[]{
    {"vocoder", "<distance> <bands> <signal> <modulator> <output>",
     run_vocoder},
    {"lowpass", "<cutoff> <input> <output>", run_lowpass},
    {"noop", "<input> <output>", run_noop},
    {"benchmark", "<input>", run_benchmark},
};

void usage(char const *name) {
    printf("Usage:\n");
    for (auto const &mode : g_modes) {
        printf("  %s %s %s\n", name, mode.name, mode.args);
    }
}

int run_vocoder(int argc, char **argv) {
    if (argc < 7) {
        printf("Not enough args to vocoder\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Read off args.
    double const distance = std::atof(argv[2]);
    int const num_bands = std::atoi(argv[3]);
    char const *const signal_path = argv[4];
    char const *const carrier_path = argv[5];
    char const *const output_path = argv[6];
    printf(
        "Running with num_bands=%i, signal_path=%s, carrier_path=%s, "
        "output_path=%s\n",
        num_bands, signal_path, carrier_path, output_path);

    // Read in the signals.
    auto signal = pwv::load_wav(signal_path);
    if (!signal) {
        printf("Failed to load wav: %s - %s\n", signal_path,
               signal.error().c_str());
        return EXIT_FAILURE;
    }
    auto carrier = pwv::load_wav(carrier_path);
    if (!carrier) {
        printf("Failed to load wav: %s - %s\n", carrier_path,
               carrier.error().c_str());
        return EXIT_FAILURE;
    }

    // Check that they're compatible.
    if (signal->sampling_rate != carrier->sampling_rate) {
        printf("Sampling rate mismatch\n");
        return EXIT_FAILURE;
    }

    // Run the filter on the data.
    std::size_t const num_samples =
        std::min(signal->samples.size(), carrier->samples.size());
    auto output = pwv::Vocoder(distance, num_bands, signal->sampling_rate)
                      .process(std::span{signal->samples.data(), num_samples},
                               std::span{carrier->samples.data(), num_samples});

    // Save it.
    pwv::WAVData wav_out;
    wav_out.sampling_rate = signal->sampling_rate;
    wav_out.samples = std::move(output);
    if (!pwv::save_wav(wav_out, output_path)) {
        printf("Failed to save wav: %s\n", output_path);
        return EXIT_FAILURE;
    }

    printf("Success!\n");
    return EXIT_SUCCESS;
}

int run_lowpass(int argc, char **argv) {
    if (argc < 5) {
        printf("Not enough args to lowpass\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Read off args.
    float const cutoff_hz = std::atof(argv[2]);
    char const *const input_path = argv[3];
    char const *const output_path = argv[4];
    printf("Running with cutoff_hz=%f, input_path=%s, output_path=%s\n",
           cutoff_hz, input_path, output_path);

    // Read in the data to process.
    auto input = pwv::load_wav(input_path);
    if (!input) {
        printf("Failed to load wav: %s - %s\n", input_path,
               input.error().c_str());
        return EXIT_FAILURE;
    }

    // Run the filter on the data.
    pwv::LowPass(input->sampling_rate, cutoff_hz).process(input->samples);

    // Save it.
    if (!pwv::save_wav(*input, output_path)) {
        printf("Failed to save wav: %s\n", output_path);
        return EXIT_FAILURE;
    }

    printf("Success!\n");
    return EXIT_SUCCESS;
}

int run_noop(int argc, char **argv) {
    if (argc < 4) {
        printf("Not enough args to noop\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Read off args.
    char const *const input_path = argv[2];
    char const *const output_path = argv[3];
    printf("Running with input_path=%s, output_path=%s\n", input_path,
           output_path);

    // Read in the data to process.
    auto input = pwv::load_wav(input_path);
    if (!input) {
        printf("Failed to load wav: %s - %s\n", input_path,
               input.error().c_str());
        return EXIT_FAILURE;
    }

    // Save it.
    if (!pwv::save_wav(*input, output_path)) {
        printf("Failed to save wav: %s\n", output_path);
        return EXIT_FAILURE;
    }

    printf("Success!\n");
    return EXIT_SUCCESS;
}

int run_benchmark(int argc, char **argv) {
    if (argc < 3) {
        printf("Not enough args to benchmark\n");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Read off args.
    char const *const input_path = argv[2];
    printf("Running with input_path=%s\n", input_path);

    // Read in the input.
    auto input = pwv::load_wav(input_path);
    if (!input) {
        printf("Failed to load wav: %s - %s\n", input_path,
               input.error().c_str());
        return EXIT_FAILURE;
    }

    // Quick timer class.
    struct Timer {
        using Clock = std::chrono::high_resolution_clock;
        Clock::time_point start;

        Timer() : start(Clock::now()) {}
        auto elapsed() const {
            return std::chrono::duration_cast<std::chrono::duration<double>>(
                Clock::now() - start);
        }
    };

    // Shrink the data so that it's a nice block size.
    std::size_t const chunk_size = pwv::VocoderRT::k_block_size * 2;
    std::size_t const num_samples =
        (input->samples.size() / chunk_size) * chunk_size;
    input->samples.resize(num_samples);

    // Helper to report a result.
    double const input_time =
        num_samples / static_cast<double>(input->sampling_rate);
    auto log_result = [&](char const *name, int opt, double elapsed) {
        printf("%s (%i):\t%fs in %fs (%fx)\n", name, opt, input_time, elapsed,
               input_time / elapsed);
    };

    // Lowpass.
    for (int cutoff_hz : {200, 1000, 2000}) {
        pwv::LowPass filter(input->sampling_rate, cutoff_hz);
        auto signal_copy = input->samples;
        {
            Timer timer;
            filter.process(signal_copy);
            log_result("Lowpass bulk", cutoff_hz, timer.elapsed().count());
        }
        {
            Timer timer;
            for (std::size_t chunk_start = 0; chunk_start < num_samples;
                 chunk_start += chunk_size) {
                filter.process(
                    std::span{signal_copy}.subspan(chunk_start, chunk_size));
            }
            log_result("Lowpass rt", cutoff_hz, timer.elapsed().count());
        }
    }

    // Bandpass.
    for (int num_bands : {10, 40, 80}) {
        auto q = pwv::BandPass::approximate_q(input->sampling_rate, num_bands);
        pwv::BandPass filter(input->sampling_rate, 600, q);
        auto signal_copy = input->samples;
        {
            Timer timer;
            filter.process(signal_copy);
            log_result("BandPass bulk", num_bands, timer.elapsed().count());
        }
        {
            Timer timer;
            for (std::size_t chunk_start = 0; chunk_start < num_samples;
                 chunk_start += chunk_size) {
                filter.process(
                    std::span{signal_copy}.subspan(chunk_start, chunk_size));
            }
            log_result("BandPass rt", num_bands, timer.elapsed().count());
        }
    }

    // Vocoder.
    for (int num_bands : {10, 40, 80}) {
        {
            pwv::Vocoder filter(20, num_bands, input->sampling_rate);
            auto signal_copy = input->samples;
            Timer timer;
            filter.process(signal_copy, signal_copy);
            log_result("Vocoder bulk", num_bands, timer.elapsed().count());
        }
        {
            pwv::VocoderRT filter(20, num_bands, input->sampling_rate);
            auto const &signal_copy = input->samples;
            std::vector<float> output(signal_copy.size());
            Timer timer;
            for (std::size_t chunk_start = 0; chunk_start < num_samples;
                 chunk_start += chunk_size) {
                filter.process(signal_copy.data(), signal_copy.data(),
                               chunk_size, output.data());
            }
            log_result("Vocoder rt", num_bands, timer.elapsed().count());
        }
    }

    printf("Success!\n");
    return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Find the mode.
    std::string_view const opt = argv[1];
    for (auto const &mode : g_modes) {
        if (opt == mode.name) {
            return mode.run(argc, argv);
        }
    }

    printf("Unknown option: %s\n", argv[1]);
    return EXIT_FAILURE;
}
