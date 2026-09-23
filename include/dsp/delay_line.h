#pragma once

#include "dsp/ring_buffer.h"
#include "general/basics.h"
#include "general/value_smoother.h"

/// @file
/// A feedback delay line built on RingBuffer (storage) and
/// ValueSmoother (click-free delay-time changes), with a pluggable
/// feedback-shaping hook for damping/saturation.

namespace omni {

/// Feedback delay line with smoothed delay-time changes.
/// @tparam max_buffer_size Maximum delay length in samples, fixed at
///                          compile time (see RingBuffer).
template <int max_buffer_size> class DelayLine {
  public:
    /// @param _sample_rate Audio sample rate in Hz.
    /// @param _buffer_size Host block size (currently unused).
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;

        smoother.prepare(fs);
        smoother.setType(SmoothingType::LINEAR);
        smoother.setSmoothingTime(0.02); // 20ms
        smoother.setCurrentAndTargetValue(smoother.getCurrentValue());
    }

    /// Sets the target delay in milliseconds; the read position glides
    /// there per the current smoothing settings rather than jumping.
    void setDelayTimeMs(double delay_ms) {
        double delay_samples = ms2samples(delay_ms, fs);
        delay_samples =
            std::clamp(delay_samples, 0., (double)(max_buffer_size - 2));
        smoother.setTargetValue(delay_samples);
    }

    /// Feedback gain, clamped to [-1, 1]
    void setFeedback(double _feedback) {
        feedback = std::clamp(_feedback, -1.0, 1.0);
    }

    /// See ValueSmoother::SmoothingType
    void setSmoothingType(SmoothingType type) { smoother.setType(type); }

    /// See ValueSmoother::setSmoothingTime()
    void setSmoothingTime(double time_sec) {
        smoother.setSmoothingTime(time_sec);
    }

    /// Function applied to the delayed sample before it is returned
    /// AND before it's written back for feedback. Both the output tap
    /// and the feedback path go through this. Use for damping, saturation,
    /// etc. Defaults to identity passthrough.
    void setFeedbackFunction(double (*function)(double)) {
        feedback_function = function;
    }

    /// Reads the delayed (and feedback_function-shaped) sample, then
    /// writes `xn + feedback * delayed` back into the buffer. Read
    /// happens before write, so the minimum achievable delay is 1 sample.
    double processSample(double xn) {
        double delay_samples = smoother.getValue();
        double delayed       = buffer.readFractional_linear(delay_samples);

        delayed = feedback_function(delayed);

        buffer.push(xn + feedback * delayed);

        return delayed;
    }

    /// Empties the buffer and resets the smoother to its current value
    void clear() {
        buffer.clear();
        smoother.setCurrentAndTargetValue(smoother.getCurrentValue());
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    RingBuffer<max_buffer_size> buffer;
    ValueSmoother               smoother;

    double feedback = 0.;

    double (*feedback_function)(double) = [](double xn) { return xn; };
};

} // namespace omni
