#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include <algorithm>
#include <array>

class SampleTap {
  public:
    static constexpr int capacity = 1 << 15;

    void push(const float *src, int n) {
        int s1, n1, s2, n2;
        fifo.prepareToWrite(n, s1, n1, s2, n2);
        std::copy(src, src + n1, buf.begin() + s1);
        std::copy(src + n1, src + n1 + n2, buf.begin() + s2);
        fifo.finishedWrite(n1 + n2);
    }

    int read(float *dest, int max) {
        int s1, n1, s2, n2;
        fifo.prepareToRead(max, s1, n1, s2, n2);
        std::copy(buf.begin() + s1, buf.begin() + s1 + n1, dest);
        std::copy(buf.begin() + s2, buf.begin() + s2 + n2, dest + n1);
        fifo.finishedRead(n1 + n2);
        return n1 + n2;
    }

    void push(float xn) { push(&xn, 1); }

  private:
    juce::AbstractFifo          fifo{capacity};
    std::array<float, capacity> buf{};
};
