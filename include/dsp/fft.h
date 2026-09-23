#pragma once

#include <cassert>
#include <pffft/pffft.h>
#include <pffft/pffft_double.h>

#include <complex>
#include <vector>

/// @file
/// PFFFT wrapper, allowing for easy FFT and IFFT

namespace omni {

/// FFT and IFFT wrapper. Does not hold any input/output state
class FFT {
  public:
    /// Must declare FFT size at construction of object
    FFT(size_t size) : _size(size), work(size), buffer(size) {
        assert(size != 0 && "FFT size cannot be zero");
        setup = pffftd_new_setup((int)size, PFFFT_REAL);
        assert(setup && "Failed to create PFFFT setup");
    }

    ~FFT() {
        if (setup) {
            pffftd_destroy_setup(setup);
        }
    }

    FFT(const FFT &)            = delete;
    FFT &operator=(const FFT &) = delete;

    FFT(FFT &&other) noexcept
        : _size(other._size), setup(other.setup), work(std::move(other.work)),
          buffer(std::move(other.buffer)) {
        other.setup = nullptr;
    }

    FFT &operator=(FFT &&other) noexcept {
        if (this != &other) {
            if (setup)
                pffftd_destroy_setup(setup);
            _size       = other._size;
            setup       = other.setup;
            work        = std::move(other.work);
            buffer      = std::move(other.buffer);
            other.setup = nullptr;
        }
        return *this;
    }

    /// Forward FFT. Uses REAL inputs (time) and produces COMPLEX outputs
    /// (spectrum)
    void forward(const double *input, std::complex<double> *output) {
        pffftd_transform_ordered(
            setup, input, buffer.data(), work.data(), PFFFT_FORWARD);

        const size_t half = _size / 2;

        output[0] = {buffer[0], 0};

        for (size_t k = 1; k < half; ++k) {
            output[k] = {buffer[2 * k], buffer[2 * k + 1]};
        }

        output[half] = {buffer[1], 0};
    }

    /// Inverse FFT. Uses COMPLEX inputs (spectrum) and produces REAL
    /// outputs (time)
    void inverse(const std::complex<double> *input, double *output) {
        const size_t half = _size / 2;

        buffer[0] = input[0].real();
        buffer[1] = input[half].real();

        for (size_t k = 1; k < half; ++k) {
            buffer[2 * k]     = input[k].real();
            buffer[2 * k + 1] = input[k].imag();
        }

        pffftd_transform_ordered(
            setup, buffer.data(), output, work.data(), PFFFT_BACKWARD);

        const double scale = 1 / (double)_size;

        for (size_t i = 0; i < _size; ++i) {
            output[i] *= scale;
        }
    }

    /// Get the size of the FFT (temporal samples)
    size_t size() const { return _size; }

    /// Get the size of the spectrum output (number of bins)
    size_t spectrumSize() const { return _size / 2 + 1; }

  private:
    std::size_t _size;

    PFFFTD_Setup *setup = nullptr;

    std::vector<double> work;
    std::vector<double> buffer;
};

} // namespace omni
