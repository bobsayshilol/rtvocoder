#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace pwv {

struct WAVData {
    std::size_t sampling_rate;
    std::vector<float> samples;
};

std::expected<WAVData, std::string> load_wav(std::filesystem::path path);
bool save_wav(WAVData const& data, std::filesystem::path path);

}  // namespace pwv
