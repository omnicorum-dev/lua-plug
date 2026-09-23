#pragma once

#include <array>
#include <cassert>
#include <complex>
#include <cstddef>
#include <initializer_list>
#include <vector>

#include "dsp/fft.h"
#include "dsp/ring_buffer.h"
#include "general/basics.h"
#include "general/fifo.h"
#include "general/overlap_add.h"

/// @file Short-Time Fourier Transform processor.

namespace omni {

#define SpectralFn [&](std::complex<double> * spectrum, size_t num_bins)

/// STFT processor. Processes an input signal in overlapping windows,
/// applying a user-provided spectral function to each frequency-domain
/// frame, and reconstructs the processed signal using overlap-add
///
/// Multiple FFT sizes can be provided at construction time, with one
/// selected as the active processing size.
///
/// @tparam max_size Maximum supported FFT size
template <size_t max_size> class STFT {
  public:
    static constexpr size_t max_spectrum_size = max_size / 2 + 1;

    /// Constructs an STFT processor with a set of supported FFT sizes
    /// The first size in @p sizes is selected as the initial active size
    /// @param sizes FFT sizes supported by the processor
    STFT(std::initializer_list<size_t> sizes) {
        slots.reserve(sizes.size());
        for (size_t size : sizes) {
            slots.emplace_back(size);
        }
        setSize(*sizes.begin());
    }

    /// Selects the active FFT size.
    /// Resets the internal processing state when the size changes
    void setSize(size_t size) {
        active = findSlot(size);
        history.clear();
        overlap_add.clear();
        wet_queue.clear();
        hop_counter = 0;
    }

    /// Get the processing latency in samples.
    /// This is the number of samples in the active hop size.
    size_t getLatencySamples() const { return active->hop_size; }

    /// Process a single input sample.
    /// Use general form `processSample(xn, SpectralFn { .. });`
    /// SpectralFn gives access to std::complex<double> *spectrum,
    /// and size_t num_bins
    ///
    /// Samples are accumulated into overlapping frames.
    /// Once a frame is ready, it is transformed to the frequency-domain
    /// and passed to @p spectral_fn for processing before being transformed
    /// back and reconstructed.
    ///
    /// @tparam SpectralFunction Callable type used to process the spectrum
    /// @param input Input sample
    /// @param spectral_fn Function applied to the frequency spectrum.
    template <typename SpectralFunction>
    double processSample(double input, SpectralFunction &&spectral_fn) {
        history.push(input);

        if (++hop_counter >= (int)active->hop_size) {
            hop_counter = 0;
            processFrame(spectral_fn);
        }

        return wet_queue.pop();
    }

  private:
    /// Stores processing state associated with an FFT size
    struct Slot {
        FFT                 fft;
        std::vector<double> window;
        size_t              size;
        size_t              hop_size;
        size_t              spectrum_size;

        Slot(size_t s)
            : fft(s), window(makeSqrtHannWindow(s)), size(s), hop_size(s / 2),
              spectrum_size(s / 2 + 1) {}
    };

  private:
    Slot *findSlot(size_t size) {
        for (auto &s : slots) {
            if (s.size == size)
                return &s;
        }
        assert(false && "STFT: Unsupported size requested");
        return &slots.front();
    }

    /// Processes one complete STFT frame
    ///
    /// Retrieves the most recent samples, applies the analysis window,
    /// performs the forward FFT, processes the spectrum, performs the IFFT,
    /// applies the synthesis window, and adds the result to the overlap-add
    /// buffer.
    ///
    /// @tparam SpectralFunction Callable type used to process the spectrum
    /// @param spectral_fn Function applied to the frequency-domain spectrum
    template <typename SpectralFunction>
    void processFrame(SpectralFunction &&spectral_fn) {
        if (history.getSize() < (int)active->size)
            return;

        history.getRecent(fft_input.data(), (int)active->size);

        for (size_t i = 0; i < active->size; ++i) {
            fft_input[i] *= active->window[i];
        }

        active->fft.forward(fft_input.data(), spectrum.data());

        spectral_fn(spectrum.data(), active->spectrum_size);

        active->fft.inverse(spectrum.data(), fft_output.data());

        for (size_t i = 0; i < active->size; ++i) {
            fft_output[i] *= active->window[i];
        }

        std::array<double, max_size> hop_out{};

        overlap_add.addFrameAndAdvance(
            fft_output.data(), active->size, active->hop_size, hop_out.data());

        wet_queue.push(hop_out.data(), active->hop_size);
    }

  private:
    std::vector<Slot> slots;
    Slot             *active = nullptr;

    RingBuffer<max_size>                                history{};
    std::array<double, max_size>                        fft_input{};
    std::array<std::complex<double>, max_spectrum_size> spectrum{};
    std::array<double, max_size>                        fft_output{};

    OverlapAddBuffer<max_size> overlap_add{};
    Fifo<max_size * 2>         wet_queue{};
    int                        hop_counter = 0;
};

} // namespace omni
