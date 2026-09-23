#pragma once

#include <array>
#include <cassert>
#include <cstddef>

/// @file
/// Fixed-capacity circular sample buffer with delay-style reads
/// (including linear-interpolated fractional reads) -- the storage
/// primitive behind DelayLine.

namespace omni {

/// Fixed-capacity circular buffer of `double` samples, indexed by delay
/// (samples ago) rather than by absolute position.
/// @tparam max_buffer_size Capacity in samples, fixed at compile time.
template <int max_buffer_size> class RingBuffer {
  public:
    /// Writes one sample, overwriting the oldest once full
    void push(double xn) {
        buffer[write_head]            = xn;
        buffer[write_head + capacity] = xn;

        write_head = wrap(write_head + 1);
        if (size < capacity) {
            ++size;
        }
    }

    /// Writes `num_samples` samples in order
    void push(const double *data, size_t num_samples) {
        for (size_t i = 0; i < num_samples; ++i) {
            push(data[i]);
        }
    }

    /// Reads the sample `delay_samples` ago (0 = most recently pushed)
    double read(int delay_samples = 0) const {
        int index = wrap(write_head - 1 - delay_samples);
        return buffer[index];
    }

    /// Linearly-interpolated read at a fractional delay in samples
    /// @note the wrap() calls on d0/d0+1 here are redundant with the
    ///       wrapping read() already does internally -- harmless since
    ///       wrap() is idempotent, but worth knowing if you're tracing
    ///       through this.
    double readFractionalLinear(double delay_samples) const {
        int    d0   = (int)delay_samples;
        double frac = delay_samples - d0;
        double s0   = read(d0);
        double s1   = read(d0 + 1);
        return s0 + frac * (s1 - s0);
    }

    /// Copies the most recently pushed samples to an output buffer
    /// Samples are copied in chronological order, from oldest to
    /// newest.
    /// @param output Destination buffer for the samples
    /// @param num_samples Number of recent samples to retrieve
    ///
    /// @note @p num_samples must not exceed the current number of samples
    ///       stored in the buffer
    void getRecent(double *output, size_t num_samples) const {
        assert(num_samples <= size);
        const int start = wrap(static_cast<int>(write_head - num_samples));
        std::copy(buffer.begin() + start,
                  buffer.begin() + start + num_samples,
                  output);
    }

    /// Zeros the buffer and resets read/write position
    void clear() {
        write_head = 0;
        size       = 0;
        buffer.fill(0);
    }

    size_t        getCapacity() const { return capacity; }
    size_t        getSize() const { return size; }
    bool          isEmpty() const { return size == 0; }
    bool          isFull() const { return size == capacity; }
    const double *getData() const { return buffer.data(); }
    size_t        getWriteHead() const { return write_head; }

  protected:
    int wrap(int index) const {
        const int cap = static_cast<int>(capacity);
        return ((index % cap) + cap) % cap;
    }

  private:
    static_assert(max_buffer_size > 0, "rb2: capacity must be positive");

    std::array<double, max_buffer_size * 2> buffer{};

    int capacity = max_buffer_size; // was size_t — kept as int so `wrap`'s
                                    // signed-modulo trick works correctly
    int write_head = 0; // for negative inputs (e.g. delay > write_head)
    int size       = 0;
};

/// Fixed-capacity circular buffer of `double` samples, indexed by delay
/// (samples ago) rather than by absolute position.
/// @tparam max_buffer_size Capacity in samples, fixed at compile time.
/*
template <int max_buffer_size> class RingBuffer {
  public:
    /// Writes one sample, overwriting the oldest once full
    void push(double xn) {
        buffer[(size_t)write_head] = xn;
        write_head                 = wrap(write_head + 1);
        if (size < capacity)
            ++size;
    }

    /// Writes `num_samples` samples in order
    void push(const double *data, int num_samples) {
        for (int i = 0; i < num_samples; ++i)
            push(data[i]);
    }

    /// Reads the sample `delay_samples` ago (0 = most recently pushed)
    double read(int delay_samples = 0) const {
        int index = wrap(write_head - 1 - delay_samples);
        return buffer[index];
    }

    /// Linearly-interpolated read at a fractional delay in samples
    /// @note the wrap() calls on d0/d0+1 here are redundant with the
    ///       wrapping read() already does internally -- harmless since
    ///       wrap() is idempotent, but worth knowing if you're tracing
    ///       through this.
    double readFractional_linear(double delay_samples) {
        int    d0   = (int)delay_samples;
        double frac = delay_samples - d0;
        double s0   = read(wrap(d0));
        double s1   = read(wrap(d0 + 1));
        return s0 + frac * (s1 - s0);
    }

    /// Copies the most recently pushed samples to an output buffer
    /// Samples are copied in chronological order, from oldest to
    /// newest.
    /// @param output Destination buffer for the samples
    /// @param num_samples Number of recent samples to retrieve
    ///
    /// @note @p num_samples must not exceed the current number of samples
    ///       stored in the buffer
    void getRecent(double *output, int num_samples) const {
        assert(num_samples <= size);

        const int start = wrap(write_head - num_samples);

        const int first = std::min(num_samples, capacity - start);

        std::copy(
            buffer.begin() + start, buffer.begin() + start + first, output);

        if (first < num_samples) {
            std::copy(buffer.begin(),
                      buffer.begin() + (num_samples - first),
                      output + first);
        }
    }

    /// Zeros the buffer and resets read/write position
    void clear() {
        write_head = 0;
        size       = 0;
        buffer.fill(0);
    }

    int           getCapacity() const { return capacity; }
    int           getSize() const { return size; }
    bool          isEmpty() const { return size == 0; }
    bool          isFull() const { return size == capacity; }
    const double *getData() const { return buffer.data(); }
    int           getWriteHead() const { return write_head; }

  protected:
    int wrap(int index) const {
        return ((index % capacity) + capacity) % capacity;
    }

  private:
    std::array<double, max_buffer_size> buffer{};

    int capacity   = max_buffer_size;
    int write_head = 0;
    int size       = 0;
};
*/

} // namespace omni
