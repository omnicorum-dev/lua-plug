#include "processor.h"
#include "editor.h"
#include "juce/midi_cursor.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include "lua_handler.h"
#include <cmath>

/* ======================================================== */

Processor::Processor()
    : juce::AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "Parameters", createParameterLayout()) {
}

Processor::~Processor() = default;

/* ======================================================== */

APVTS::ParameterLayout Processor::createParameterLayout() {
    APVTS::ParameterLayout layout;

    addFloat(layout,
             Params::inGain_ID,
             Params::inGain_name,
             Params::inGain_min,
             Params::inGain_max,
             Params::inGain_default,
             Params::inGain_stepSize,
             Params::inGain_skew,
             Params::inGain_suffix);

    addFloat(layout,
             Params::outGain_ID,
             Params::outGain_name,
             Params::outGain_min,
             Params::outGain_max,
             Params::outGain_default,
             Params::outGain_stepSize,
             Params::outGain_skew,
             Params::outGain_suffix);

    addFloat(layout,
             Params::mix_ID,
             Params::mix_name,
             Params::mix_min,
             Params::mix_max,
             Params::mix_default,
             Params::mix_stepSize,
             Params::mix_skew,
             Params::mix_suffix);

    addFloat(layout,
             Params::macro1_ID,
             Params::macro1_name,
             Params::macro1_min,
             Params::macro1_max,
             Params::macro1_default,
             Params::macro1_stepSize,
             Params::macro1_skew,
             Params::macro1_suffix);

    addFloat(layout,
             Params::macro2_ID,
             Params::macro2_name,
             Params::macro2_min,
             Params::macro2_max,
             Params::macro2_default,
             Params::macro2_stepSize,
             Params::macro2_skew,
             Params::macro2_suffix);

    addFloat(layout,
             Params::macro3_ID,
             Params::macro3_name,
             Params::macro3_min,
             Params::macro3_max,
             Params::macro3_default,
             Params::macro3_stepSize,
             Params::macro3_skew,
             Params::macro3_suffix);

    addFloat(layout,
             Params::macro4_ID,
             Params::macro4_name,
             Params::macro4_min,
             Params::macro4_max,
             Params::macro4_default,
             Params::macro4_stepSize,
             Params::macro4_skew,
             Params::macro4_suffix);

    addFloat(layout,
             Params::macro5_ID,
             Params::macro5_name,
             Params::macro5_min,
             Params::macro5_max,
             Params::macro5_default,
             Params::macro5_stepSize,
             Params::macro5_skew,
             Params::macro5_suffix);

    addFloat(layout,
             Params::macro6_ID,
             Params::macro6_name,
             Params::macro6_min,
             Params::macro6_max,
             Params::macro6_default,
             Params::macro6_stepSize,
             Params::macro6_skew,
             Params::macro6_suffix);

    addFloat(layout,
             Params::macro7_ID,
             Params::macro7_name,
             Params::macro7_min,
             Params::macro7_max,
             Params::macro7_default,
             Params::macro7_stepSize,
             Params::macro7_skew,
             Params::macro7_suffix);

    addFloat(layout,
             Params::macro8_ID,
             Params::macro8_name,
             Params::macro8_min,
             Params::macro8_max,
             Params::macro8_default,
             Params::macro8_stepSize,
             Params::macro8_skew,
             Params::macro8_suffix);

    addBool(
        layout, Params::bypass_ID, Params::bypass_name, Params::bypass_default);

    return layout;
}

/* ======================================================== */

void Processor::prepareToPlay(double sample_rate, int buffer_size) {
    outGainSmooth.prepare(sample_rate, buffer_size, &apvts, Params::outGain_ID);
    inGainSmooth.prepare(sample_rate, buffer_size, &apvts, Params::inGain_ID);
    mixSmooth.prepare(sample_rate, buffer_size, &apvts, Params::mix_ID);

    bypassParam.prepare(sample_rate, buffer_size, &apvts, Params::bypass_ID);

    macro1Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro1_ID);
    macro2Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro2_ID);
    macro3Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro3_ID);
    macro4Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro4_ID);
    macro5Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro5_ID);
    macro6Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro6_ID);
    macro7Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro7_ID);
    macro8Smooth.prepare(sample_rate, buffer_size, &apvts, Params::macro8_ID);

    lua_handler.prepare(sample_rate);

    // Recompile when the rate changes so top-level code that reads sampleRate()
    // sees the real value also covers state that was restored before the first
    // prepareToPlay
    if (sample_rate != last_sample_rate) {
        last_sample_rate = sample_rate;
        auto code        = getLuaCode();
        if (code.isNotEmpty()) {
            lua_handler.compile(code);
        }
    }
}

void Processor::releaseResources() {}

// If you want a custom editor, swap the commented line
juce::AudioProcessorEditor *Processor::createEditor() {
    // return new juce::GenericAudioProcessorEditor(*this);
    return new Editor(*this, apvts);
}

juce::String Processor::compileLua(const juce::String &code) {
    auto error = lua_handler.compile(code);
    if (error.isEmpty()) {
        const juce::ScopedLock sl(lua_text_lock);
        lua_code = code;
    }
    return error;
}

