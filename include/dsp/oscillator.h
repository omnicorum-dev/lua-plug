#pragma once

#include <cmath>

/// @file
/// A phase-accumulator oscillator driven by an arbitrary generator
/// function (defaults to sine), with phase/frequency modulation inputs.

namespace omni {

/// Output range for BasicOscillator::processSample.
enum class Polarity {
    UNIPOLAR, ///< Output in [0, 1]
    BIPOLAR,  /// Output in [-1, 1] (the generator function's native range)
};

/// Phase-accumulator oscillator. Not band-limited -- fine for sine via
/// the default generator, but a naive (non-PolyBLEP) generator for
/// anything with sharp edges (saw/square/pulse) will alias.
class BasicOscillator {
  public:
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;
        reset();
    }

    /// Sets the waveform generator, called each sample with phase in
    /// [0, 1). Defaults to a sine wave.
    void setGeneratorFunction(double (*function)(double)) {
        generatorFunction = function;
    }

    /// Recomputes phase_increment from frequency/sample rate; zeros phase.
    void reset() {
        if (fs > 0.f) {
            phase_increment = frequency / fs;
        }
        phase = 0;
    }

    /// Sets oscillator frequency in Hz.
    void setFrequency(double hz) {
        frequency = hz;
        if (fs > 0)
            phase_increment = frequency / fs;
    }

    void setPolarity(Polarity _polarity) { polarity = _polarity; }

    /// Constant phase offset in [0, 1) added at generation time.
    void setPhaseOffset(double offset) { phase_offset = offset; }

    /// Advances the oscillator by one sample and returns the
    /// (polarity-adjusted) output.
    /// @param phase_mod     Added to phase for this sample only (not
    ///                      accumulated).
    /// @param frequency_mod Added to frequency for this sample's phase
    ///                      advance only; if 0, the precomputed
    ///                      phase_increment is used instead.
    double processSample(double phase_mod = 0.0, double frequency_mod = 0.0) {
        double sample = generateSample(phase + phase_mod);

        phase += frequency_mod == 0.0 ? phase_increment
                                      : (frequency + frequency_mod) / fs;

        phase -= std::floor(phase);

        return applyPolarity(sample);
    }

    double getPhase() { return phase; }
    double getFrequency() { return frequency; }

  protected:
    double generateSample(double p) const {
        double ph = std::fmod(p + phase_offset, 1.0);
        if (ph < 0.0)
            ph += 1.0;

        return generatorFunction(ph);
    }

    double applyPolarity(double sample) const {
        if (polarity == Polarity::UNIPOLAR)
            return (sample + 1.0) * 0.5;
        return sample;
    }

    void advancePhase() {
        phase += phase_increment;
        if (phase >= 1.0)
            phase -= 1.0;
    }

  private:
    double phase_increment = 0;
    double phase           = 0;
    double phase_offset    = 0;
    double frequency       = 10;

    Polarity polarity = Polarity::BIPOLAR;

    double (*generatorFunction)(double) = [](double ph) {
        return std::sin(ph * 2 * M_PI);
    };

    double fs          = 0.f;
    int    buffer_size = 512;
};

} // namespace omni
