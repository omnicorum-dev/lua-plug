#pragma once

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"

/// @file
/// APVTS parameter-layout builder helpers, plus per-parameter
/// reader/smoother wrappers (pFloat/pBool/pInt) for use inside
/// processBlock. This is the one file in the template scoped to
/// depend on JUCE directly (parameter handling is the exception).

#define SCFloat static constexpr float
#define SCInt static constexpr int
#define SCBool static constexpr bool
#define SCString static constexpr const char *
#define SCStringArr inline static const juce::StringArray

namespace omni {

typedef juce::AudioProcessorValueTreeState APVTS;

/// Common NormalisableRange skew values for addFloat().
struct Skew {
    SCFloat linear      = 1.0f; ///< No skew
    SCFloat exponential = 3.f;  /// Biases toward low-end (e.g. frequency knob)
    SCFloat logarithmic = 0.3f; /// Biases toward high-end
};

/* ======================================================== */

/// @name Parameter-layout builders
/// Each adds one parameter to `layout`; call while building
/// createParameterLayout() in your processor.
/// @{

/// @param skew   NormalisableRange skew (see Skew); 1.0 = linear.
/// @param suffix Unit label shown in the host (e.g. " dB").
inline void addFloat(APVTS::ParameterLayout &layout, const char *ID,
                     const char *name, const float min, const float max,
                     const float defaultValue, const float stepSize,
                     const float skew = 1.f, const char *suffix = "") {
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ID, 1),
        name,
        juce::NormalisableRange<float>(min, max, stepSize, skew),
        defaultValue,
        juce::AudioParameterFloatAttributes().withLabel(suffix)));
}

inline void addBool(APVTS::ParameterLayout &layout, const char *ID,
                    const char *name, const bool defaultValue) {
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ID, 1), name, defaultValue));
}

inline void addInt(APVTS::ParameterLayout &layout, const char *ID,
                   const char *name, const int min, const int max,
                   const int defaultValue, const char *suffix = "") {
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(ID, 1),
        name,
        min,
        max,
        defaultValue,
        juce::AudioParameterIntAttributes().withLabel(suffix)));
}

inline void addChoice(APVTS::ParameterLayout &layout, const char *ID,
                      const char *name, juce::StringArray choices,
                      const int defaultValue) {
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ID, 1), name, choices, defaultValue));
}
/// @}

/* ======================================================== */

/// Reads a float APVTS parameter and exposes a smoothed (ramped)
/// version via juce::SmoothedValue, for use in the per-sample loop.
/// @note addFloat/addBool/addInt each have a p* reader below; addChoice
///       doesn't have a pChoice counterpart yet.
class pFloat {
  public:
    pFloat() { apvts = nullptr; }

    /// @param _ID         Must match the ID passed to addFloat() for this
    ///                    parameter.
    /// @param _smoothRate Smoothing ramp time in seconds.
    void prepare(double _sampleRate, int _blockSize,
                 juce::AudioProcessorValueTreeState *_apvts, const char *_ID,
                 double _smoothRate = 0.02) {
        smoothRate = _smoothRate;
        ID         = _ID;
        apvts      = _apvts;
        reset(_sampleRate, _blockSize);
    }

    /// Re-syncs the smoother to the parameter's current value (no ramp)
    void reset(double _sampleRate, int _blockSize) {
        fs        = _sampleRate;
        blockSize = _blockSize;

        smoothValue.reset(fs, smoothRate);
        smoothValue.setCurrentAndTargetValue(
            apvts->getRawParameterValue(ID)->load());
    }

    /// Last raw (unsmoothed) value seen by update().
    float getRaw() { return rawValue; }

    /// The raw value before that.
    float getPrev() { return prevValue; }

    /// True if the raw value moved since the last update().
    bool changed() { return std::abs(rawValue - prevValue) > 0.001f; }

    /// Call once per block: re-reads the APVTS parameter and, if it
    /// changed, retargets the smoother. Returns the new raw value.
    float update() {
        prevValue    = rawValue;
        float newRaw = apvts->getRawParameterValue(ID)->load();
        if (newRaw != rawValue) {
            rawValue = newRaw;
            smoothValue.setTargetValue(rawValue);
        }
        return newRaw;
    }

    /// Advances the smoother by one sample (or `skip` samples) and
    /// returns the smoothed value. Call in the per-sample loop.
    float getNextValue(int skip = 0) {
        if (skip == 0) {
            return smoothValue.getNextValue();
        } else {
            return smoothValue.skip(skip);
        }
    }

    /// Smoothed value without advancing.
    float getCurrentValue() { return smoothValue.getCurrentValue(); }

  private:
    juce::AudioProcessorValueTreeState *apvts;
    juce::SmoothedValue<float>          smoothValue;
    const char                         *ID;
    float                               rawValue  = 0;
    float                               prevValue = 0;
    double                              fs;
    double                              smoothRate;
    int                                 blockSize;
};

/// Reads a bool APVTS parameter (thresholded at 0.5). No smoothing --
/// read as a discrete, block-rate value.
class pBool {
  public:
    void prepare(double _sampleRate, int _blockSize,
                 juce::AudioProcessorValueTreeState *_apvts, const char *_ID) {
        ID    = _ID;
        apvts = _apvts;
        reset(_sampleRate, _blockSize);
    }

    void reset(double _sampleRate, int _blockSize) {
        fs        = _sampleRate;
        blockSize = _blockSize;

        param = apvts->getRawParameterValue(ID);
    }

    /// Re-reads the parameter; call once per block.
    bool getNextValue() {
        prevValue = rawValue;
        rawValue  = param->load() > 0.5f;
        return rawValue;
    }

    /// Last value read, without re-reading.
    bool getCurrentValue() { return rawValue; }

    bool changed() { return rawValue != prevValue; }

  private:
    juce::AudioProcessorValueTreeState *apvts;
    std::atomic<float>                 *param = nullptr;
    const char                         *ID;
    bool                                rawValue  = false;
    bool                                prevValue = false;
    double                              fs;
    int                                 blockSize;
};

/// Reads an int APVTS parameter. Same shape as pBool -- discrete,
/// block-rate, no smoothing.
class pInt {
  public:
    void prepare(double _sampleRate, int _blockSize,
                 juce::AudioProcessorValueTreeState *_apvts, const char *_ID) {
        ID    = _ID;
        apvts = _apvts;
        reset(_sampleRate, _blockSize);
    }

    void reset(double _sampleRate, int _blockSize) {
        fs        = _sampleRate;
        blockSize = _blockSize;

        param = apvts->getRawParameterValue(ID);
    }

    int getNextValue() {
        prevValue = rawValue;
        rawValue  = static_cast<int>(param->load());
        return rawValue;
    }

    int getCurrentValue() { return rawValue; }

    bool changed() { return rawValue != prevValue; }

  private:
    juce::AudioProcessorValueTreeState *apvts;
    std::atomic<float>                 *param = nullptr;
    const char                         *ID;
    int                                 rawValue  = 0;
    int                                 prevValue = 0;
    double                              fs;
    int                                 blockSize;
};

} // namespace omni
