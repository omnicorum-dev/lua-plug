#pragma once

#include <array>
#include <cstddef>

/// @file Fixed-capacity sample FIFO queue with double-precision

namespace omni {

/// Fixed-capacity sample FIFO queue with double-precision
template <size_t capacity> class Fifo {
  public:
    /// Clears all stored samples and resets the FIFO state
    void clear() {
        buffer.fill(0.0);
        write_index = read_index = count = 0;
    }

    /// Adds a single sample to the FIFO
    /// If full, the oldest sample is overwritten
    void push(const double value) {
        buffer[write_index] = value;
        write_index         = (write_index + 1) % capacity;
        if (count < capacity)
            ++count;
        else
            read_index = (read_index + 1) % capacity;
    }

    /// Adds a buffer of values to the FIFO
    /// Samples are pushed in the order they appear in the buffer
    void push(const double *data, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            push(data[i]);
        }
    }

    /// Removes and returns the oldest sample
    /// Returns 0.0 if the FIFO is empty
    double pop() {
        if (count == 0)
            return 0.0;
        double v   = buffer[read_index];
        read_index = (read_index + 1) % capacity;
        --count;
        return v;
    }

  private:
    std::array<double, capacity> buffer{};

    size_t write_index = 0, read_index = 0, count = 0;
};

} // namespace omni
