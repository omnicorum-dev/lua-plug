#pragma once

#include "content.h"
#include "juce/juce.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "processor.h"
#include "theme.h"

class Editor : public juce::AudioProcessorEditor {
  public:
    Editor(Processor &, APVTS &);
    ~Editor() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

  private:
    void onFrame();

  private:
    Processor &audio_processor;
    APVTS     &apvts;

    Theme theme;

    struct PluginLookAndFeel : juce::LookAndFeel_V4 {
        explicit PluginLookAndFeel(const Theme &t) {
            setColour(juce::PopupMenu::backgroundColourId, t.background);
            setColour(juce::PopupMenu::textColourId, t.text);
            setColour(juce::PopupMenu::highlightedBackgroundColourId, t.panel);
            setColour(juce::PopupMenu::highlightedTextColourId, t.text);

            setColour(juce::TextEditor::backgroundColourId, t.background);
            setColour(juce::TextEditor::textColourId, t.text);
            setColour(juce::TextEditor::highlightedTextColourId, t.text);

            setDefaultSansSerifTypeface(Fonts::jetBrainsMono());
        }

        juce::Font getPopupMenuFont() override {
            return Fonts::jetBrainsMono(30.f);
        }

    } laf{theme};

    Content content;

    juce::VBlankAttachment vblank{this, [this] { onFrame(); }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Editor)
};
