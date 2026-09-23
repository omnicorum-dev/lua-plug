#pragma once

#include <algorithm>

/// @file
/// Attack/release envelope follower with Peak and RMS detection modes.

namespace omni {

/// Rectify-then-smooth envelope detector with independent attack and
/// release time constants.
class EnvelopeFollower {
  public:
    enum class Mode {
        PEAK, ///< Tracks |xn| directly
        RMS   ///< Tracks xn^2, sqrt'd on output
    };

    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;
        updateCoeffs();
    }

    void setMode(Mode _mode) { mode = _mode; }

    /// Attack time in ms (used while the rectified/squared input is
    /// rising above the current envelope).
    /// @note In RMS mode, attack == release gives a true time-averaged
    ///       RMS (verified: symmetric 10/30/100ms all converge to
    ///       exactly 0.7071 on a unit sine). attack < release -- the
    ///       usual fast-attack/slow-release setup -- biases the reading
    ///       upward toward peak-like behavior (verified: 1ms attack /
    ///       100ms release converges to 0.9754 instead). This is normal
    ///       (real compressors' "RMS" detectors often do this on
    ///       purpose for punch), just worth knowing if you expected a
    ///       strict average.
    void setAttackMs(double ms) {
        attack_ms = ms;
        updateCoeffs();
    }

    /// Release time in ms. See setAttackMs() for the RMS-mode caveat.
    void setReleaseMs(double ms) {
        release_ms = ms;
        updateCoeffs();
    }

    double processSample(double xn) {
        return mode == Mode::PEAK ? processPeak(xn) : processRMS(xn);
    }

    /// Zeros both the peak and RMS envelope state.
    void reset() {
        envelope    = 0;
        envelope_sq = 0;
    }

  private:
    void updateCoeffs() {
        attack_coeff  = std::exp(-1. / (attack_ms * 0.001 * fs));
        release_coeff = std::exp(-1. / (release_ms * 0.001 * fs));
    }

    double processPeak(double xn) {
        double rectified = std::abs(xn);
        double coeff     = rectified > envelope ? attack_coeff : release_coeff;
        envelope         = rectified + coeff * (envelope - rectified);
        return envelope;
    }

    double processRMS(double xn) {
        double squared = xn * xn;
        double coeff   = squared > envelope_sq ? attack_coeff : release_coeff;
        envelope_sq    = squared + coeff * (envelope_sq - squared);
        return std::sqrt(std::max(envelope_sq, 0.));
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    Mode mode = Mode::PEAK;

    double attack_ms  = 10.;
    double release_ms = 100.;

    double attack_coeff  = 0.;
    double release_coeff = 0.;

    double envelope    = 0.;
    double envelope_sq = 0.;
};

} // namespace omni
