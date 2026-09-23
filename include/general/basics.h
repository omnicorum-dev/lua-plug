#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

/// @file
/// Small stateless helpers: dB/linear conversions, RMS/peak measurement,
/// two simple waveshaping functions, and compile-time window-function
/// tables (Hann, Hamming, Blackman, Bartlett)

namespace omni {

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_2_PI
#define M_2_PI 0.636619772367581343076
#endif

/// @name Conversions
/// @{

/// dB -> linear amplitude
inline double db2mag(double db) { return std::pow(10, db / 20); }

/// linear amplitude -> dB
inline double mag2db(double mag) { return 20 * std::log10(mag); }

/// dB -> linear power (amplitude^2)
inline double db2pow(double db) { return std::pow(10, db / 10); }

/// linear power -> dB
inline double pow2db(double pow) { return 10 * std::log10(pow); }

/// center freq + bandwidth (Hz) -> Q
inline double bw2q(double f0, double bw) { return f0 / bw; }

/// center freq + Q -> bandwidth (Hz)
inline double q2bw(double f0, double q) { return f0 / q; }

/// sample count -> ms
inline double samples2ms(double samples, double sample_rate) {
    return (samples / sample_rate) * 1000;
}

/// ms -> sample count
inline double ms2samples(double ms, double sample_rate) {
    return (ms / 1000) * sample_rate;
}
/// @}

/// @name useful functions
/// @{

/// -1, 0, 1 depending on sign of x
inline int sign(double x) {
    if (x > 0)
        return 1;
    if (x < 0)
        return -1;
    return 0;
}

/// Root-mean-squared of `num_samples` values starting at `data`
inline double rms(const double *data, int num_samples) {
    double sum = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / num_samples);
}
inline float rms(const float *data, int num_samples) {
    float sum = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / (float)num_samples);
}

/// Peak (mean absolute value) of `num_samples` values starting at `data`
inline double peak(const double *data, int num_samples) {
    double max_val = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        max_val = std::max(max_val, std::abs(data[i]));
    }
    return max_val;
}
inline float peak(const float *data, int num_samples) {
    float max_val = 0;
    for (size_t i = 0; i < (size_t)num_samples; ++i) {
        max_val = std::max(max_val, std::abs(data[i]));
    }
    return max_val;
}

/// Soft-knee saturating waveshaper.
/// Larger `knee` tightens the knee
/// (saturated harder, closer in)
inline double sigmoid(double xn, double knee = 1) {
    return xn / (1 + std::abs(knee * xn));
}

/// Cubic soft clipper: parabolic below |xn| = 1
/// hard-clipped to +-1 beyond that.
inline double cubicClip(double xn) {
    if (std::abs(xn) > 1)
        return sign(xn);
    return xn - (xn * xn * xn) / 3.;
}
/// @}

/// @name Windowing functions and generators
/// n: sample index [0, N-1]. N: window length
/// @{

/// Hann window value at sample index `n` of `N`
constexpr double hann(int n, int N) {
    return 0.5 * (1.0 - std::cos(2 * M_PI * n / (N - 1)));
}

/// Compile-time generated Hann window table of length `window_size`
template <int window_size>
constexpr std::array<double, window_size> hannWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = hann(n, window_size);
    return window;
}();

/// Periodic (DFT-even) Hann value at sample index n of N -- unlike
/// hann() above, this is what constant-overlap-add reconstruction
/// needs (hann() is the symmetric/filter-design variant).
constexpr double hannPeriodic(int n, int N) {
    return 0.5 * (1.0 - std::cos(2 * M_PI * n / N));
}

template <int window_size>
constexpr std::array<double, window_size> sqrtHannWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = std::sqrt(
            hannPeriodic(n, window_size)); // not constexpr-safe pre-C++26
    return window;
}();

/// Runtime-sized counterpart, for code that picks its FFT size at
/// runtime rather than compile time (see STFT in stft.h).
inline std::vector<double> makeSqrtHannWindow(size_t N) {
    std::vector<double> window(N);
    for (size_t n = 0; n < N; ++n) {
        double w  = 0.5 * (1.0 - std::cos(2.0 * M_PI * (double)n / (double)N));
        window[n] = std::sqrt(std::max(0.0, w)); // clamp guards the last bit
                                                 // of float error right at w==0
    }
    return window;
}

/// Hamming window value at sample index `n` of `N`
constexpr double hamming(int n, int N) {
    return 0.54 - 0.46 * std::cos(2. * M_PI * n / (N - 1));
}

/// Compile-time generated Hamming window table of length `window_size`
template <int window_size>
constexpr std::array<double, window_size> hammingWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = hamming(n, window_size);
    return window;
}();

/// Blackman value at sample index `n` of `N`
constexpr double blackman(int n, int N) {
    constexpr double a0  = 0.42;
    constexpr double a1  = 0.5;
    constexpr double a2  = 0.08;
    double           arg = 2.0 * M_PI * n / (N - 1);
    return a0 - a1 * std::cos(arg) + a2 * std::cos(2. * arg);
}

/// Compile-time generated Blackman window table of length `window_size`
template <int window_size>
constexpr std::array<double, window_size> blackmanWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = blackman(n, window_size);
    return window;
}();

/// Bartlet window value at sample index `n` of `N`
constexpr double bartlet(int n, int N) {
    return 1 - std::abs(2. * n / (N - 1) - 1.);
}

/// Compile-time generated Bartlet window table of length `window_size`
template <int window_size>
constexpr std::array<double, window_size> bartletWindow = [] {
    std::array<double, window_size> window{};
    for (int n = 0; n < window_size; ++n)
        window[n] = bartlet(n, window_size);
    return window;
}();
/// @}

} // namespace omni
