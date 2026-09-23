#pragma once

#include "juce/parameter_layout.h"
#include "juce/sample_tap.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include "juce_core/juce_core.h"
#include "lua_handler.h"

using namespace omni;

class Processor final : public juce::AudioProcessor,
                        public juce::ChangeBroadcaster {
  public:
    /* ======================================================== */

    // Constructor and destructor
    Processor();
    ~Processor() override;

    /* ======================================================== */

    const juce::String getName() const override { return JucePlugin_Name; }
    // juce::StringArray getAlternateDisplayNames() const override;

    void prepareToPlay(double sample_rate, int expected_block_size) override;

    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float> &buffer,
                      juce::MidiBuffer         &messages) override;

    double getTailLengthSeconds() const override { return 0; }

    bool hasEditor() const override { return true; }

    void getStateInformation(juce::MemoryBlock &dest_data) override;
    void setStateInformation(const void *data, int size_bytes) override;

    /* ======================================================== */

    bool acceptsMidi() const override {
#if (JucePlugin_IsMidiEffect || JucePlugin_IsSynth)
        return true;
#else
        return false;
#endif
    }

    bool producesMidi() const override {
#if JucePlugin_IsMidiEffect
        return true;
#else
        return false;
#endif
    }

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override {
        if (layouts.getMainOutputChannelSet() !=
                juce::AudioChannelSet::stereo() &&
            layouts.getMainOutputChannelSet() !=
                juce::AudioChannelSet::mono()) {
            return false;
        }
#if !JucePlugin_IsSynth
        if (layouts.getMainOutputChannelSet() !=
            layouts.getMainInputChannelSet())
            return false;
#endif
        return true;
    }

    /* ======================================================== */

    int                getNumPrograms() override { return 1; }
    int                getCurrentProgram() override { return 0; }
    void               setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void               changeProgramName(int, const juce::String &) override {}

    /* ======================================================== */

  private:
    // If you want a custom editor, remove the generic editor and write the
    // definition in the cpp. It should instead return your editor.
    juce::AudioProcessorEditor *createEditor() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Processor)

    /* ======================================================== */

  public:
    APVTS apvts;

    struct Params {
        // inGain (float)
        SCString inGain_ID       = "inGain";
        SCString inGain_name     = "Input Gain";
        SCString inGain_suffix   = " dB";
        SCFloat  inGain_min      = -60.f;
        SCFloat  inGain_max      = 12.f;
        SCFloat  inGain_default  = 0.f;
        SCFloat  inGain_stepSize = 0.1f;
        SCFloat  inGain_skew     = Skew::exponential;

        // outGain (float)
        SCString outGain_ID       = "outGain";
        SCString outGain_name     = "Output Gain";
        SCString outGain_suffix   = " dB";
        SCFloat  outGain_min      = -60.f;
        SCFloat  outGain_max      = 12.f;
        SCFloat  outGain_default  = 0.f;
        SCFloat  outGain_stepSize = 0.1f;
        SCFloat  outGain_skew     = Skew::exponential;

        // mix (float)
        SCString mix_ID       = "mix";
        SCString mix_name     = "Mix";
        SCString mix_suffix   = "%";
        SCFloat  mix_min      = 0.f;
        SCFloat  mix_max      = 100.f;
        SCFloat  mix_default  = 100.f;
        SCFloat  mix_stepSize = 0.1f;
        SCFloat  mix_skew     = 3.f;

        // macro1
        SCString macro1_ID       = "macro1";
        SCString macro1_name     = "Macro 1";
        SCString macro1_suffix   = "";
        SCFloat  macro1_min      = 0.f;
        SCFloat  macro1_max      = 1.f;
        SCFloat  macro1_default  = 0.f;
        SCFloat  macro1_stepSize = 0.001f;
        SCFloat  macro1_skew     = Skew::linear;

        // macro1
        SCString macro2_ID       = "macro2";
        SCString macro2_name     = "Macro 2";
        SCString macro2_suffix   = "";
        SCFloat  macro2_min      = 0.f;
        SCFloat  macro2_max      = 1.f;
        SCFloat  macro2_default  = 0.f;
        SCFloat  macro2_stepSize = 0.001f;
        SCFloat  macro2_skew     = Skew::linear;

        // macro1
        SCString macro3_ID       = "macro3";
        SCString macro3_name     = "Macro 3";
        SCString macro3_suffix   = "";
        SCFloat  macro3_min      = 0.f;
        SCFloat  macro3_max      = 1.f;
        SCFloat  macro3_default  = 0.f;
        SCFloat  macro3_stepSize = 0.001f;
        SCFloat  macro3_skew     = Skew::linear;

        // macro1
        SCString macro4_ID       = "macro4";
        SCString macro4_name     = "Macro 4";
        SCString macro4_suffix   = "";
        SCFloat  macro4_min      = 0.f;
        SCFloat  macro4_max      = 1.f;
        SCFloat  macro4_default  = 0.f;
        SCFloat  macro4_stepSize = 0.001f;
        SCFloat  macro4_skew     = Skew::linear;

        // macro1
        SCString macro5_ID       = "macro5";
        SCString macro5_name     = "Macro 5";
        SCString macro5_suffix   = "";
        SCFloat  macro5_min      = 0.f;
        SCFloat  macro5_max      = 1.f;
        SCFloat  macro5_default  = 0.f;
        SCFloat  macro5_stepSize = 0.001f;
        SCFloat  macro5_skew     = Skew::linear;

        // macro1
        SCString macro6_ID       = "macro6";
        SCString macro6_name     = "Macro 6";
        SCString macro6_suffix   = "";
        SCFloat  macro6_min      = 0.f;
        SCFloat  macro6_max      = 1.f;
        SCFloat  macro6_default  = 0.f;
        SCFloat  macro6_stepSize = 0.001f;
        SCFloat  macro6_skew     = Skew::linear;

        // macro1
        SCString macro7_ID       = "macro7";
        SCString macro7_name     = "Macro 7";
        SCString macro7_suffix   = "";
        SCFloat  macro7_min      = 0.f;
        SCFloat  macro7_max      = 1.f;
        SCFloat  macro7_default  = 0.f;
        SCFloat  macro7_stepSize = 0.001f;
        SCFloat  macro7_skew     = Skew::linear;

        // macro1
        SCString macro8_ID       = "macro8";
        SCString macro8_name     = "Macro 8";
        SCString macro8_suffix   = "";
        SCFloat  macro8_min      = 0.f;
        SCFloat  macro8_max      = 1.f;
        SCFloat  macro8_default  = 0.f;
        SCFloat  macro8_stepSize = 0.001f;
        SCFloat  macro8_skew     = Skew::linear;

        // bypass (bool)
        SCString bypass_ID      = "bypass";
        SCString bypass_name    = "Bypass";
        SCBool   bypass_default = false;
    };

    pFloat inGainSmooth;
    pFloat outGainSmooth;
    pFloat mixSmooth;

    pBool bypassParam;

    pFloat macro1Smooth;
    pFloat macro2Smooth;
    pFloat macro3Smooth;
    pFloat macro4Smooth;
    pFloat macro5Smooth;
    pFloat macro6Smooth;
    pFloat macro7Smooth;
    pFloat macro8Smooth;

    SampleTap input_tap;
    SampleTap output_tap;

    LuaHandler lua_handler;

    juce::String compileLua(const juce::String &code);

    juce::String getLuaCode() const;
    juce::String getLuaDraft() const;
    void         setLuaDraft(const juce::String &text);

    std::array<float, 8> macro_values{};

  private:
    static APVTS::ParameterLayout createParameterLayout();

    SCString luaCode_ID  = "luaCode";
    SCString luaDraft_ID = "luaDraft";

    mutable juce::CriticalSection lua_text_lock;
    juce::String                  lua_code;
    juce::String                  lua_draft;

    double last_sample_rate = 0.0;
};
