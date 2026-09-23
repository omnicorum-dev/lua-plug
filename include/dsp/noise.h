#pragma once

#include <cstddef>
#include <cstdint>

class NoiseGenerator {
  public:
    enum class Color {
        White,
        Pink,
        Brown,
        Blue,
    };

    void prepare(uint32_t seed = 0) {
        state = (seed == 0) ? 0x9E3779B9u : seed;
        reset();
    }

    void setColor(Color new_color) { color = new_color; }

    Color getColor() const { return color; }

    void reset() {
        pink_b0 = pink_b1 = pink_b2 = pink_b3 = pink_b4 = pink_b5 = pink_b6 =
            0.0;
        brown_state = 0.0;
        prev_white  = 0.0;
    }

    double processSample() {
        switch (color) {
        case Color::White:
            return processWhite();
        case Color::Pink:
            return processPink();
        case Color::Brown:
            return processBrown();
        case Color::Blue:
            return processBlue();
        }
    }

    void processBlock(double *out, size_t num_samples) {
        for (size_t i = 0; i < num_samples; ++i) {
            out[i] = processSample();
        }
    }

  private:
    double nextWhite() {
        // xorshift32 randomizer
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (double)(int32_t)(state) * (1.f / 2147483648.0f);
    }

    double processWhite() { return nextWhite(); }

    double processPink() {
        double white = nextWhite();

        pink_b0 = 0.99886 * pink_b0 + white * 0.0555179;
        pink_b1 = 0.99332 * pink_b1 + white * 0.0750759;
        pink_b2 = 0.96900 * pink_b2 + white * 0.1538520;
        pink_b3 = 0.86650 * pink_b3 + white * 0.3104856;
        pink_b4 = 0.55000 * pink_b4 + white * 0.5329522;
        pink_b5 = -0.7616 * pink_b5 - white * 0.0168980;

        double pink = pink_b0 + pink_b1 + pink_b2 + pink_b3 + pink_b4 +
                      pink_b5 + pink_b6 + white * 0.5362;
        pink_b6 = white * 0.115926;

        return pink * 0.11;
    }

    double processBrown() {
        double white = nextWhite();
        brown_state  = (brown_state + (0.02 * white)) / 1.02;
        return brown_state * 3.5;
    }

    double processBlue() {
        double white = nextWhite();
        double blue  = (white - prev_white) * 0.5;
        prev_white   = white;
        return blue;
    }

  private:
    uint32_t state = 0x9E3779B9u;
    Color    color = Color::White;

    double pink_b0 = 0., pink_b1 = 0., pink_b2 = 0., pink_b3 = 0., pink_b4 = 0.,
           pink_b5 = 0., pink_b6 = 0.;

    double brown_state = 0.f;

    double prev_white = 0.f;
};
