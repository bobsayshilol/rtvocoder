#pragma once

#include <span>

namespace pwv {

class BandPass {
  public:
    BandPass(double sampling_rate, double hz, double q);
    ~BandPass();

    void reset(double sampling_rate, double hz, double q);

    void process(std::span<float> input);

  private:
    BandPass(BandPass const&) = delete;
    BandPass& operator=(BandPass const&) = delete;

  private:
};

}  // namespace pwv
