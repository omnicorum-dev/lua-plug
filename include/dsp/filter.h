#pragma once

#include <array>
#include <cmath>

/// @file
/// Biquad-family and state-variable filters: RBJ cookbook EQ types,
/// a Linkwitz-Riley crossover pair, a cascaded Butterworth, a DC
/// blocker, a TPT state-variable filter, and a one-pole smoother/filter.

namespace omni {

/// 2*pi; used to convert Hz to angular frequency (w0 = twoPi * f0 / fs).
constexpr double twoPi = 2.0 * M_PI;

/// Base class for a direct-form-I biquad. Holds the shared state,
/// parameter setters, and difference-equation processing.
/// Derived classes only need to fill in updateCoeffs()
class Biquad {
  public:
    Biquad() { reset(); }

    /// @param _sample_rate Sample rate in Hz
    /// @param _buffer_size Host block size
    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;
    }

    /// Resets f0/Q/A and coeffs to a pass-through state.
    void reset() {
        f0    = 1000.f;
        Q     = 0.7071f;
        A     = 1.f;
        w0    = 0;
        alpha = 0;
        b0    = 1;
        b1    = 0;
        b2    = 0;
        a0    = 1;
        a1    = 0;
        a2    = 0;
    }

    /// Set cutoff/center frequency in Hz
    void setFreq(float freq) {
        f0 = freq;
        updateCoeffs();
    }

    /// Set resonance/bandwidth
    void setQ(float q) {
        Q = q;
        updateCoeffs();
    }

    /// Set shelf/peak LINEAR gain.
    /// Used only in Bell, Highshelf, and Lowshelf variants
    void setA(float a) {
        A = a;
        updateCoeffs();
    }

    /// Set freq, Q, and A together to prevent repeated
    /// coefficient calculation
    void setAll(float freq, float q, float a) {
        f0 = freq;
        Q  = q;
        A  = a;
        updateCoeffs();
    }

    /// Process one sample through the direct-form-I difference equation.
    /// Coefficients are pre-divided by a0 so no division happens here.
    double processSample(double xn) {
        double feedforward = (b0 * xn) + (b1 * xnm1) + (b2 * xnm2);
        double feedback    = (a1 * ynm1) + (a2 * ynm2);
        double yn          = feedforward - feedback;

        xnm2 = xnm1;
        xnm1 = xn;
        ynm2 = ynm1;
        ynm1 = yn;

        return yn;
    }

  protected:
    virtual void updateCoeffs() = 0;

  protected:
    double fs          = 48000;
    int    buffer_size = 512;

    double f0, Q, A;
    double w0, alpha;
    double a0, a1, a2, b0, b1, b2;
    double xnm1, xnm2, ynm1, ynm2;
};

/// Selects the response shape for RBJ, Butterworth, and SVF filters
enum class FilterType {
    LOWPASS = 0,    ///< -12dB/oct lowpass (2-pole)
    HIGHPASS,       ///< -12dB/oct highpass (2-pole)
    BANDPASS_SKIRT, ///< Bandpass with constant skirt gain (peak gain = Q)
    BANDPASS_PEAK,  ///< Bandpass with constant 0dB peak gain
    NOTCH,          ///< Notch / band-reject
    BELL,           ///< Peaking EQ, boost/cut around f0 (uses A)
    HIGHSHELF,      ///< High shelf (uses A)
    LOWSHELF,       ///< Low shelf (uses A)
    ALLPASS,        ///< All-pass -- flat magnitude, phase shift only
};

/// RBJ (Robert Bristow-Johnson) audio EQ cookbook biquad
/// One coefficient set per FilterType
/// @see https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
class RBJ : public Biquad {
  public:
    /// Selects the response and recomputes coefficients
    void setFilterType(FilterType new_filter_type) {
        filter_type = new_filter_type;
        updateCoeffs();
    }

