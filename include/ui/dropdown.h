#pragma once

#include "juce/juce.h"
#include "theme.h"

class Dropdown : public juce::Component {
  public:
    explicit Dropdown(const Theme &t, juce::AudioParameterChoice &p)
        : theme(t), param(p), attachment(p, [this](float v) {
              index = (int)std::lround(v);
              repaint();
          }) {
        attachment.sendInitialUpdate();
    }

    void paint(juce::Graphics &g) override {
        auto area = getLocalBounds().toFloat();
        g.setColour(hovered ? theme.deselected : theme.background);
        g.fillRoundedRectangle(area, theme.round_radius);

        const float h          = area.getHeight();
        auto        arrow_area = area.removeFromRight(h).reduced(h * 0.3f);
        juce::Path  arrow;
        arrow.addTriangle(arrow_area.getX(),
                          arrow_area.getY() + h * 0.1f,
                          arrow_area.getRight(),
                          arrow_area.getY() + h * 0.1f,
                          arrow_area.getCentreX(),
                          arrow_area.getBottom() - h * 0.1f);
        g.setColour(theme.text_dim);
        g.fillPath(arrow);

        g.setFont(Fonts::jetBrainsMono(h * 0.45f));
        g.setColour(theme.text);
        g.drawText(param.choices[index],
                   area.reduced(12.f, 0.f),
                   juce::Justification::centredLeft);
    }

  public:
    void mouseEnter(const juce::MouseEvent &) override {
        hovered = true;
        repaint();
    }

    void mouseExit(const juce::MouseEvent &) override {
        hovered = false;
        repaint();
    }

    void mouseDown(const juce::MouseEvent &) override { showMenu(); }

    // optional: scroll wheel steps through the options
    void mouseWheelMove(const juce::MouseEvent &,
                        const juce::MouseWheelDetails &w) override {
        if (w.deltaY != 0.f)
            select(std::clamp(
                index + (w.deltaY < 0 ? 1 : -1), 0, param.choices.size() - 1));
    }

  private:
    void showMenu() {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&getLookAndFeel());
        for (int i = 0; i < param.choices.size(); ++i) {
            menu.addItem(i + 1, param.choices[i], true, i == index);
        }

        menu.showMenuAsync(
            juce::PopupMenu::Options()
                .withTargetComponent(this)
                .withMinimumWidth(getWidth()),
            [safe = juce::Component::SafePointer<Dropdown>(this)](int result) {
                if (safe != nullptr && result != 0) // 0 means dismissed
                    safe->select(result - 1);
            });
    }

    void select(int i) {
        index = i;
        repaint();
        attachment.setValueAsCompleteGesture((float)i);
    }

  private:
    const Theme                &theme;
    juce::AudioParameterChoice &param;
    juce::ParameterAttachment   attachment;
    int                         index   = 0;
    bool                        hovered = false;
};
