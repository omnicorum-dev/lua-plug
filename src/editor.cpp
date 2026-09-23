#include "editor.h"
#include "juce/parameter_layout.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_graphics/juce_graphics.h"
#include "processor.h"

Editor::Editor(Processor &p, APVTS &a)
    : juce::AudioProcessorEditor(&p), audio_processor(p), apvts(a),
      content(p.apvts, theme, audio_processor.input_tap,
              audio_processor.output_tap, p) {

    setOpaque(true);
    addAndMakeVisible(content);

    setResizeLimits(Content::design_width / 4,
                    Content::design_height / 4,
                    Content::design_width * 3,
                    Content::design_height * 3);
    getConstrainer()->setFixedAspectRatio((double)Content::design_width /
                                          Content::design_height);

    setResizable(true, false);

    setSize(Content::design_width / 2, Content::design_height / 2);

    setLookAndFeel(&laf);

    auto ui = a.state.getOrCreateChildWithName("ui", nullptr);
    ui.setProperty("width", getWidth(), nullptr);
    ui.setProperty("height", getHeight(), nullptr);
}

Editor::~Editor() { setLookAndFeel(nullptr); }

void Editor::onFrame() {
    // Runs once per display refresh.
    // Poll data from audio thread here (lock-free!)
    // e.g. meter levels, spectrum data, etc.
    // repaint only widgets whose data changed
    content.frame(audio_processor.getSampleRate());
}

void Editor::paint(juce::Graphics &g) { g.fillAll(theme.background); }

void Editor::resized() {
    const float scale =
        std::min((float)getWidth() / (float)Content::design_width,
                 (float)getHeight() / (float)Content::design_height);
    content.setTransform(juce::AffineTransform::scale(scale));

    auto ui = apvts.state.getOrCreateChildWithName("ui", nullptr);
    ui.setProperty("width", getWidth(), nullptr);
    ui.setProperty("height", getHeight(), nullptr);
}
