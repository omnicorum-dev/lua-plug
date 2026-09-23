#pragma once

#include "dsp/fft.h"
#include "juce/sample_tap.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "theme.h"

template <size_t fft_size> class SpectrumView : public juce::Component {
  public:
    static constexpr float min_hz = 20.f, max_hz = 20000.f;
    static constexpr float min_db = -60.f, max_db = 6.f;

    SpectrumView(const Theme &th, SampleTap &t, bool highlight = true)
        : theme(th), tap(t), fft(fft_size), window(fft_size),
          samples(fft_size, 0.0), input(fft_size), bins(fft.spectrumSize()),
          db(fft.spectrumSize(), min_db), use_accent(highlight) {
        for (size_t i = 0; i < fft_size; ++i) {
            window[i] =
                0.5 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi *
                                     (double)i / (double)fft_size);

            window_sum += window[i];
        }
    }

    void update(double sample_rate) {
        fs = sample_rate;

        float chunk[1024];
        bool  got = false;
        for (int n; (n = tap.read(chunk, 1024)) > 0;) {
            got = true;
            std::copy(samples.begin() + n, samples.end(), samples.begin());
            for (int i = 0; i < n; ++i) {
                samples[fft_size - (size_t)n + (size_t)i] = chunk[i];
            }
        }
        if (!got)
            return;

        for (size_t i = 0; i < fft_size; ++i) {
            input[i] = samples[i] * window[i];
        }
        fft.forward(input.data(), bins.data());

        const double norm = 2.0 / window_sum;
        for (size_t k = 0; k < bins.size(); ++k) {
            float d =
                (float)(20.0 *
                        std::log10(std::max(std::abs(bins[k]) * norm, 1e-9)));
            db[k] = d > db[k] ? d : db[k] + (d - db[k]) * 0.15f;
        }
        repaint();
    }

    void paint(juce::Graphics &g) override {
        auto b = getLocalBounds().toFloat();

        if (fs <= 0.0)
            return;

        const double hz_per_bin = fs / (double)fft_size;
        const size_t last       = db.size() - 1;

        auto x_to_hz = [&](float x) {
            return min_hz * std::pow(max_hz / min_hz, x / b.getWidth());
        };

        auto db_to_y = [&](float d) {
            float t =
                (std::clamp(d, min_db, max_db) - min_db) / (max_db - min_db);
            return b.getBottom() - t * b.getHeight();
        };

        juce::Path line;

        auto at = [&](double pos) {
            size_t i = std::min((size_t)pos, last - 1);
            return juce::jmap((float)(pos - (double)i), db[i], db[i + 1]);
        };

        for (float x = 0.f; x <= b.getWidth(); x += 2.f) {
            double lo = std::clamp(x_to_hz(x) / hz_per_bin, 0.0, (double)last);
            double hi =
                std::clamp(x_to_hz(x + 2.f) / hz_per_bin, 0.0, (double)last);

            float d = std::max(at(lo), at(hi));
            for (size_t i = (size_t)lo + 1; i <= (size_t)hi;
                 ++i) // bins inside this column
                d = std::max(d, db[i]);

            (x == 0.f) ? line.startNewSubPath(b.getX() + x, db_to_y(d))
                       : line.lineTo(b.getX() + x, db_to_y(d));
        }

        auto fill = line;
        fill.lineTo(b.getRight(), b.getBottom());
        fill.lineTo(b.getX(), b.getBottom());
        fill.closeSubPath();
        if (use_accent) {
            g.setColour(theme.accent.withAlpha(0.2f));
        } else {
            g.setColour(theme.text_dim.withAlpha(0.2f));
        }
        g.fillPath(fill);
        if (use_accent) {
            g.setColour(theme.accent);
        } else {
            g.setColour(theme.text_dim);
        }
        g.strokePath(line, juce::PathStrokeType(1.5f));

        // gridlines at 100 Hz / 1 kHz / 10 kHz
        g.setColour(theme.text_dim);
        for (float hz : {100.f, 1000.f, 10000.f}) {
            float x = b.getX() + b.getWidth() * std::log(hz / min_hz) /
                                     std::log(max_hz / min_hz);
            g.drawVerticalLine((int)x, b.getY(), b.getBottom());
        }
    }

  private:
    const Theme                      &theme;
    SampleTap                        &tap;
    omni::FFT                         fft;
    std::vector<double>               window, samples, input;
    std::vector<std::complex<double>> bins;
    std::vector<float>                db;
    double                            window_sum = 0.0;
    double                            fs         = 0.0;
    bool                              use_accent;
};