  protected:
    void updateCoeffs() override {
        w0           = twoPi * f0 / fs;
        alpha        = sin(w0) / (2 * Q);
        double coswo = cos(w0);

        switch (filter_type) {
        case FilterType::LOWPASS:
            b0 = (1 - coswo) / 2.f;
            b1 = 1 - coswo;
            b2 = b0;
            a0 = 1 + alpha;
            a1 = -2 * coswo;
            a2 = 1 - alpha;
            break;
        case FilterType::HIGHPASS:
            b0 = (1 + coswo) / 2.f;
            b1 = -(1 + coswo);
            b2 = b0;
            a0 = 1 + alpha;
            a1 = -2 * coswo;
            a2 = 1 - alpha;
            break;
        case FilterType::BANDPASS_SKIRT:
            b0 = Q * alpha;
            b1 = 0;
            b2 = -b0;
            a0 = 1 + alpha;
            a1 = -2 * coswo;
            a2 = 1 - alpha;
            break;
        case FilterType::BANDPASS_PEAK:
            b0 = alpha;
            b1 = 0;
            b2 = -alpha;
            a0 = 1.f + alpha;
            a1 = -2.f * coswo;
            a2 = 1.f - alpha;
            break;
        case FilterType::NOTCH:
            b0 = 1.f;
            b1 = -2.f * coswo;
            b2 = 1.f;
            a0 = 1.f + alpha;
            a1 = b1;
            a2 = 1.f - alpha;
            break;
        case FilterType::BELL:
            b0 = 1 + (alpha * A);
            b1 = -2 * coswo;
            b2 = 1 - (alpha * A);
            a0 = 1 + (alpha / A);
            a1 = b1;
            a2 = 1 - (alpha / A);
            break;
        case FilterType::HIGHSHELF: {
            double twoasqrtA = 2 * alpha * std::sqrt(A);
            double ap1       = A + 1;
            double am1       = A - 1;

            b0 = A * ((ap1) + ((am1)*coswo) + (twoasqrtA));
            b1 = -2 * A * ((am1) + ((ap1)*coswo));
            b2 = A * ((ap1) + ((am1)*coswo) - (twoasqrtA));
            a0 = (ap1) - ((am1)*coswo) + (twoasqrtA);
            a1 = 2 * ((am1) - ((ap1)*coswo));
            a2 = (ap1) - ((am1)*coswo) - (twoasqrtA);
            break;
        }
        case FilterType::LOWSHELF: {
            double twoasqrtA = 2 * alpha * std::sqrt(A);
            double ap1       = A + 1;
            double am1       = A - 1;

            b0 = A * ((ap1) - ((am1)*coswo) + (twoasqrtA));
            b1 = 2 * A * ((am1) - ((ap1)*coswo));
            b2 = A * ((ap1) - ((am1)*coswo) - (twoasqrtA));
            a0 = (ap1) + ((am1)*coswo) + (twoasqrtA);
            a1 = -2 * ((am1) + ((ap1)*coswo));
            a2 = (ap1) + ((am1)*coswo) - (twoasqrtA);
            break;
        }
        case FilterType::ALLPASS:
            b0 = 1 - alpha;
            b1 = -2 * coswo;
            b2 = 1 + alpha;
            a0 = b2;
            a1 = b1;
            a2 = b0;
            break;
        default:
            break;
        }

        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

  private:
    FilterType filter_type = FilterType::LOWPASS;
};

/// 4th-order Linkwitz-Riley filter: two cascaded Q=0.7071 RBJ biquads,
/// giving -24dB/oct with flat summed magnitude at the crossover point --
/// the standard choice for a crossover network. Only LOWPASS/HIGHPASS
/// are valid; anything else is coerced to LOWPASS.
class LR4 {
  public:
    void prepare(float _sample_rate, int _buffer_size) {
        stage1.prepare(_sample_rate, _buffer_size);
        stage2.prepare(_sample_rate, _buffer_size);

        stage1.setQ(0.7071f);
        stage2.setQ(0.7071f);
    }

    /// LOWPASS or HIGHPASS only; anything else reverts to LOWPASS.
    void setFilterType(FilterType new_filter_type) {
        if (new_filter_type != FilterType::LOWPASS &&
            new_filter_type != FilterType::HIGHPASS) {
            new_filter_type = FilterType::LOWPASS;
        }

        stage1.setFilterType(new_filter_type);
        stage2.setFilterType(new_filter_type);
    }

    /// Crossover frequency in Hz.
    void setFreq(float new_freq) {
        stage1.setFreq(new_freq);
        stage2.setFreq(new_freq);
    }

    double processSample(double xn) {
        double wn = stage1.processSample(xn);
        return stage2.processSample(wn);
    }

  private:
    RBJ stage1, stage2;
};

/// One-pole DC blocker (leaky-integrator highpass) -- removes DC offset
/// and subsonic content without coloring the audible band.
class DCBlocker {
  public:
    double processSample(double xn) {
        double yn = xn - xnm1 + R * ynm1;

        xnm1 = xn;
        ynm1 = yn;

        return yn;
    }

  private:
    /// Pole radius; closer to 1 = lower cutoff.
    static constexpr double R = 0.9999;

