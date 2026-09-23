#pragma once

#include "juce/juce.h"
#include "theme.h"

// Generic Toggle switch
class Toggle : public juce::Component {
  public:
    explicit Toggle(const Theme &t, juce::AudioParameterBool &p,
                    juce::String label_text = {})
        : theme(t), attachment(p,
                               [this](float v) {
                                   on = v >= 0.5f;
                                   repaint();
                               }),
          label(label_text.isEmpty() ? p.getName(32) : label_text) {
        attachment.sendInitialUpdate();
    }

    void paint(juce::Graphics &g) override {
        auto bounds     = getLocalBounds().toFloat();
        auto value_area = bounds.removeFromBottom(text_height);
        (void)value_area;
        auto label_area = bounds.removeFromBottom(text_height);

        auto toggle_area = bounds.withSizeKeepingCentre(
            bounds.getWidth(),
            juce::jmin(bounds.getHeight(), bounds.getWidth() / 2));

        g.setColour(theme.background);
        g.fillRoundedRectangle(toggle_area, theme.round_radius);

        constexpr float inset = 5.f;

        toggle_area = toggle_area.reduced(inset);
        g.setColour(theme.panel);
        g.fillRoundedRectangle(toggle_area, theme.round_radius - inset);

        if (on) {
            g.setColour(theme.accent);
            g.fillRoundedRectangle(
                toggle_area.removeFromRight(toggle_area.getWidth() / 2),
                theme.round_radius - inset);
        } else {
            g.setColour(theme.text_dim);
            g.fillRoundedRectangle(
                toggle_area.removeFromLeft(toggle_area.getWidth() / 2),
                theme.round_radius - inset);
        }

        g.setFont(Fonts::jetBrainsMono(text_height));
        g.setColour(theme.text);
        g.drawText(label, label_area, juce::Justification::centred);
    }

  public:
    void mouseDown(const juce::MouseEvent &) override { setOn(!on); }

  private:
    void setOn(bool new_state) {
        on = new_state;
        attachment.setValueAsCompleteGesture(on ? 1.f : 0.f);
        repaint();
    }

  private:
    const Theme              &theme;
    juce::ParameterAttachment attachment;
    juce::String              label;
    bool                      on = false;
};

// Specifically a bypass toggle switch (lit when off)
class BypassToggle : public juce::Component {
  public:
    BypassToggle(const Theme &t, juce::AudioParameterBool &p)
        : theme(t), attachment(p, [this](float v) {
              on = v >= 0.5f;
              repaint();
          }) {
        attachment.sendInitialUpdate();
    }

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(on ? theme.deselected : theme.accent);
        g.fillRoundedRectangle(bounds, theme.round_radius);
    }

    void mouseEnter(const juce::MouseEvent &) override {
        hovered = true;
        repaint();
    }

    void mouseExit(const juce::MouseEvent &) override {
        hovered = false;
        repaint();
    }

    void mouseDown(const juce::MouseEvent &) override { setOn(!on); }

  private:
    void setOn(bool new_state) {
        on = new_state;
        attachment.setValueAsCompleteGesture(on ? 1.f : 0.f);
        repaint();
    }

  private:
    const Theme              &theme;
    juce::ParameterAttachment attachment;
    bool                      on      = false;
    bool                      hovered = false;
};
