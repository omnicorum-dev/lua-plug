#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

/// @file Fixed-Size Buffer for overlap-add processing
/// Accumulates overlapping frames and outputs one hop
/// of samples at a time.

namespace omni {

/// Fixed-Size Buffer for overlap-add processing
/// Accumulates overlapping frames and outputs one hop
/// of samples at a time.
template <size_t maxSize> class OverlapAddBuffer {
  public:
    /// Clears all accumulated samples
    void clear() { buffer.fill(0.0); }

    /// Adds a frame to the buffer and outputs the next hop
    ///
    /// The first @p hopSize samples of the accumulated buffer
    /// are copied to the output. The remaining samples are
    /// shifted to the beginning of the buffer for the next frame.
    ///
    /// @param frame Input frame to add
    /// @param frameSize Number of samples in the frame
    /// @param hopSize Number of samples to output and advance by
    /// @param output Destination for the output samples
    void addFrameAndAdvance(const double *frame, size_t frameSize,
                            size_t hopSize, double *output) {
        for (size_t i = 0; i < frameSize; ++i)
            buffer[i] += frame[i];

        std::copy(buffer.begin(), buffer.begin() + hopSize, output);

        std::copy(buffer.begin() + hopSize,
                  buffer.begin() + frameSize,
                  buffer.begin());

        std::fill(buffer.begin() + (frameSize - hopSize),
                  buffer.begin() + frameSize,
                  0.0);
    }

  private:
    std::array<double, maxSize> buffer{};
};

} // namespace omni
