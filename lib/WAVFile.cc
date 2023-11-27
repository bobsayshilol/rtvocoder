#include "WAVFile.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>

namespace pwv {

namespace {

struct FileCloser {
    void operator()(FILE *f) const {
        if (f != nullptr) {
            fclose(f);
        }
    }
};
using File = std::unique_ptr<FILE, FileCloser>;

struct ChunkHeader {
    char chunk_id[4];
    uint32_t chunk_size;
};
static_assert(sizeof(ChunkHeader) == 8);

struct FmtChunk {
    uint16_t format;
    uint16_t channels;
    uint32_t sampling_rate;
    uint32_t byte_rate;    // sampling_rate * channels * bits_per_sample / 8
    uint16_t block_align;  // channels * bits_per_sample / 8
    uint16_t bits_per_sample;
};
static_assert(sizeof(FmtChunk) == 16);

template <typename T>
bool read(File &file, T *data, std::size_t bytes = sizeof(T)) {
    return fread(data, 1, bytes, file.get()) == bytes;
}

template <typename T>
void write(File &file, T *data, std::size_t bytes = sizeof(T)) {
    fwrite(data, 1, bytes, file.get());
}

using SampleType = int16_t;

}  // namespace

std::expected<WAVData, std::string> load_wav(std::filesystem::path path) {
    // ifstream would be useful, if you could tell how much you've read...
    File input(fopen(path.string().c_str(), "rb"));
    if (!input) {
        return std::unexpected("Failed to open file");
    }

    // Read the RIFF header.
    ChunkHeader header{};
    if (!read(input, &header) || std::memcmp(header.chunk_id, "RIFF", 4)) {
        return std::unexpected("Bad RIFF header");
    }
    if (!read(input, &header, 4) || std::memcmp(header.chunk_id, "WAVE", 4)) {
        return std::unexpected("Bad WAVE header");
    }

    // Read the FMT header.
    if (!read(input, &header) || std::memcmp(header.chunk_id, "fmt ", 4) ||
        header.chunk_size != 16) {
        return std::unexpected("Bad FMT header");
    }

    // Read the FMT chunk.
    FmtChunk fmt_chunk{};
    if (!read(input, &fmt_chunk)) {
        return std::unexpected("Bad FMT chunk");
    }

    // Pull out info about the file.
    WAVData wav_file;
    wav_file.sampling_rate = fmt_chunk.sampling_rate;
    if (fmt_chunk.format != 1 || fmt_chunk.channels != 1 ||
        fmt_chunk.bits_per_sample != 16) {
        return std::unexpected("Unhandled audio format");
    }

    // Read the DATA header.
    if (!read(input, &header) || std::memcmp(header.chunk_id, "data", 4) ||
        header.chunk_size % sizeof(SampleType)) {
        return std::unexpected("Bad DATA header");
    }

    // Read the data in.
    std::size_t const num_samples = header.chunk_size / sizeof(SampleType);
    std::vector<SampleType> raw_data(num_samples);
    if (!read(input, raw_data.data(), raw_data.size() * sizeof(SampleType))) {
        return std::unexpected("End of file");
    }

    // Convert to float.
    wav_file.samples.resize(num_samples);
    std::transform(raw_data.begin(), raw_data.end(), wav_file.samples.begin(),
                   [](SampleType sample) -> float {
                       return -static_cast<float>(sample) /
                              std::numeric_limits<SampleType>::min();
                   });

    return wav_file;
}

bool save_wav(WAVData const &data, std::filesystem::path path) {
    // Convert to shorts.
    std::vector<SampleType> raw_data(data.samples.size());
    std::transform(
        data.samples.begin(), data.samples.end(), raw_data.begin(),
        [](float sample) -> SampleType {
            return std::clamp(
                -sample * std::numeric_limits<SampleType>::min(),
                static_cast<float>(std::numeric_limits<SampleType>::min()),
                static_cast<float>(std::numeric_limits<SampleType>::max()));
        });

    // Build the FMT chunk.
    FmtChunk fmt_chunk;
    fmt_chunk.format = 1;
    fmt_chunk.channels = 1;
    fmt_chunk.sampling_rate = data.sampling_rate;
    fmt_chunk.bits_per_sample = sizeof(SampleType) * 8;
    fmt_chunk.block_align = fmt_chunk.channels * fmt_chunk.bits_per_sample / 8;
    fmt_chunk.byte_rate = fmt_chunk.sampling_rate * fmt_chunk.block_align;

    // Build the headers.
    ChunkHeader fmt_header;
    std::memcpy(fmt_header.chunk_id, "fmt ", 4);
    fmt_header.chunk_size = sizeof(fmt_chunk);
    ChunkHeader data_header;
    std::memcpy(data_header.chunk_id, "data", 4);
    data_header.chunk_size = raw_data.size() * sizeof(SampleType);
    ChunkHeader riff_header;
    std::memcpy(riff_header.chunk_id, "RIFF", 4);
    riff_header.chunk_size = 4 +                          // WAVE
                             8 + fmt_header.chunk_size +  // FMT chunk
                             8 + data_header.chunk_size;  // DATA chunk

    // Open the output file.
    File output(fopen(path.string().c_str(), "wb"));
    if (!output) {
        return false;
    }

    // Dump it all out.
    write(output, &riff_header);
    write(output, "WAVE", 4);
    write(output, &fmt_header);
    write(output, &fmt_chunk);
    write(output, &data_header);
    write(output, raw_data.data(), raw_data.size() * sizeof(SampleType));

    return true;
}

}  // namespace pwv
