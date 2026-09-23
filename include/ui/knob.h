#pragma once

#include "juce/juce.h"

#include "theme.h"
#include <memory>

class Knob : public juce::Component {
  public:
    explicit Knob(const Theme &t, juce::RangedAudioParameter &p,
                  juce::String label_text = {}, bool show_value_ = true)
        : theme(t), param(p),
          label(label_text.isEmpty() ? p.getName(32) : label_text),
          attachment(p,
                     [this](float real_value) {
                         norm = param.convertTo0to1(real_value);
                         repaint();
                     }),
          show_value(show_value_) {
        attachment.sendInitialUpdate();
    }

    void paint(juce::Graphics &g) override {
        const float filled_stroke = 10.f;
        const float bar_stroke    = 5.f;

        auto bounds     = getLocalBounds().toFloat();
        auto value_area = bounds.removeFromBottom(text_height);
        auto label_area = bounds.removeFromBottom(text_height);
        auto knob_area  = bounds;

        auto c = knob_area.getCentre();
        auto r = std::min(knob_area.getWidth(), knob_area.getHeight()) * .5f -
                 filled_stroke;

        const float a0 = juce::degreesToRadians(-145.f);
        const float a1 = juce::degreesToRadians(145.f);
        const float a  = a0 + norm * (a1 - a0);

        juce::Path track, fill;
        track.addCentredArc(c.x, c.y, r, r, 0.f, a0, a1, true);
        fill.addCentredArc(c.x, c.y, r, r, 0.f, a0, a, true);

        g.setColour(theme.background);
        g.strokePath(track,
                     juce::PathStrokeType(filled_stroke,
                                          juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

        g.setColour(theme.accent);
        g.strokePath(fill,
                     juce::PathStrokeType(filled_stroke,
                                          juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

        auto direction = juce::Point<float>(std::sin(a), -std::cos(a));
        auto bar_end   = c + direction * (r * 0.6f);
        auto bar_start = c + direction * (r * 0.2f);

        juce::Path bar_path;
        bar_path.startNewSubPath(bar_start);
        bar_path.lineTo(bar_end);
        g.setColour(theme.text);
        g.strokePath(bar_path,
                     juce::PathStrokeType(bar_stroke,
                                          juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));

        g.setFont(Fonts::jetBrainsMono(text_height));
        g.setColour(theme.text);
        g.drawText(label, label_area, juce::Justification::centred);

        if (show_value) {
            g.setColour(theme.text_dim);
            g.drawText(
                getValueText(), value_area, juce::Justification::centred);
        }
    }

  public:
    void mouseDown(const juce::MouseEvent &e) override {
        if (editor != nullptr) {
            finishEditing(true);
        }

        if (e.mods.isPopupMenu()) {
            showTextEditor();
            return;
        }

        dragging = true;
        attachment.beginGesture();
        drag_start_norm = norm;
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        if (!dragging)
            return;

        const float sensitivity = e.mods.isShiftDown() ? 750.f : 200.f;
        setNorm(drag_start_norm -
                (float)e.getDistanceFromDragStartY() / sensitivity);
    }

    void mouseUp(const juce::MouseEvent &) override {
        if (!dragging)
            return;

        dragging = false;
        attachment.endGesture();
    }

    void mouseDoubleClick(const juce::MouseEvent &e) override {
        if (e.mods.isPopupMenu())
            return;

        attachment.setValueAsCompleteGesture(
            param.convertFrom0to1(param.getDefaultValue()));
    }

  private:
    juce::String getValueText() const {
        auto text = param.getText(norm, 0);
        auto unit = param.getLabel();
        return unit.isEmpty() ? text : text + unit;
    }

    void setNorm(float n) {
        norm = std::clamp(n, 0.f, 1.f);
        attachment.setValueAsPartOfGesture(param.convertFrom0to1(norm));
        repaint();
    }

    void showTextEditor() {
        auto *top = getTopLevelComponent();

        if (top == nullptr || editor != nullptr) {
            return;
        }

        editor = std::make_unique<juce::TextEditor>();
        editor->setJustification(juce::Justification::centred);
        editor->setText(param.getText(norm, 0), false);
        editor->selectAll();

        editor->onReturnKey = [this] { finishEditing(true); };
        editor->onEscapeKey = [this] { finishEditing(false); };
        editor->onFocusLost = [this] { finishEditing(true); };

        juce::Rectangle<int> area(110, 24);
        area.setCentre(top->getLocalPoint(this, getLocalBounds().getCentre()));
        area = area.constrainedWithin(top->getLocalBounds());

        top->addAndMakeVisible(*editor);
        editor->setBounds(area);
        editor->grabKeyboardFocus();
    }

    void finishEditing(bool commit) {
        if (editor == nullptr || finishing) {
            return;
        }

        finishing = true;

        if (commit) {
            applyText(editor->getText());
        }

        juce::MessageManager::callAsync(
            [safe = juce::Component::SafePointer<Knob>(this)] {
                if (safe != nullptr) {
                    safe->editor.reset();
                    safe->finishing = false;
                }
            });
    }

    void applyText(const juce::String &text) {
        auto new_norm = param.getValueForText(text.trim());
        attachment.setValueAsCompleteGesture(param.convertFrom0to1(new_norm));
    }

  private:
    const Theme                &theme;
    juce::RangedAudioParameter &param;
    juce::String                label;
    juce::ParameterAttachment   attachment;

    std::unique_ptr<juce::TextEditor> editor;

    float norm            = 0.f;
    float drag_start_norm = 0.f;
    bool  dragging        = false;
    bool  finishing       = false;
    bool  show_value      = false;
};