    double xnm1 = 0.f;
    double ynm1 = 0.f;
};

/// Cascaded-biquad Butterworth filter of order 2*max_stages, built from
/// per-stage Q values derived from the maximally-flat pole layout.
/// @tparam max_stages Maximum number of cascaded 2nd-order stages
///                     (max achievable order = 2 * max_stages).
template <int max_stages> class Butterworth {
  public:
    void prepare(double _sample_rate, int _buffer_size) {
        sample_rate = _sample_rate;
        for (auto &s : stages)
            s.prepare(_sample_rate, _buffer_size);
        updateCoeffs();
    }

    double processSample(double xn) {
        double yn = xn;
        for (int i = 0; i < num_stages; ++i)
            yn = stages[i].processSample(yn);
        return yn;
    }

    /// LOWPASS or HIGHPASS only. Anything else reverts to LOWPASS
    void setFilterType(FilterType type) {
        if (type != FilterType::LOWPASS && type != FilterType::HIGHPASS) {
            type = FilterType::LOWPASS;
        }
        filter_type = type;
        for (int i = 0; i < num_stages; ++i)
            stages[i].setFilterType(filter_type);
        updateCoeffs();
    }

    /// Sets the active stage count, clamped to [1, max_stages]
    void setStages(int _stages) {
        num_stages = std::max(1, std::min(_stages, (int)max_stages));
        for (int i = 0; i < num_stages; ++i)
            stages[i].setFilterType(filter_type);
        updateCoeffs();
    }

    /// Cutoff frequency in Hz, shared by all stages
    void setFreq(float freq) {
        target_freq = freq;
        updateCoeffs();
    }

    /// Overall resonance, applied via the last stage's Q correction
    /// A Q of 0.7071 results in the classic high-order Butterworth
    void setQ(float q) {
        resonance_q = std::max(q, 0.001f);
        updateCoeffs();
    }

    /// Sets freq and Q together
    void setFreqAndQ(float freq, float q) {
        target_freq = freq;
        resonance_q = std::max(q, 0.001f);
        updateCoeffs();
    }

    int   getNumStages() const { return num_stages; }
    float getStageFreq(int i) const { return stage_freq[i]; }
    float getStageQ(int i) const { return stage_q[i]; }

  protected:
    /// Recomputes each stage's frequency/Q from num_stages, target_freq,
    /// and resonance_q (maximally-flat Butterworth pole placement).
    void updateCoeffs() {
        if (sample_rate <= 0.f || num_stages <= 0)
            return;

        std::array<float, max_stages> relFreq{};
        std::array<float, max_stages> q{};

        int   n    = num_stages;
        float Nord = (float)n * 2;

        for (int m = 0; m < n; ++m) {
            float theta =
                (2.f * ((float)m + 1.f) - 1.f) * (float)M_PI / (2.f * Nord);
            relFreq[m] = 1.f;
            q[m]       = 1.f / (2.f * std::cos(theta));
        }

        constexpr double neutralQ = 0.7071;
        q[n - 1] *= (resonance_q / neutralQ);

        for (int m = 0; m < num_stages; ++m) {
            stage_freq[m] = target_freq * relFreq[m];
            stage_q[m]    = q[m];
            stages[m].setFreq(stage_freq[m]);
            stages[m].setQ(stage_q[m]);
        }
    }

  protected:
    FilterType filter_type = FilterType::LOWPASS;
    int        num_stages  = 2;
    double     sample_rate = 48000.f;
    float      target_freq = 1000.f;

  private:
    double resonance_q = 0.7071;

    std::array<RBJ, max_stages>   stages;
    std::array<float, max_stages> stage_freq{};
    std::array<float, max_stages> stage_q{};
};

/// State-variable filter using Andy Simper/Cytomic's TPT (topology-
/// preserving transform) formulation. Outputs every FilterType from one
/// shared pair of integrator states and stays frequency-
/// accurate under fast cutoff modulation.
/// @see https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
class SVF {
  public:
    SVF() { clear(); }

    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;
        updateCoeffs();
    }

    /// Processes one sample and advances the states
    double processSample(double xn) {
        double v3 = xn - ic2eq;
        double v1 = a1 * ic1eq + a2 * v3;
        double v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2 * v1 - ic1eq;
        ic2eq = 2 * v2 - ic2eq;

        return (m0 * xn) + (m1 * v1) + (m2 * v2);
    }

    /// Set cutoff/center frequency in Hz
    void setFreq(float freq) {
        f0 = freq;
        updateCoeffs();
    }