juce::String Processor::getLuaCode() const {
    const juce::ScopedLock sl(lua_text_lock);
    return lua_code;
}

juce::String Processor::getLuaDraft() const {
    const juce::ScopedLock sl(lua_text_lock);
    return lua_draft;
}

void Processor::setLuaDraft(const juce::String &text) {
    const juce::ScopedLock sl(lua_text_lock);
    lua_draft = text;
}

void Processor::processBlock(juce::AudioBuffer<float> &buffer,
                             juce::MidiBuffer         &messages) {

    juce::ScopedNoDenormals no_denormals;

    size_t total_input_channels  = (size_t)getTotalNumInputChannels();
    size_t total_output_channels = (size_t)getTotalNumOutputChannels();
    size_t num_samples           = (size_t)buffer.getNumSamples();

    for (auto i = total_input_channels; i < total_output_channels; ++i) {
        buffer.clear((int)i, 0, buffer.getNumSamples());
    }

    /* ======================================================== */

    // Read control-rate parameters
    bool bypass = bypassParam.getNextValue();

    /* ======================================================== */

    // Update smoothers
    outGainSmooth.update();
    inGainSmooth.update();
    mixSmooth.update();

    macro1Smooth.update();
    macro2Smooth.update();
    macro3Smooth.update();
    macro4Smooth.update();
    macro5Smooth.update();
    macro6Smooth.update();
    macro7Smooth.update();
    macro8Smooth.update();

    /* ======================================================== */

    if (bypass)
        return;

    LuaHandler::ScopedBlock lua(lua_handler);

    /* ======================================================== */

    // Update objects for discrete changes

    /* ======================================================== */

    MidiCursor midi(messages);

    constexpr size_t max_channels = 8;
    size_t           num_channels = total_output_channels;

    std::array<float *, max_channels> channel_ptrs;

    for (size_t channel = 0; channel < num_channels; ++channel) {
        channel_ptrs[channel] = buffer.getWritePointer((int)channel);
    }

    /* ======================================================== */

    // SAMPLE/CHANNEL LOOP

    for (size_t sample = 0; sample < num_samples; ++sample) {

        while (midi.hasEvent() && midi.event().samplePosition == (int)sample) {
            juce::MidiMessage message = midi.event().getMessage();

            /* ======================================================== */

            // apply changes based on the midi message received this sample

            /* ======================================================== */

            midi.advance();
        }

        float in_gain  = std::pow(10.f, inGainSmooth.getNextValue() / 20.f);
        float out_gain = std::pow(10.f, outGainSmooth.getNextValue() / 20.f);
        float mix      = mixSmooth.getNextValue();

        /* ======================================================== */

        // Read sample-rate parameters

        macro_values[0] = macro1Smooth.getNextValue();
        macro_values[1] = macro2Smooth.getNextValue();
        macro_values[2] = macro3Smooth.getNextValue();
        macro_values[3] = macro4Smooth.getNextValue();
        macro_values[4] = macro5Smooth.getNextValue();
        macro_values[5] = macro6Smooth.getNextValue();
        macro_values[6] = macro7Smooth.getNextValue();
        macro_values[7] = macro8Smooth.getNextValue();

        /* ======================================================== */

        // Update objects for continuous changes

        for (int i = 0; i < 8; ++i) {
            lua.setMacro(i, (double)macro_values[(size_t)i]);
        }

        /* ======================================================== */

        for (size_t channel = 0; channel < num_channels; ++channel) {
            float *channel_data = channel_ptrs[channel];
            float  dry          = channel_data[sample];
            float  xn           = dry * in_gain;

            // lua.setChannel((int)channel);

            if (channel == 0) {
                input_tap.push(xn);
            }

            /* ======================================================== */

            float yn = (float)lua.process(xn, (int)channel);
            if (!std::isfinite(yn)) {
                yn = 0.f;
            }

            /* ======================================================== */

            float mixed = (yn * mix * 0.01f) + (dry * (100.f - mix) * 0.01f);
            channel_data[sample] = mixed * out_gain;
        }
    }

    output_tap.push(buffer.getReadPointer(0), buffer.getNumSamples());
}

/* ======================================================== */

void Processor::getStateInformation(juce::MemoryBlock &dest) {
    auto state = apvts.copyState();

    {
        const juce::ScopedLock sl(lua_text_lock);
        state.setProperty(luaCode_ID, lua_code, nullptr);
        state.setProperty(luaDraft_ID, lua_draft, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void Processor::setStateInformation(const void *data, int size_in_bytes) {
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, size_in_bytes));
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return;

    auto tree  = juce::ValueTree::fromXml(*xml);
    auto code  = tree.getProperty(luaCode_ID).toString();
    auto draft = tree.getProperty(luaDraft_ID, code).toString();

    // Keep the Lua text out of apvts.state; the processor owns it.
    tree.removeProperty(luaCode_ID, nullptr);
    tree.removeProperty(luaDraft_ID, nullptr);
    apvts.replaceState(tree);

    setLuaDraft(draft);

    // Empty (old project / blank preset) -> reset to pass-through.
    compileLua(code.isNotEmpty() ? code : juce::String(LuaHandler::baseline));

    sendChangeMessage(); // async; tells an open editor to refresh
}

/* ======================================================== */

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new Processor();
}
