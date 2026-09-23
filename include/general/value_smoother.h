#pragma once

#include <algorithm>

/// @file
/// A value smoother that ramps to a target either linearly (exact
/// arrival time) or exponentially (one-pole-style time constant),
/// selectable at runtime. Built for DelayLine's click-free delay-time
/// changes, but generic -- a JUCE-free alternative to SmoothedValue for
/// anything outside actual plugin parameters.

namespace omni {

/// @see ValueSmoother::setSmoothingTime -- "smoothing time" means
///      something different in each mode.
enum class SmoothingType { LINEAR, EXPONENTIAL };

class ValueSmoother {
  public:
    void prepare(double _sample_rate) { fs = _sample_rate; }

    /// Takes effect on the next setSmoothingTime()/setTargetValue()
    void setType(SmoothingType _type) { type = _type; }

    /// Sets how long a change takes to settle.
    /// LINEAR -- exact time to reach target
    /// EXPONENTIAL -- time constant (time to close ~63%)
    void setSmoothingTime(double time_seconds) {
        ramp_samples = std::max(1.0, time_seconds * fs);
        b1           = std::exp(-1.0 / ramp_samples);
    }

    /// Jumps immediately to `value` without ramping
    void setCurrentAndTargetValue(double value) {
        current = target  = value;
        increment         = 0.;
        samples_remaining = 0;
    }

    /// Sets a new target to smooth toward
    void setTargetValue(double value) {
        if (value == target)
            return;
        target = value;

        if (type == SmoothingType::LINEAR) {
            samples_remaining = (int)ramp_samples;
            increment         = (target - current) / ramp_samples;
        }
    }

    /// Advances the smoothing by one step and returns a new value.
    /// Call once per sample in the signal path.
    double getValue() {
        if (type == SmoothingType::LINEAR) {
            if (samples_remaining <= 0)
                return current;
            current += increment;
            if (--samples_remaining == 0)
                current = target;
        } else {
            current = target + (current - target) * b1;
            if (std::abs(target - current) < settle_threshold)
                current = target;
        }
        return current;
    }

    /// Current value without advancing
    double getCurrentValue() const { return current; }

    /// True while still ramping/settling toward the target
    bool isSmoothing() const {
        return type == SmoothingType::LINEAR
                   ? samples_remaining > 0
                   : std::abs(target - current) >= settle_threshold;
    }

  private:
    double        fs   = 48000;
    SmoothingType type = SmoothingType::LINEAR;

    double current = 0;
    double target  = 0;

    // LINEAR state
    double ramp_samples      = 1;
    double increment         = 0;
    int    samples_remaining = 0;

    // EXPONENTIAL state
    double b1 = 0;

    static constexpr double settle_threshold = 1e-5;
};

} // namespace omni
