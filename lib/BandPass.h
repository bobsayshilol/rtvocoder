#pragma once

#include <span>

namespace pwv {

class BandPass {
  public:
    BandPass(float sampling_rate, float hz, float q);
    ~BandPass();

    void reset(float sampling_rate, float hz, float q);

    void process(std::span<float> input);

  private:
    BandPass(BandPass const&) = delete;
    BandPass& operator=(BandPass const&) = delete;

  private:
};

}  // namespace pwv
