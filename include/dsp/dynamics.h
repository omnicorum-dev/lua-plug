#pragma once

#include "dsp/envelope_follower.h"
#include "general/basics.h"

namespace omni {

/// Downward/upward compressor/expander with soft knee.
class Dynamics {
  public:
    /// Which side of the threshold responds, and whether
    /// it compresses (pulls towards threshold) or
    /// expands (pushes away from threshold)
    enum class Mode {
        DOWNWARDS_COMPRESSION, ///< Attenuates signal above threshold
        UPWARDS_COMPRESSION,   ///< Boosts signal below threshold
        DOWNWARDS_EXPANSION,   ///< Attenuates signal below threshold
        UPWARDS_EXPANSION      ///< Boosts signal above threshold
    };

    /// Prepares the level detector(s).
    /// @param _sample_rate Sample rate in Hz
    /// @param _buffer_size Block size in samples
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;

        detector.prepare(_sample_rate, _buffer_size);
        detector.setMode(EnvelopeFollower::Mode::PEAK);
        detector.setAttackMs(attack_ms);
        detector.setReleaseMs(release_ms);

        pre_filter.prepare(_sample_rate, _buffer_size);
        pre_filter.setMode(EnvelopeFollower::Mode::RMS);
        pre_filter.setAttackMs(0.0);
        pre_filter.setReleaseMs(50.0);
    }

    /// Selects Downward/Upward Compression/Expansion
    void setType(Mode _mode) { mode = _mode; }

    /// Selects Peak or RMS detection for the level detector.
    void setFollowerMode(EnvelopeFollower::Mode _mode) {
        detector.setMode(_mode);
    }

    /// Sets attack time of the level detector in milliseconds. Attack
    /// is how fast the detector's envelope rises to meet a *louder*
    /// input, release how fast it falls back for a *quieter* one --
    /// the same convention every hardware/software dynamics processor
    /// uses, independent of Mode.
    void setAttack(double _attack_ms) {
        attack_ms = _attack_ms;
        detector.setAttackMs(attack_ms);
    }

    /// Sets release time of the level detector in milliseconds. See
    /// setAttack() for the attack/release convention.
    void setRelease(double _release_ms) {
        release_ms = _release_ms;
        detector.setReleaseMs(release_ms);
    }

    /// Sets the threshold in dB
    void setThreshold(double _threshold_dB) { threshold_dB = _threshold_dB; }

    /// Sets the compression/expansion ratio (e.g. 4 for 4:1)
    void setRatio(double _ratio) { ratio = _ratio; }

    /// Sets the knee width in dB. 0 -> hard knee
    void setKnee(double _knee_dB) {
        knee_dB      = _knee_dB;
        half_knee_dB = 0.5 * knee_dB;
    }

    /// Sets the maximum gain change (attenuation or boost) the gain
    /// computer is allowed to produce, in dB.
    void setRange(double _range_dB) { range_dB = std::abs(_range_dB); }

    /// Processes one sample: detects level, then applies a static
    /// (memoryless) gain curve to it -- see the class comment for why
    /// there is deliberately no second smoothing stage here.
    double processSample(double xn) {
        double detector_input = xn;

        if (mode == Mode::UPWARDS_COMPRESSION ||
            mode == Mode::DOWNWARDS_EXPANSION) {
            detector_input = pre_filter.processSample(xn);
        }

        double level    = detector.processSample(detector_input);
        double level_dB = mag2db(std::max(level, 1e-10));

        double gr_dB = calculateGain_dB(level_dB);
        gr_dB        = std::clamp(gr_dB, -range_dB, range_dB);

        double gain = db2mag(-gr_dB);
        return xn * gain;
    }

  protected:
    double calculateGain_dB(double level_dB) {
        double slope = ratio;
        if (mode == Mode::DOWNWARDS_COMPRESSION ||
            mode == Mode::UPWARDS_COMPRESSION) {
            slope = 1 / ratio;
        }

        int sign = -1;
        if (mode == Mode::DOWNWARDS_COMPRESSION ||
            mode == Mode::UPWARDS_EXPANSION) {
            sign = 1;
        }

        double e = sign * (level_dB - threshold_dB);
        double f = 0;

        if (e <= -half_knee_dB) {
            f = 0;
        } else if (e <= half_knee_dB) {
            f = (1 - slope) * std::pow(e + half_knee_dB, 2) / (2 * knee_dB);
        } else {
            f = e * (1 - slope);
        }

        return sign * f;
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    EnvelopeFollower detector;   ///< the one and only ballistics stage
    EnvelopeFollower pre_filter; ///< fixed RMS pre-smoother (see prepare())

    Mode mode = Mode::DOWNWARDS_COMPRESSION;

    double threshold_dB = 0;
    double ratio        = 1;
    double knee_dB      = 0;
    double half_knee_dB = 0;
    double range_dB     = 60;

    double attack_ms  = 10;
    double release_ms = 100;
};

} // namespace omni
