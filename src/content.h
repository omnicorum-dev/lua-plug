#pragma once

#include "juce/juce.h"
#include "juce/parameter_layout.h"
#include "juce/sample_tap.h"

#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "lua_handler.h"
#include "processor.h"

#include "theme.h"
#include "ui/dropdown.h"
#include "ui/knob.h"
#include "ui/spectrum_view.h"
#include "ui/toggle.h"

class Content : public juce::Component, private juce::ChangeListener {
  public:
    static constexpr int design_width  = 800;
    static constexpr int design_height = 1300;

    // CONSTRUCTOR: initialize widgets
    Content(APVTS &apvts_, const Theme &t, SampleTap &input, SampleTap &output,
            Processor &processor_)
        : theme(t), apvts(apvts_),
          // initialize all components
          bypass(theme, getBool(Processor::Params::bypass_ID)),
          macro1(theme, getFloat(apvts, Processor::Params::macro1_ID)),
          macro2(theme, getFloat(apvts, Processor::Params::macro2_ID)),
          macro3(theme, getFloat(apvts, Processor::Params::macro3_ID)),
          macro4(theme, getFloat(apvts, Processor::Params::macro4_ID)),
          macro5(theme, getFloat(apvts, Processor::Params::macro5_ID)),
          macro6(theme, getFloat(apvts, Processor::Params::macro6_ID)),
          macro7(theme, getFloat(apvts, Processor::Params::macro7_ID)),
          macro8(theme, getFloat(apvts, Processor::Params::macro8_ID)),
          input_gain(theme, getFloat(apvts, Processor::Params::inGain_ID)),
          output_gain(theme, getFloat(apvts, Processor::Params::outGain_ID)),
          mix(theme, getFloat(apvts, Processor::Params::mix_ID)),
          input_spectrum(theme, input, false),
          output_spectrum(theme, output, true), processor(processor_) {
        //
        // setting initial plugin size
        setSize(design_width, design_height);

        // ADD AND MAKE VISIBLE
        addAndMakeVisible(bypass);
        addAndMakeVisible(input_spectrum);
        addAndMakeVisible(output_spectrum);

        addAndMakeVisible(macro1);
        addAndMakeVisible(macro2);
        addAndMakeVisible(macro3);
        addAndMakeVisible(macro4);
        addAndMakeVisible(macro5);
        addAndMakeVisible(macro6);
        addAndMakeVisible(macro7);
        addAndMakeVisible(macro8);

        addAndMakeVisible(input_gain);
        addAndMakeVisible(output_gain);
        addAndMakeVisible(mix);

        editor.setMultiLine(true, true);
        editor.setTabKeyUsedAsCharacter(true);
        editor.setReturnKeyStartsNewLine(true);
        editor.setScrollbarsShown(true);
        editor.setCaretVisible(true);
        editor.setReadOnly(false);
        editor.setFont(Fonts::jetBrainsMono(30.f));
        addAndMakeVisible(editor);

        editor.setText(processor.getLuaDraft(), false);
        editor.onTextChange = [this] {
            processor.setLuaDraft(editor.getText());
        };
        processor.addChangeListener(this);

        confirm.onClick = [this] { onCompile(); };
        addAndMakeVisible(confirm);

        error_box.setMultiLine(true, true);
        error_box.setReadOnly(true);
        error_box.setCaretVisible(false);
        error_box.setScrollbarsShown(true);
        error_box.setInterceptsMouseClicks(false, false);
        error_box.setFont(Fonts::jetBrainsMono(20.f));
        error_box.setJustification(juce::Justification::topLeft);
        addChildComponent(error_box); // added, but starts hidden
        error_box.setVisible(false);

        // click anywhere on the plugin dismisses the box while it's showing
        addMouseListener(this, true);
    }

    ~Content() override { processor.removeChangeListener(this); }

    // Draw non-widget elements (background)
    void paint(juce::Graphics &g) override {
        // Background
        g.fillAll(theme.background);
        auto area = getLocalBounds();

        // Plugin Header
        auto header = area.removeFromTop(100);
        g.setColour(theme.panel);
        g.fillRect(header);

        auto title_area = header.reduced(25);
        g.setColour(theme.text);
        g.setFont(Fonts::jetBrainsMonoBold(50));
        g.drawText("LUA PLUGIN", title_area, juce::Justification::centredRight);

        auto non_spectrum_area = area.withTrimmedTop(200);
        g.setColour(theme.panel);
        g.fillRect(non_spectrum_area);
    }