    /// Set resonance/bandwidth
    void setQ(float q) {
        Q = q;
        updateCoeffs();
    }

    /// Sets shelf/peak LINEAR gain (Bell/Highshelf/Lowshelf only)
    void setA(float a) {
        A = a;
        updateCoeffs();
    }

    /// Sets freq, Q, and A together to prevent repeated coeff calculation
    void setAll(float freq, float q, float a) {
        f0 = freq;
        Q  = q;
        A  = a;
        updateCoeffs();
    }

    /// Selects the response and recalculated coefficients
    void setFilterType(FilterType new_filter_type) {
        filter_type = new_filter_type;
        updateCoeffs();
    }

    /// Zeros integrator state to default to passthrough
    void clear() {
        ic1eq = 0;
        ic2eq = 0;

        a1 = 0;
        a2 = 0;
        a3 = 0;
        m0 = 1; // passthrough
        m1 = 0;
        m2 = 0;
    }

  protected:
    /// Recomputes a1..a3 and the m0..m2 output mix for the current
    /// filter_type/f0/Q/A, via prewarped g = tan(pi*f0/fs).
    void updateCoeffs() {
        double g = tan(M_PI * f0 / fs);
        double k = filter_type == FilterType::BELL ? 1. / (Q * A) : 1. / Q;

        a1 = 1. / (1. + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;

        switch (filter_type) {
        case FilterType::LOWPASS:
            m0 = 0;
            m1 = 0;
            m2 = 1;
            break;
        case FilterType::HIGHPASS:
            m0 = 1;
            m1 = -k;
            m2 = -1;
            break;
        case FilterType::BANDPASS_SKIRT:
            m0 = 0;
            m1 = 1;
            m2 = 0;
            break;
        case FilterType::BANDPASS_PEAK:
            m0 = 1;
            m1 = -k;
            m2 = -2;
            break;
        case FilterType::NOTCH:
            m0 = 1;
            m1 = -k;
            m2 = 0;
            break;
        case FilterType::BELL:
            m0 = 1;
            m1 = k * (A * A - 1);
            m2 = 0;
            break;
        case FilterType::HIGHSHELF:
            m0 = A * A;
            m1 = k * (1 - A) * A;
            m2 = (1 - A * A);
            break;
        case FilterType::LOWSHELF:
            m0 = 1.;
            m1 = k * (A - 1.);
            m2 = A * A - 1.;
            break;
        case FilterType::ALLPASS:
            m0 = 1;
            m1 = -2 * k;
            m2 = 0;
            break;
        }
    }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    FilterType filter_type = FilterType::LOWPASS;

    double f0 = 1000.;
    double Q  = 0.7071;
    double A  = 1.;

    double ic1eq = 0;
    double ic2eq = 0;

    double a1, a2, a3;
    double m0, m1, m2;
};

/// One-pole filter/smoother. Can act as an actual LOWPASS/HIGHPASS
/// filter via setCutoff(), or as a smoother/envelope-follower element
/// via setTimeConstant() -- see each method for which to use when.
class OnePole {
  public:
    enum class Type {
        LOWPASS, ///< Standard one-pole lowpass
        HIGHPASS ///< Complement of the lowpass (xn - lowpass)
    };

    void prepare(double _sample_rate, int _buffer_size) {
        fs          = _sample_rate;
        buffer_size = _buffer_size;
    }

    double processSample(double xn) {
        z = a0 * xn + b1 * z;
        return type == Type::LOWPASS ? z : xn - z;
    }

    /// Selects LOWPASS or HIGHPASS
    void setType(Type new_type) { type = new_type; }

    /// Frequency-based form -- use when acting as an actual filter
    void setCutoff(double freq) {
        f0 = freq;
        b1 = std::exp(-twoPi * f0 / fs);
        a0 = 1. - b1;
    }

    /// Time-constant form -- use when this is acting as a smoother or
    /// envelope-follower element instead of a filter. `samples` is how
    /// long it takes to reach ~63% (1 - 1/e) of a step change.
    void setTimeConstant(double samples) {
        b1 = std::exp(-1.0 / samples);
        a0 = 1.0 - b1;
    }

    /// Zeros filter state
    void reset() { z = 0.; }

  private:
    double fs          = 48000;
    int    buffer_size = 512;

    Type type = Type::LOWPASS;

    double f0 = 1000.;
    double a0 = 1.;
    double b1 = 0.;
    double z  = 0.;
};

} // namespace omni
