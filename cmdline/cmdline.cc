#include <LowPass.h>
#include <Vocoder.h>
#include <WAVFile.h>
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace {

int run_vocoder(int argc, char **argv);
int run_lowpass(int argc, char **argv);
int run_noop(int argc, char **argv);

const struct {
    char const *name;
    char const *args;
    int (*run)(int argc, char **argv);
} g_modes[]{
    {"vocoder", "<distance> <bands> <signal> <modulator> <output>",
     run_vocoder},
    {"lowpass", "<cutoff> <input> <output>", run_lowpass},
    {"noop", "<input> <output>", run_noop},
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
    char const *const modulator_path = argv[5];
    char const *const output_path = argv[6];
    printf(
        "Running with num_bands=%i, signal_path=%s, input_path=%s, "
        "output_path=%s\n",
        num_bands, signal_path, modulator_path, output_path);

    // Read in the signals.
    auto signal = pwv::load_wav(signal_path);
    if (!signal) {
        printf("Failed to load wav: %s - %s\n", signal_path,
               signal.error().c_str());
        return EXIT_FAILURE;
    }
    auto modulator = pwv::load_wav(modulator_path);
    if (!modulator) {
        printf("Failed to load wav: %s - %s\n", modulator_path,
               modulator.error().c_str());
        return EXIT_FAILURE;
    }

    // Check that they're compatible.
    if (signal->sampling_rate != modulator->sampling_rate) {
        printf("Sampling rate mismatch\n");
        return EXIT_FAILURE;
    }

    // Run the filter on the data.
    std::size_t const num_samples =
        std::min(signal->samples.size(), modulator->samples.size());
    auto output =
        pwv::Vocoder(distance, num_bands, signal->sampling_rate)
            .process(std::span{signal->samples.data(), num_samples},
                     std::span{modulator->samples.data(), num_samples});

    // Save it.
    pwv::WAVData wav_out;
    wav_out.sampling_rate = signal->sampling_rate;
    wav_out.samples = std::move(output);
    if (!pwv::save_wav(wav_out, output_path)) {
        printf("Failed to save wav: %s\n", output_path);
        return EXIT_FAILURE;
    }

    printf("Success!\n");
    return EXIT_FAILURE;
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