    // Set bounds of all widgets
    void resized() override {
        auto area = getLocalBounds();

        // Header
        auto header       = area.removeFromTop(100);
        auto header_inner = header.reduced(25);
        bypass.setBounds(
            header_inner.removeFromLeft(50).withSizeKeepingCentre(50, 50));

        // Input/Output Spectrum
        spectrum_bounds = area.removeFromTop(200);
        input_spectrum.setBounds(spectrum_bounds);
        output_spectrum.setBounds(spectrum_bounds);

        error_box.setBounds(spectrum_bounds);

        // Body
        auto panel_bounds = area.reduced(25);

        // Entry field
        auto editor_bounds = panel_bounds.removeFromTop(350);
        editor.setBounds(editor_bounds);

        panel_bounds.removeFromTop(25);
        auto button_bounds = panel_bounds.removeFromTop(50);
        confirm.setBounds(button_bounds);

        panel_bounds.removeFromTop(25);

        // Macros: split remaining panel_bounds into two rows of four
        auto macro_row_1 =
            panel_bounds.removeFromTop(panel_bounds.getHeight() / 3);
        auto macro_row_2 =
            panel_bounds.removeFromTop(panel_bounds.getHeight() / 2);
        auto standard_row = panel_bounds;

        int col_width_1 = macro_row_1.getWidth() / 4;
        macro1.setBounds(macro_row_1.removeFromLeft(col_width_1));
        macro2.setBounds(macro_row_1.removeFromLeft(col_width_1));
        macro3.setBounds(macro_row_1.removeFromLeft(col_width_1));
        macro4.setBounds(macro_row_1); // remainder, absorbs any rounding

        int col_width_2 = macro_row_2.getWidth() / 4;
        macro5.setBounds(macro_row_2.removeFromLeft(col_width_2));
        macro6.setBounds(macro_row_2.removeFromLeft(col_width_2));
        macro7.setBounds(macro_row_2.removeFromLeft(col_width_2));
        macro8.setBounds(macro_row_2);

        int col_width_3 = standard_row.getWidth() / 3;
        mix.setBounds(standard_row.removeFromLeft(col_width_3));
        input_gain.setBounds(standard_row.removeFromLeft(col_width_3));
        output_gain.setBounds(standard_row.removeFromLeft(col_width_3));
    }

    void onCompile() {
        compile_error = processor.compileLua(editor.getText());
        showing_error = compile_error.isNotEmpty();

        error_box.setText(compile_error, false);
        error_box.setVisible(showing_error);
    }

    // Update all widgets that require sample rate
    void frame(double fs) {
        input_spectrum.update(fs);
        output_spectrum.update(fs);
    }

    void mouseDown(const juce::MouseEvent &) override {
        if (showing_error) {
            showing_error = false;
            error_box.setVisible(false);
        }
    }

  private:
    void changeListenerCallback(juce::ChangeBroadcaster *) override {
        auto draft = processor.getLuaDraft();
        if (draft != editor.getText()) {
            editor.setText(draft, false);
        }
    }

  private:
    juce::AudioParameterBool &getBool(const char *id) {
        auto *p =
            dynamic_cast<juce::AudioParameterBool *>(apvts.getParameter(id));
        jassert(p != nullptr);
        return *p;
    }

    static juce::AudioParameterChoice &getChoice(APVTS &apvts, const char *id) {
        auto *p =
            dynamic_cast<juce::AudioParameterChoice *>(apvts.getParameter(id));
        jassert(p != nullptr);
        return *p;
    }

    static juce::AudioParameterInt &getInt(APVTS &apvts, const char *id) {
        auto *p =
            dynamic_cast<juce::AudioParameterInt *>(apvts.getParameter(id));
        jassert(p != nullptr);
        return *p;
    }

    static juce::RangedAudioParameter &getFloat(APVTS &apvts, const char *id) {
        auto *p = apvts.getParameter(id);
        jassert(p != nullptr);
        return *p;
    }

  private:
    const Theme &theme;
    APVTS       &apvts;

    // UI Components
    BypassToggle bypass;

    juce::TextEditor editor;
    juce::TextButton confirm{"Compile"};

    juce::String compile_error;
    bool         showing_error = false;

    juce::Rectangle<int> spectrum_bounds;
    juce::TextEditor     error_box;

    Knob macro1;
    Knob macro2;
    Knob macro3;
    Knob macro4;
    Knob macro5;
    Knob macro6;
    Knob macro7;
    Knob macro8;

    Knob input_gain;
    Knob output_gain;
    Knob mix;

    SpectrumView<4096> input_spectrum;
    SpectrumView<4096> output_spectrum;

    Processor &processor;
};
